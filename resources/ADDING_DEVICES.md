# 📱 Adding Device Support Guide

**Professional iOS A12+ Bypass Toolkit - Device Configuration**

---

## 📋 Overview

This toolkit supports **Apple A12 Bionic and newer chipsets** (iPhone XS/XR and later, iPad Pro 2018+ with A12X/Z). Each device model requires a device-specific `MobileGestalt.plist` configuration file for proper activation bypass functionality.

---

## ✅ Currently Supported Devices

### 🍎 iPhones

| Series | Device Identifiers | Chips Supported |
|--------|-------------------|-----------------|
| **iPhone XS/XR** | iPhone11,2 • iPhone11,4/6 • iPhone11,8 | A12 Bionic |
| **iPhone 11** | iPhone12,1 • iPhone12,3 • iPhone12,5 • iPhone12,8 | A13 Bionic |
| **iPhone 12** | iPhone13,1 • iPhone13,2 • iPhone13,3 • iPhone13,4 | A14 Bionic |
| **iPhone 13** | iPhone14,2 • iPhone14,3 • iPhone14,4 • iPhone14,5 • iPhone14,6 | A15 Bionic |
| **iPhone 14** | iPhone14,7 • iPhone14,8 • iPhone15,2 • iPhone15,3 | A15/A16 Bionic |
| **iPhone 15** | iPhone15,4 • iPhone15,5 • iPhone16,1 • iPhone16,2 | A16/A17 Pro |
| **iPhone 16** | iPhone17,1 • iPhone17,2 • iPhone17,3 • iPhone17,4 | A18/A18 Pro |

### 📱 iPads

| Series | Device Identifiers | Chips Supported |
|--------|-------------------|-----------------|
| **iPad Air** | iPad11,x • iPad13,x • iPad14,x (Air 3,4,5 + M1/M2 variants) | A12/A14/M1/M2 |
| **iPad Pro** | iPad8,x • iPad13,x • iPad14,x (11"/12.9" from 2018+) | A12X/Z • M1/M2 |
| **iPad** | iPad11,x • iPad12,x • iPad14,x (7th-10th gen) | A12/A13/A14 |
| **iPad mini** | iPad11,1/2 (mini 5th gen) | A12 Bionic |

---

## 🔧 Adding a New Device

### Step 1️⃣: Identify Device Model

**Option A - Using This Toolkit:**
```bash
./a12_bypass --info
```

**Option B - Using libimobiledevice:**
```bash
ideviceinfo -k ProductType
# Output example: iPhone14,5
```

**Device Identifier Format:** `iPhoneXX,Y` or `iPadXX,Y`

---

### Step 2️⃣: Extract MobileGestalt Configuration

The required file is located at:
```
/private/var/containers/Shared/SystemGroup/
systemgroup.com.apple.mobilegestaltcache/Library/Caches/
com.apple.MobileGestalt.plist
```

#### 🔓 Extraction Methods:

**A. Jailbroken Device** (Recommended)
```bash
# Connect via SSH/USB tunnel
scp root@device-ip:/private/var/containers/Shared/SystemGroup/\
systemgroup.com.apple.mobilegestaltcache/Library/Caches/\
com.apple.MobileGestalt.plist ./
```

**B. Checkm8 Exploit Chain**
- Use **checkra1n** or **palera1n** for temporary jailbreak
- Mount filesystem via AFC or SSH
- Extract file directly

**C. Backup Extraction** (Limited)
- Create unencrypted iTunes/Finder backup
- Use specialized backup extraction tools
- Locate MobileGestalt.plist in backup archive

> **⚠️ Important**: The plist file is iOS version-specific. Extract from the exact iOS version you intend to bypass.

---

### Step 3️⃣: Create Device Resource Directory

```bash
# Navigate to project resources
cd resources/plists/

# Create directory (convert comma to dash in identifier)
# Example: iPhone14,5 → iPhone14-5
mkdir -p iPhone14-5

# Copy the extracted plist
cp /path/to/com.apple.MobileGestalt.plist iPhone14-5/

# Verify structure
ls -l iPhone14-5/
# Should show: com.apple.MobileGestalt.plist
```

**Directory Naming Convention:**
- Replace `,` with `-` in device identifier
- Examples: `iPhone15,2` → `iPhone15-2` | `iPad13,1` → `iPad13-1`

---

### Step 4️⃣: Verify Integration

```bash
# Rebuild project
cd build
make

# Verify device appears in supported list
./a12_bypass --list
```

Your newly added device should now appear in the output.

---

## 📂 Resource File Structure

```
resources/plists/
├── iPhone14-5/
│   └── com.apple.MobileGestalt.plist    ✅ Required
├── iPhone15-2/
│   └── com.apple.MobileGestalt.plist    ✅ Required
├── iPad13-1/
│   └── com.apple.MobileGestalt.plist    ✅ Required
└── ...
```

