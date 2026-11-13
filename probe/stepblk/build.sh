#!/bin/sh
# Usage: build.sh [ta-lib tree]   (default: the checkout this script is in)
# Builds ./probe and the -DPROBE_RESEED=1 mutant ./probe_mutant next to this
# script, linking the tree's cmake-build/libta-lib.a by path, never an
# installed library.
#
# Each inl_<x>.c includes the generated src/ta_func/ta_<X>.c and is compiled
# with the flags CMake used for that file, read from the tree's build
# directory. Linked before the archive, it defines every symbol of that
# archive member, so the member is never pulled and `ref` runs the same code.
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
TREE="${1:-$HERE/../..}"
TREE="$(cd "$TREE" && pwd)"
BUILD="$TREE/cmake-build"
LIB="$BUILD/libta-lib.a"
TGT="CMakeFiles/ta-lib-static.dir"
OBJ="$HERE/obj"

[ -f "$LIB" ] || { echo "build.sh: no $LIB" >&2; exit 1; }

# Sets LIBCC and LIBFLAGS to the compiler and flags CMake used for
# src/ta_func/$1.c in the static library target. LIBFLAGS is shell syntax,
# as make hands it to the shell: run it through eval.
lib_flags()
{
   f="$1"
   json="$BUILD/compile_commands.json"
   if [ -f "$json" ]; then
      cmd=$(grep '"command":' "$json" | grep -F "ta-lib-static.dir/src/ta_func/$f.c.o" | head -n 1 | sed \
            -e 's/^[[:space:]]*"command":[[:space:]]*"//' -e 's/",\{0,1\}[[:space:]]*$//' \
            -e 's/\\"/"/g' -e 's/\\\\/\\/g')
      [ -n "$cmd" ] || { echo "build.sh: $f.c not in $json" >&2; exit 1; }
      eval "set -- $cmd"
      LIBCC="$1"
      shift
      LIBFLAGS=""
      while [ $# -gt 0 ]; do
         case "$1" in
            -o|-c|-MT|-MF|-MQ) shift 2 ;;
            -MD|-MMD) shift ;;
            *) LIBFLAGS="$LIBFLAGS '$(printf '%s' "$1" | sed "s/'/'\\\\''/g")'"; shift ;;
         esac
      done
   elif [ -f "$BUILD/$TGT/flags.make" ]; then
      fm="$BUILD/$TGT/flags.make"
      LIBCC=$(sed -n 's/^# compile C with //p' "$fm")
      # Per-source flags sit between $(C_FLAGS) and -MD on the object's rule.
      extra=$(sed -n "s|.*\$(C_FLAGS) \(.*\) -MD -MT $TGT/src/ta_func/$f\.c\.o .*|\1|p" "$BUILD/$TGT/build.make")
      LIBFLAGS="$(sed -n 's/^C_DEFINES = //p' "$fm") $(sed -n 's/^C_INCLUDES = //p' "$fm") $(sed -n 's/^C_FLAGS = //p' "$fm") $extra"
   else
      echo "build.sh: neither compile_commands.json nor $TGT/flags.make under $BUILD" >&2
      exit 1
   fi
   [ -n "$LIBCC" ] || { echo "build.sh: no compiler found for $f.c" >&2; exit 1; }
}

mkdir -p "$OBJ"
OBJS=""
for f in ta_MIDPRICE ta_SMA ta_TRANGE ta_VAR ta_STDDEV ta_LINEARREG ta_EMA; do
   x=$(echo "$f" | sed 's/^ta_//' | tr 'ABCDEFGHIJKLMNOPQRSTUVWXYZ' 'abcdefghijklmnopqrstuvwxyz')
   lib_flags "$f"
   echo "$LIBCC $LIBFLAGS -I$TREE/src/ta_func -I$HERE -c inl_$x.c" > "$OBJ/inl_$x.cmd"
   eval "\"\$LIBCC\" $LIBFLAGS -I\"\$TREE/src/ta_func\" -I\"\$HERE\" -c \"\$HERE/inl_$x.c\" -o \"\$OBJ/inl_$x.o\""
   OBJS="$OBJS $OBJ/inl_$x.o"
done

if [ "$(uname -m)" = "x86_64" ]; then
   FLAGS="-O3 -ffp-contract=off -fno-math-errno -mfma -falign-functions=64 -falign-loops=64 -Wall -Wextra"
else
   FLAGS="-O3 -ffp-contract=off -fno-math-errno -Wall -Wextra"
fi
# shellcheck disable=SC2086
${CC:-cc} $FLAGS -I"$TREE/include" -I"$HERE" "$HERE/sqzmom_probe.c" $OBJS "$LIB" -lm -o "$HERE/probe"
# shellcheck disable=SC2086
${CC:-cc} $FLAGS -DPROBE_RESEED=1 -I"$TREE/include" -I"$HERE" "$HERE/sqzmom_probe.c" $OBJS "$LIB" -lm -o "$HERE/probe_mutant"
