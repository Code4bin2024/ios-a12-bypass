# 🎯 Quick Start Guide

## Prerequisites

Ensure you have the following installed:
- **macOS** (10.15+)
- **Homebrew** package manager
- **Xcode Command Line Tools**

## Installation

### Step 1: Install Dependencies

```bash
brew install cmake pkg-config libimobiledevice libplist libzip sqlite3
```

### Step 2: Clone Repository

```bash
git clone https://github.com/alisakkaf/ios-a12-bypass.git
cd ios-a12-bypass
```

### Step 3: Build

```bash
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.ncpu)
```

### Step 4: Run

```bash
# Connect your A12+ device via USB, then:
./a12_bypass --auto
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

# Reinstall if needed
brew reinstall libimobiledevice libplist
```

## Next Steps

- Read the full [README.md](README.md) for detailed documentation
- Check [CONTRIBUTING.md](CONTRIBUTING.md) if you want to contribute
- See [CHANGELOG.md](CHANGELOG.md) for version history

---

**Need help?** Open an issue on GitHub!
