#!/bin/sh

set -eu

IMAGE="fbprinter-builder"

# ------------------------------------------------------------
# Find container runtime
# ------------------------------------------------------------

if command -v podman >/dev/null 2>&1; then
    CONTAINER_RUNTIME="podman"
elif command -v docker >/dev/null 2>&1; then
    CONTAINER_RUNTIME="docker"
else
    echo "Error: neither Podman nor Docker was found."
    echo
    echo "Please install Podman or Docker first."
    exit 1
fi

# ------------------------------------------------------------
# Check ARCH
# ------------------------------------------------------------

if [ -z "${ARCH:-}" ]; then
    echo "Error: ARCH is not set."
    echo
    echo "Set ARCH before building:"
    echo "  export ARCH=arm64"
    echo "  export ARCH=arm"
    exit 1
fi

case "$ARCH" in
    arm64|arm)
        ;;
    *)
        echo "Error: unsupported ARCH='$ARCH'"
        echo
        echo "Supported architectures:"
        echo "  arm64"
        echo "  arm"
        exit 1
        ;;
esac

# ------------------------------------------------------------
# Select build target
# ------------------------------------------------------------

case "${1:-dynamic}" in
    dynamic)
        TARGET="dynamic"
        ;;

    static)
        TARGET="static"
        ;;

    library-only)
        TARGET="library-only"
        ;;

    clean)
        echo "Cleaning build directory..."
        rm -rf build
        exit 0
        ;;

    *)
        echo "Usage: $0 [dynamic|static|library-only|clean]"
        exit 1
        ;;
esac

# ------------------------------------------------------------
# Build
# ------------------------------------------------------------

echo "========================================"
echo " fbprinter build"
echo "========================================"
echo "Container    : $CONTAINER_RUNTIME"
echo "Architecture : $ARCH"
echo "Target       : $TARGET"
echo "Image        : $IMAGE"
echo "========================================"

"$CONTAINER_RUNTIME" run --rm \
    -v "$PWD:/src:Z" \
    -w /src \
    "$IMAGE" \
    make ARCH="$ARCH" "$TARGET"

echo
echo "Build complete."
echo

case "$TARGET" in
    dynamic)
        echo "Output:"
        echo "  build/dynamic/fbprinter"
        echo "  build/dynamic/libfbprinter.so"
        ;;

    static)
        echo "Output:"
        echo "  build/static/fbprinter"
        ;;

    library-only)
        echo "Output:"
        echo "  build/library/libfbprinter.so"
        ;;
esac
