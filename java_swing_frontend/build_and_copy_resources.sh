#!/bin/bash
# Compile Java sources into bin/ and copy resources/ into bin/resources
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="$ROOT_DIR/src"
BIN_DIR="$ROOT_DIR/bin"
RES_DIR="$ROOT_DIR/resources"

mkdir -p "$BIN_DIR"

# Compile
find "$SRC_DIR" -name "*.java" > /tmp/java_files.txt
javac -d "$BIN_DIR" -sourcepath "$SRC_DIR" @/tmp/java_files.txt
rm /tmp/java_files.txt

# Copy resources into bin so they are available on the classpath as /resources/...
if [ -d "$RES_DIR" ]; then
  rm -rf "$BIN_DIR/resources"
  mkdir -p "$BIN_DIR/resources"
  cp -R "$RES_DIR/"* "$BIN_DIR/resources/"
  echo "Copied resources to $BIN_DIR/resources"
else
  echo "No resources directory found at $RES_DIR"
fi

echo "Build finished. Run with: java -cp $BIN_DIR ui.ChessApp"

