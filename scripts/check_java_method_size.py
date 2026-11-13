#!/usr/bin/env python3
"""No generated Java method may exceed HotSpot's 8000-byte JIT limit.

HotSpot never JIT-compiles a method whose bytecode is over `HugeMethodLimit`
(8000, fixed in product builds, enforced while `DontCompileHugeMethods` is on,
the default). That method runs interpreted for good, an order of magnitude
slower, with no failing test and no changed output. A bench run with
`-XX:-TieredCompilation` hides it: C2 compiles the method there.

Sizes are read straight from the class files, not from javap: each method's
Code attribute `code_length`, exact, for every class under io/github/talib/.

KNOWN entries are debts, not approvals. When this goes red, shrink the method
or split it in its emitter (ta_codegen/generator/src/backends/java_stream.rs;
java_metadata.rs for metadata.Dispatch); never raise LIMIT, and add to KNOWN
only what the owner accepted as a debt.

Usage: check_java_method_size.py <classes-dir>
"""

import os
import struct
import sys

LIMIT = 8000

# (class relative to io.github.talib, method, descriptor or None while the
# name has no overload).
KNOWN = {
    ("Core", "cdladvanceblockOpenImpl", None),
    ("Core", "cdladvanceblockOpenSinkFrame", None),
    ("metadata.Dispatch", "call", None),
    ("metadata.Dispatch", "displayShift", None),
    ("metadata.Dispatch", "lookback", None),
}

# The library measures ~5700 today; far below means the walk or parse broke.
MIN_METHODS = 5000

# Constant-pool entry sizes after the tag byte, by tag (JVMS 4.4).
CP_SIZES = {3: 4, 4: 4, 5: 8, 6: 8, 7: 2, 8: 2, 9: 4, 10: 4, 11: 4, 12: 4,
            15: 3, 16: 2, 17: 4, 18: 4, 19: 2, 20: 2}


def die(msg: str):
    print("::error::%s" % msg)
    sys.exit(1)


def methods_of(path: str):
    """Yield (class, method, descriptor, code_length or None) for one class file."""
    with open(path, "rb") as f:
        b = f.read()
    if b[:4] != b"\xca\xfe\xba\xbe":
        die("%s is not a class file." % path)
    pos = 8
    (count,) = struct.unpack_from(">H", b, pos)
    pos += 2
    utf8, classes = {}, {}
    i = 1
    while i < count:
        tag = b[pos]
        pos += 1
        if tag == 1:
            (n,) = struct.unpack_from(">H", b, pos)
            utf8[i] = b[pos + 2:pos + 2 + n].decode("utf-8", "replace")
            pos += 2 + n
        elif tag in CP_SIZES:
            if tag == 7:
                classes[i] = struct.unpack_from(">H", b, pos)[0]
            pos += CP_SIZES[tag]
        else:
            die("%s: unknown constant-pool tag %d at entry %d." % (path, tag, i))
        i += 2 if tag in (5, 6) else 1

    _, this_class, _, n_ifaces = struct.unpack_from(">HHHH", b, pos)
    pos += 8 + 2 * n_ifaces
    cls = utf8[classes[this_class]]
    if not cls.startswith("io/github/talib/"):
        die("%s declares class %s, outside io/github/talib/." % (path, cls))
    cls = cls[len("io/github/talib/"):].replace("/", ".")

    def members(pos, want_code):
        (n,) = struct.unpack_from(">H", b, pos)
        pos += 2
        out = []
        for _ in range(n):
            _, name, desc, n_attrs = struct.unpack_from(">HHHH", b, pos)
            pos += 8
            code = None
            for _ in range(n_attrs):
                a_name, a_len = struct.unpack_from(">HI", b, pos)
                if want_code and utf8[a_name] == "Code":
                    code = struct.unpack_from(">I", b, pos + 6 + 4)[0]
                pos += 6 + a_len
            out.append((cls, utf8[name], utf8[desc], code))
        return pos, out

    pos, _ = members(pos, False)
    pos, methods = members(pos, True)
    return methods


def main():
    if len(sys.argv) != 2:
        die("usage: check_java_method_size.py <classes-dir>")
    root = os.path.join(sys.argv[1], "io", "github", "talib")
    if not os.path.isdir(root):
        die("%s does not exist: point this at the javac -d directory." % root)

    sized = []
    for d, _, files in os.walk(root):
        for f in sorted(files):
            if f.endswith(".class"):
                sized += [m for m in methods_of(os.path.join(d, f)) if m[3] is not None]
    if len(sized) < MIN_METHODS:
        die("measured only %d methods with bytecode under %s (floor %d): the "
            "class walk or the parse broke, so this gate measured almost nothing."
            % (len(sized), root, MIN_METHODS))

    def known(c, m, desc):
        return (c, m, None) in KNOWN or (c, m, desc) in KNOWN

    errors = []
    for c, m, desc in sorted(KNOWN, key=str):
        hits = [s for s in sized if s[0] == c and s[1] == m and desc in (None, s[2])]
        if len(hits) > 1:
            errors.append("KNOWN %s.%s matches %d overloads: add its descriptor."
                          % (c, m, len(hits)))
        elif not hits:
            errors.append("KNOWN %s.%s%s no longer exists: delete its row."
                          % (c, m, desc or ""))
        elif hits[0][3] <= LIMIT:
            errors.append("KNOWN %s.%s is %d bytes, back under %d: delete its row."
                          % (c, m, hits[0][3], LIMIT))

    over = sorted((s for s in sized if s[3] > LIMIT and not known(*s[:3])),
                  key=lambda s: -s[3])
    if over:
        errors.append("%s over HotSpot's %d-byte HugeMethodLimit, so never JIT-"
                      "compiled: %s. Shrink or split it in its emitter "
                      "(ta_codegen/generator/src/backends/java_stream.rs, or "
                      "java_metadata.rs for metadata.Dispatch)."
                      % ("Method" if len(over) == 1 else "Methods", LIMIT,
                         ", ".join("%s.%s%s at %d" % s for s in over)))
    if errors:
        for e in errors[:-1]:
            print("::error::%s" % e)
        die(errors[-1])

    under = sorted((s for s in sized if s[3] <= LIMIT), key=lambda s: -s[3])
    for c, m, _, n in under[:5]:
        print("%-48s %5d bytes (%d under)" % ("%s.%s" % (c, m), n, LIMIT - n))
    print("%d methods measured; none over %d bytes but the %d KNOWN debts."
          % (len(sized), LIMIT, len(KNOWN)))


if __name__ == "__main__":
    main()
