# 📂 New_Extract_GUID

<h3 align="center">Cross-Platform iOS GUID Extractor</h3>

<p align="center">
  A modern, automated, and lightweight utility designed to dynamically extract the system Shared SystemGroup GUID from connected iOS devices. Fully compliant across <b>Windows</b>, <b>macOS</b>, and <b>Linux</b> environments.
</p>

<p align="center">
  <a href="https://github.com/alisakkaf/ios-a12-bypass"><img src="https://img.shields.io/badge/Project-iOS%20A12%20Bypass-blue?style=for-the-badge&logo=github" alt="Project"></a>
  <img src="https://img.shields.io/badge/Platform-Windows%20%7C%20macOS%20%7C%20Linux-orange?style=for-the-badge" alt="Platform">
  <img src="https://img.shields.io/badge/Python-3.7+-green?style=for-the-badge&logo=python" alt="Python">
</p>

---

## ⚡ Overview

This submodule automates the collection and parsing of live iOS syslog archives. By triggering a diagnostic hardware reboot and tracking the device via multiplexer interfaces, it captures the critical boot execution phase to cleanly map and filter references to `BLDatabaseManager.sqlite` handled by `bookassetd`.

---

## 🛠️ Environment Prerequisites

To ensure seamless execution, the following environment dependencies and low-level system libraries (`libs`) must be configured according to your host operating system.

### 1. Cross-Platform Core
* **Python 3.7+** must be configured and exposed inside your environment system variables (`PATH`).

### 2. OS-Specific System Libraries (`libs`)

#### 🔹 Windows Environment
* **iTunes Desktop Client:** Essential for providing the structural `usbmuxd` (Apple Mobile Device Support) connectivity layer. *(It is highly recommended to install the standard standalone installer rather than the Microsoft Store package).*

#### 🔹 macOS Environment
* Native system drivers handle device virtualization natively. Ensure **Xcode Command Line Tools** are provisioned:
  ```bash
  xcode-select --install
    ```bash

🔹 Linux Environment (Ubuntu/Debian-based)
Native communication bindings for backend hardware routing must be installed explicitly:

Bash
sudo apt update
sudo apt install usbmuxd libimobiledevice-utils libimobiledevice6
sudo systemctl restart usbmuxd
🚀 Installation & Setup
Navigate directly to the working component directory and execute the package installation workflow:

Bash
# Navigate to the tool directory
cd New_Extract_GUID

# Install the standardized python dependency pipeline
pip install pymobiledevice3
💻 Technical Usage
Securely connect the iOS device to your host machine using a certified USB cable.

Complete full device pairing authentication by selecting "Trust This Computer" on the screen and providing the passcode.

Initiate the extraction deployment vector:

🏁 Execution Pipelines
Windows Host (Command Prompt / PowerShell):

DOS
python extract_guid.py
macOS / Linux Terminals:

Bash
chmod +x extract_guid.py
./extract_guid.py
🔍 Execution Architecture Workflow
Phase I (Reboot): Dispatches an encrypted remote software signal instructing the system diagnostics agent to perform a hard reboot.

Phase II (Polling): Evaluates connection points dynamically across the local host loop infrastructure waiting for interface restoration.

Phase III (Capture): Aggregates structural streaming log structures directly into an isolated runtime sandbox directory.

Phase IV (Extraction): Applies high-speed regular expression filters to parsing indices, cleanly displaying the absolute hex sequence string mapping back to the system framework.

📄 License
This module is hosted as an integrated core element of the ios-a12-bypass architecture. Distributed under standard project parameters.