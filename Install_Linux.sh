#!/bin/sh

echo "============================================"
echo " Flux Language Installer v1.0"
echo "============================================"
echo

TARGET="$HOME/.local/share/flux"
BIN_DIR="$TARGET/bin"
MODULES_DIR="$TARGET/modules"

echo "Installing to: $TARGET"
echo

# 디렉터리 생성
mkdir -p "$BIN_DIR"
mkdir -p "$MODULES_DIR"

# 실행 파일 복사
echo "Copying flux..."
cp ./flux "$BIN_DIR/flux"

echo "Copying flux-pkg..."
cp ./flux-pkg "$BIN_DIR/flux-pkg"

# 실행 권한 부여
chmod +x "$BIN_DIR/flux"
chmod +x "$BIN_DIR/flux-pkg"

# PATH 추가
SHELL_RC="$HOME/.profile"

if [ -n "$BASH_VERSION" ]; then
    SHELL_RC="$HOME/.bashrc"
elif [ -n "$ZSH_VERSION" ]; then
    SHELL_RC="$HOME/.zshrc"
fi

PATH_LINE='export PATH="$HOME/.local/share/flux/bin:$PATH"'

if ! grep -Fq "$HOME/.local/share/flux/bin" "$SHELL_RC" 2>/dev/null; then
    echo
    echo "Adding Flux to PATH..."
    echo "$PATH_LINE" >> "$SHELL_RC"
    echo "Added to $SHELL_RC"
else
    echo
    echo "Flux is already in PATH."
fi

echo
echo "============================================"
echo " Usage:"
echo "   flux script.fx        - Run a Flux script"
echo "   flux-pkg list         - List installed modules"
echo "   flux-pkg install math - Install math module"
echo "============================================"
echo
echo "Restart your terminal or run:"
echo "source $SHELL_RC"