# 🎯 Quick Start Guide
## Prerequisites
Ensure you have one of the following environments ready:
- **macOS** (10.15+) with **Homebrew** and **Xcode Command Line Tools**
- **Linux** (Debian/Kali/Ubuntu family) with `apt`

## Installation

### Step 1: Install Dependencies
#### macOS (Homebrew)

```bash
brew install cmake pkg-config libimobiledevice libplist libzip sqlite3
```
#### Linux (APT)
```bash
sudo apt update
sudo apt install -y cmake pkg-config ninja-build binutils \
  libimobiledevice-dev libplist-dev libusbmuxd-dev libzip-dev libsqlite3-dev
```

### Step 2: Clone Repository

```bash
git clone https://github.com/alisakkaf/ios-a12-bypass.git
cd ios-a12-bypass
```

### Step 3: Build
#### Preferred (Ninja)

```bash
cmake -S . -B build -G Ninja
cmake --build build -j "$(nproc)"
```
If `pkg-config` cannot detect `libzip` on Linux, configure with:
```bash
PKG_CONFIG_PATH=/usr/lib/x86_64-linux-gnu/pkgconfig:/usr/lib/pkgconfig:/usr/share/pkgconfig \
cmake -S . -B build -G Ninja
cmake --build build -j "$(nproc)"
```
#### Alternative (Makefiles)
```bash
mkdir -p build && cd build
cmake ..
make -j"$(nproc)"
```

### Step 4: Run

```bash
# Connect your A12+ device via USB, then:
./a12_bypass --auto
```
### Step 5: Verify Build
```bash
ctest --test-dir build --output-on-failure
./build/a12_bypass --version
./build/a12_bypass --help
./build/a12_bypass --list
```
### Step 6: Clean Build Artifacts (Optional)
```bash
rm -rf build
```

## Common Commands

```bash
# Show help
./a12_bypass --help

# Show version
./a12_bypass --version

# List supported devices
./a12_bypass --list

# Check connected device
./a12_bypass --info

# Dry run (no changes)
./a12_bypass --dry-run --verbose

# Use preset GUID
./a12_bypass --guid YOUR-GUID-HERE
```

## Troubleshooting

### Device not detected?
```bash
# Check if libimobiledevice can see your device
idevice_id -l

# If empty, reconnect device and trust computer
```

### Build errors?
```bash
# Verify dependencies
pkg-config --cflags libimobiledevice-1.0
# Reinstall if needed (macOS)
brew reinstall libimobiledevice libplist

# Reinstall if needed (Linux)
sudo apt install --reinstall libimobiledevice-dev libplist-dev libusbmuxd-dev libzip-dev
```

## Next Steps

- Read the full [README.md](README.md) for detailed documentation
- Check [CONTRIBUTING.md](CONTRIBUTING.md) if you want to contribute
- See [CHANGELOG.md](CHANGELOG.md) for version history

---

**Need help?** Open an issue on GitHub!
