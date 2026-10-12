set -e

BINARY_DIR=$(grep 'binary_dir' ../../../config.yaml | sed -E 's/.*binary_dir:[[:space:]]*"([^"]+)".*/\1/')

"$BINARY_DIR/build_master/master"
