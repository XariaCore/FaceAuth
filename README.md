<p align="center">
  <img src="icon.png" width="256" alt="FaceAuth Logo">
</p>

<h1 align="center">FaceAuth</h1>

<p align="center">
  <strong>Biometric Authentication Stack for Linux Systems</strong>
</p>

<p align="center">
  <a href="https://github.com/XariaCore/FaceAuth/releases"><img src="https://img.shields.io/badge/Status-BETA-red.svg?style=flat-square" alt="Status"></a>
  <a href="https://www.gnu.org/licenses/gpl-3.0"><img src="https://img.shields.io/badge/License-GPL--3.0-blue.svg?style=flat-square" alt="License"></a>
  <a href="https://onnxruntime.ai/"><img src="https://img.shields.io/badge/AI--Engine-ONNX--Runtime-white.svg?style=flat-square" alt="Engine"></a>
  <a href="https://opencv.org/"><img src="https://img.shields.io/badge/Vision-OpenCV-white.svg?style=flat-square" alt="Vision"></a>
</p>

---

## Technical Synopsis

FaceAuth is a high-performance biometric security framework that implements deep learning-based facial verification directly into the Linux PAM (Pluggable Authentication Modules) stack. It bridges the gap between hardware-level camera access and system-level authentication, enabling secure, passwordless access for GDM, Sudo, and Terminal sessions.



## Key Capabilities

* **Inference Engine:** Optimized **SCRFD** for face detection and **ArcFace** for feature embedding extraction.
* **Liveness Validation:** Integrated **MiniFASNetV2** for hardware-level anti-spoofing against digital and physical presentation attacks.
* **PAM Integration:** Native shared library (`pam_faceauth.so`) providing seamless hooks into system authentication requests.
* **Non-Persistent Data:** Biometric data is stored as 512-bit normalized mathematical vectors. No raw imagery is ever written to disk.
* **Policy Enforcement:** Fully integrated with **PolicyKit** to ensure administrative operations follow system-wide security policies.

---

## System Architecture

The framework operates through three distinct layers ensuring process isolation and security:

1. **faceauthd (Service Layer):** A C++ daemon managing the continuous camera lifecycle and asynchronous AI inference.
2. **faceauth-cli (Control Layer):** A secure administrative interface for identity enrollment and database management.
3. **pam_faceauth (Bridge Layer):** The library interface connecting system auth calls to the service layer via Unix Domain Sockets.



---

## Deployment

### System Requirements
* **Operating System:** Linux (Debian, Ubuntu, Fedora, or Arch based)
* **Hardware:** USB or Integrated UVC-compliant camera
* **Libraries:** `OpenCV 4.x`, `ONNX Runtime 1.18+`, `libpam-dev`, `Polkit`

### Installation
The automated installer handles cross-compilation and system service configuration:

```bash
git clone https://github.com/XariaCore/FaceAuth.git
cd FaceAuth
chmod +x install.sh
sudo ./install.sh
```

# Administrative Interface

The `faceauth-cli` provides an intuitive management protocol:

| Action               | Command Syntax                              | Access Level      |
|----------------------|----------------------------------------------|-------------------|
| Identity Enrollment  | `sudo faceauth-cli enroll <username>`        | Root / Admin      |
| Revoke Identity      | `sudo faceauth-cli delete <username>`        | Root / Admin      |
| Audit Identity List  | `faceauth-cli list`                          | Authorized User   |
| Integrity Test       | `faceauth-cli verify`                        | Authorized User   |

---

# Hardware Configuration

Operational thresholds can be modified in `include/Config.h` to suit specific environmental requirements:

```cpp
/* Precision Parameters */
const float RECOG_THRESHOLD = 0.70f;  // Biometric matching strictness
const float SPOOF_THRESHOLD = 0.80f;  // Anti-spoofing sensitivity
const int   SCAN_TIMEOUT_MS = 1024;   // Temporal limit per request
```

# Security Declaration

## Fail-Safe Mechanism
In the event of hardware failure or daemon unavailability, the PAM module returns `PAM_AUTHINFO_UNAVAIL`, defaulting to the standard system password prompt.

## Data Isolation
All biometric assets are stored in `/opt/FaceAuth/data`, restricted by `700` permissions to the `root` user.

# License

FaceAuth is open-source software, licensed under the **GPL-3.0**.  
Review the `LICENSE` file for further legal details.

