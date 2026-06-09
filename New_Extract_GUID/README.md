# 📂 New_Extract_GUID

<h3 align="center">Cross-Platform iOS GUID Extractor</h3>

<p align="center">
  A modern, automated, and lightweight utility designed to dynamically extract the system Shared SystemGroup GUID from connected iOS devices. Fully compliant across <b>Windows</b>, <b>macOS</b>, and <b>Linux</b> environments.
</p>

<p align="center">
  <a href="https://github.com/alisakkaf/ios-a12-bypass"><img src="https://img.shields.io/badge/Project-iOS%20A12%20Bypass-blue?style=for-the-badge&logo=github" alt="Project"></a>
  <img src="https://img.shields.io/badge/Platform-Windows%20%7C%20macOS%20%7C%20Linux-orange?style=for-the-badge" alt="Platform">
  <img src="https://img.shields.io/badge/Python-3.6+-green?style=for-the-badge&logo=python" alt="Python">
</p>

---

## ⚡ Overview

This submodule automates the collection and parsing of live iOS syslog archives. By triggering a diagnostic hardware reboot and tracking the device via multiplexer interfaces, it captures the critical boot execution phase to cleanly map and filter references to `BLDatabaseManager.sqlite` handled by `bookassetd`.

---

## 🛠️ Environment Prerequisites & Dependencies

To ensure seamless execution across all platforms, the following dependencies must be installed correctly.

### 🔹 Windows Environment
1. **Python 3.6+**: Ensure you check the box **"Add Python to PATH"** during installation.
2. **iTunes Desktop Client**: Required for `usbmuxd` drivers (Apple Mobile Device Support). *Use the standard standalone installer, avoiding the Microsoft Store version.*
3. **Python Libraries**:
   ```cmd
   python -m pip install --upgrade pip
   python -m pip install pymobiledevice3
   ```

### 🔹 macOS Environment
macOS uses `python3` by default. You need Homebrew to install the required system libraries.
1. **System Tools**:
   ```bash
   xcode-select --install
   brew install libimobiledevice curl
   ```
2. **Python Libraries**:
   ```bash
   python3 -m pip install --upgrade pip
   python3 -m pip install pymobiledevice3
   ```

### 🔹 Linux Environment (Ubuntu/Debian-based)
Native communication bindings for backend hardware routing must be installed explicitly.
1. **System Tools**:
   ```bash
   sudo apt update
   sudo apt install python3 python3-pip usbmuxd libimobiledevice-utils libimobiledevice6 curl
   sudo systemctl restart usbmuxd
   ```
2. **Python Libraries**:
   ```bash
   python3 -m pip install --upgrade pip
   python3 -m pip install pymobiledevice3
   ```

*(Note: Using `python -m pip` or `python3 -m pip` prevents the `command not found` errors by ensuring the library is linked directly to your active Python environment).*

---

## 🚀 Installation & Usage

1. Securely connect the iOS device to your host machine using a certified USB cable.
2. Complete full device pairing authentication by selecting **"Trust This Computer"** on the device screen and providing the passcode.
3. Navigate to the extraction directory and execute the script:

**Windows Host (Command Prompt / PowerShell):**
```cmd
cd New_Extract_GUID
python extract_guid.py
```

**macOS / Linux Terminals:**
```bash
cd New_Extract_GUID
chmod +x extract_guid.py
python3 extract_guid.py
```

---

## 🔗 Post-Extraction: Using the Main Tool (`a12_bypass`)

Once the script successfully extracts your Shared SystemGroup GUID, you can bypass the interactive prompts and pass it directly into the primary `a12_bypass` architecture for immediate execution.

### 📱 Execution Command

```bash
# Use pre-extracted GUID (Inject the output from the extraction script)
./a12_bypass --guid <YOUR_EXTRACTED_GUID>

# Example:
./a12_bypass --guid 2A22A82B-C342-444D-972F-5270FB5080DF
```

### 🎛️ Relevant Configuration Flags

| Flag | Description |
|---|---|
| `-g, --guid GUID` | Provide the pre-extracted SystemGroup GUID |
| `-o, --output DIR` | Specify payload generation directory *(Optional)* |
| `-v, --verbose` | Enable detailed diagnostic logging *(Optional)* |

---

## 📄 License
This module is hosted as an integrated core element of the [ios-a12-bypass](https://github.com/alisakkaf/ios-a12-bypass) architecture. Distributed under standard project parameters.
