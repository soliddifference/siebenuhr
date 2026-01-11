#!/bin/bash
#
# Build merged firmware binaries for both clock variants and copy to destination.
# Usage: ./build_firmware.sh <destination_directory> [version]
#
# Example: ./build_firmware.sh ~/src/support-soliddifference/content/downloads 1.1.0

set -e

DEST_DIR="${1:?Usage: $0 <destination_directory> [version]}"
VERSION="${2:-dev}"

if [ ! -d "$DEST_DIR" ]; then
    echo "Error: Destination directory does not exist: $DEST_DIR"
    exit 1
fi

echo "Building firmware (version: $VERSION)..."
echo ""

# Build both environments
pio run -e esp32-mini -e esp32-regular

echo ""
echo "Copying merged binaries to $DEST_DIR..."

cp .pio/build/esp32-mini/firmware-esp32-mini-merged.bin \
   "$DEST_DIR/firmware-mini-${VERSION}.bin"

cp .pio/build/esp32-regular/firmware-esp32-regular-merged.bin \
   "$DEST_DIR/firmware-regular-${VERSION}.bin"

echo ""
echo "Done:"
ls -lh "$DEST_DIR"/firmware-*-${VERSION}.bin
