#!/usr/bin/env bash
set -e

# Argus Token Router - One-Line Installer Script
# Clones/updates repository, builds executable using CMake & Qt6, and installs binary as 'argus-router'.

REPO_URL="https://github.com/dotdok132/argus-router.git"
INSTALL_DIR="/usr/local/bin"
ALT_INSTALL_DIR="$HOME/.local/bin"
BINARY_NAME="argus-router"

echo "--------------------------------------------------------"
echo "        ARGUS TOKEN ROUTER - INSTALLATION"
echo "--------------------------------------------------------"

# 1. Dependency Checks
echo "[1/4] Checking system dependencies..."

missing_deps=()
for cmd in git cmake make g++ pkg-config; do
    if ! command -v "$cmd" &>/dev/null; then
        missing_deps+=("$cmd")
    fi
done

if [ ${#missing_deps[@]} -ne 0 ]; then
    echo "[-] Error: Missing required build dependencies: ${missing_deps[*]}"
    echo "[*] On Ubuntu/Debian, install with:"
    echo "    sudo apt update && sudo apt install -y build-essential cmake git qt6-base-dev libqt6network6"
    exit 1
fi

# 2. Build Workspace Setup
BUILD_DIR="$(mktemp -d -t argus-build-XXXXXX)"
cleanup() {
    rm -rf "$BUILD_DIR"
}
trap cleanup EXIT

echo "[2/4] Fetching latest source code from GitHub..."
git clone --depth 1 "$REPO_URL" "$BUILD_DIR/source"
cd "$BUILD_DIR/source"

# 3. Compilation
echo "[3/4] Compiling Argus Token Router (C++20 / Qt6)..."
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

if [ ! -f "build/qt-token-router" ]; then
    echo "[-] Error: Compilation failed or binary 'build/qt-token-router' not found!"
    exit 1
fi

# 4. Installation into Binaries
echo "[4/4] Installing executable binary as '$BINARY_NAME'..."

TARGET_BIN=""
if [ -w "$INSTALL_DIR" ]; then
    TARGET_BIN="$INSTALL_DIR/$BINARY_NAME"
    cp build/qt-token-router "$TARGET_BIN"
    chmod +x "$TARGET_BIN"
elif sudo -n true 2>/dev/null; then
    TARGET_BIN="$INSTALL_DIR/$BINARY_NAME"
    sudo cp build/qt-token-router "$TARGET_BIN"
    sudo chmod +x "$TARGET_BIN"
else
    mkdir -p "$ALT_INSTALL_DIR"
    TARGET_BIN="$ALT_INSTALL_DIR/$BINARY_NAME"
    cp build/qt-token-router "$TARGET_BIN"
    chmod +x "$TARGET_BIN"
    
    if [[ ":$PATH:" != *":$ALT_INSTALL_DIR:"* ]]; then
        echo "[*] Note: Add '$ALT_INSTALL_DIR' to your PATH in ~/.bashrc:"
        echo "    export PATH=\"\$HOME/.local/bin:\$PATH\""
    fi
fi

echo "--------------------------------------------------------"
echo "[+] SUCCESS: Argus Token Router successfully installed!"
echo "[+] Binary location: $TARGET_BIN"
echo "[+] Run executable:  $BINARY_NAME"
echo "--------------------------------------------------------"