**Notes:**
- Only `com.apple.MobileGestalt.plist` is required per device
- EPUB payloads are generated dynamically at runtime
- Pre-generated EPUBs are not necessary

---

## 📊 Complete Device Reference Table

| Identifier | Marketing Name | SoC | iOS Support |
|------------|----------------|-----|-------------|
| **iPhone11,2** | iPhone XS | A12 Bionic | iOS 12-17+ |
| **iPhone11,4/6** | iPhone XS Max | A12 Bionic | iOS 12-17+ |
| **iPhone11,8** | iPhone XR | A12 Bionic | iOS 12-17+ |
| **iPhone12,1** | iPhone 11 | A13 Bionic | iOS 13-17+ |
| **iPhone12,3** | iPhone 11 Pro | A13 Bionic | iOS 13-17+ |
| **iPhone12,5** | iPhone 11 Pro Max | A13 Bionic | iOS 13-17+ |
| **iPhone12,8** | iPhone SE (2nd gen) | A13 Bionic | iOS 13-17+ |
| **iPhone13,1** | iPhone 12 mini | A14 Bionic | iOS 14-17+ |
| **iPhone13,2** | iPhone 12 | A14 Bionic | iOS 14-17+ |
| **iPhone13,3** | iPhone 12 Pro | A14 Bionic | iOS 14-17+ |
| **iPhone13,4** | iPhone 12 Pro Max | A14 Bionic | iOS 14-17+ |
| **iPhone14,2** | iPhone 13 Pro | A15 Bionic | iOS 15-17+ |
| **iPhone14,3** | iPhone 13 Pro Max | A15 Bionic | iOS 15-17+ |
| **iPhone14,4** | iPhone 13 mini | A15 Bionic | iOS 15-17+ |
| **iPhone14,5** | iPhone 13 | A15 Bionic | iOS 15-17+ |
| **iPhone14,6** | iPhone SE (3rd gen) | A15 Bionic | iOS 15-17+ |
| **iPhone14,7** | iPhone 14 | A15 Bionic | iOS 16-17+ |
| **iPhone14,8** | iPhone 14 Plus | A15 Bionic | iOS 16-17+ |
| **iPhone15,2** | iPhone 14 Pro | A16 Bionic | iOS 16-17+ |
| **iPhone15,3** | iPhone 14 Pro Max | A16 Bionic | iOS 16-17+ |
| **iPhone15,4** | iPhone 15 | A16 Bionic | iOS 17+ |
| **iPhone15,5** | iPhone 15 Plus | A16 Bionic | iOS 17+ |
| **iPhone16,1** | iPhone 15 Pro | A17 Pro | iOS 17+ |
| **iPhone16,2** | iPhone 15 Pro Max | A17 Pro | iOS 17+ |
| **iPhone17,1** | iPhone 16 Pro | A18 Pro | iOS 18+ |
| **iPhone17,2** | iPhone 16 Pro Max | A18 Pro | iOS 18+ |
| **iPhone17,3** | iPhone 16 | A18 | iOS 18+ |
| **iPhone17,4** | iPhone 16 Plus | A18 | iOS 18+ |

---

## 💡 Best Practices

### ✅ DO:
- Extract plist from exact iOS version you're targeting
- Verify file integrity after extraction
- Test on same device model before general deployment
- Document iOS version compatibility

### ❌ DON'T:
- Use plist from different iOS major versions
- Mix plists from different device identifiers
- Modify plist contents manually (use as-is)
- Share device-specific UDIDs or serial numbers

---

## 🐛 Troubleshooting

### Device Not Listed After Adding
```bash
# Verify directory structure
ls -R resources/plists/

# Check plist file exists and is readable
file resources/plists/iPhone14-5/com.apple.MobileGestalt.plist

# Rebuild completely
rm -rf build && mkdir build && cd build && cmake .. && make
```

### "Device Not Supported" Error
- Ensure directory name matches normalized product type (comma → dash)
- Verify plist filename is exactly `com.apple.MobileGestalt.plist`
- Check file permissions (should be readable)

---

## 📞 Contributing Device Support

If you've successfully extracted MobileGestalt.plist for a device not yet in the toolkit:

1. **Test Thoroughly**: Verify bypass works on your device
2. **Document Version**: Note exact iOS version tested
3. **Create Pull Request**: Submit plist file with documentation
4. **Attribution**: Include device model and iOS version in PR description

---

## ⚠️ Legal & Ethical Notice

- Only extract and use plists from devices **you own**
- Device configurations may contain identifying information
- Respect user privacy and data protection laws
- Use for **legitimate device recovery only**

---

**Questions?** Open an issue on GitHub with tag `device-support`

