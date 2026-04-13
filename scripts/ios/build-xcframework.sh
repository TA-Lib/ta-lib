#!/bin/bash
set -e

ROOT_DIR=$(pwd)
FRAMEWORK_NAME="ta_lib"
HEADER_DIR="${ROOT_DIR}/include"
XCFRAMEWORK_OUTPUT="${ROOT_DIR}/${FRAMEWORK_NAME}.xcframework"
TMP_HEADERS_DIR="${ROOT_DIR}/_headers"
UMBRELLA_HEADER="${TMP_HEADERS_DIR}/${FRAMEWORK_NAME}.h"

# Clean old output
rm -rf "$XCFRAMEWORK_OUTPUT" "$TMP_HEADERS_DIR"
mkdir -p "$TMP_HEADERS_DIR"

# Copy all headers to temp directory
cp "$HEADER_DIR"/*.h "$TMP_HEADERS_DIR"

# Generate umbrella header file
echo "// Umbrella header for $FRAMEWORK_NAME" > "$UMBRELLA_HEADER"
for hdr in "$TMP_HEADERS_DIR"/*.h; do
  [ "$(basename "$hdr")" = "$(basename "$UMBRELLA_HEADER")" ] && continue
  echo "#include \"$(basename "$hdr")\"" >> "$UMBRELLA_HEADER"
done

echo "✅ Generated umbrella header: $UMBRELLA_HEADER"

# Verify all slices are built
for LIB in build-ios/libta-lib.a build-ios-sim/libta-lib.a build-macos/libta-lib.a; do
  if [ ! -f "$ROOT_DIR/$LIB" ]; then
    echo "❌ Missing $LIB — run scripts/build-ios.sh first"
    exit 1
  fi
done

# Generate module.modulemap
cat > "${TMP_HEADERS_DIR}/module.modulemap" <<EOF
module ${FRAMEWORK_NAME} [system] {
    header "${FRAMEWORK_NAME}.h"
    export *
}
EOF

echo "✅ Generated module.modulemap"


# Create the XCFramework from static libs
xcodebuild -create-xcframework \
  -library "$ROOT_DIR/build-ios/libta-lib.a" -headers "$TMP_HEADERS_DIR" \
  -library "$ROOT_DIR/build-ios-sim/libta-lib.a" -headers "$TMP_HEADERS_DIR" \
  -library "$ROOT_DIR/build-macos/libta-lib.a" -headers "$TMP_HEADERS_DIR" \
  -output "$XCFRAMEWORK_OUTPUT"

echo "✅ Created $FRAMEWORK_NAME.xcframework"

# Optional cleanup
rm -rf "$TMP_HEADERS_DIR"

