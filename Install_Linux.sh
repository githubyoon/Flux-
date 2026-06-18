#!/bin/sh
set -e

echo "============================================"
echo " Flux Language Installer v1.0"
echo "============================================"
echo

# =========================
# Build
# =========================
echo "Building Flux..."

cxx="${CXX:-g++}"

"$cxx" -std=c++20 -Iinclude \
    src/main.cpp \
    src/Lexer.cpp \
    src/Parser.cpp \
    src/Compiler.cpp \
    src/VM.cpp \
    -o flux

"$cxx" -std=c++20 -Iinclude \
    flux-pkg/main.cpp \
    -o flux-pkg

echo "Build completed."
echo

# =========================
# Install
# =========================
TARGET="$HOME/.local/share/flux"
BIN_DIR="$TARGET/bin"
MODULES_DIR="$TARGET/modules"

echo "Installing to:"
echo "$TARGET"
echo

mkdir -p "$BIN_DIR"
mkdir -p "$MODULES_DIR"

cp ./flux "$BIN_DIR/flux"
cp ./flux-pkg "$BIN_DIR/flux-pkg"

chmod +x "$BIN_DIR/flux"
chmod +x "$BIN_DIR/flux-pkg"

# =========================
# PATH
# =========================
PROFILE="$HOME/.profile"

if [ -f "$HOME/.bashrc" ]; then
    PROFILE="$HOME/.bashrc"
elif [ -f "$HOME/.zshrc" ]; then
    PROFILE="$HOME/.zshrc"
fi

if ! grep -Fq "$BIN_DIR" "$PROFILE" 2>/dev/null; then
    echo >> "$PROFILE"
    echo "# Flux Language" >> "$PROFILE"
    echo "export PATH=\"$BIN_DIR:\$PATH\"" >> "$PROFILE"
    echo "Added Flux to PATH."
else
    echo "Flux is already in PATH."
fi

echo
echo "============================================"
echo " Installation completed!"
echo
echo " Usage:"
echo "   flux script.fx"
echo "   flux-pkg list"
echo "   flux-pkg install math"
echo
echo " Restart your terminal or run:"
echo "   source \"$PROFILE\""
echo "============================================"