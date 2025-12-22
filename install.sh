#!/bin/bash

# --- CONFIGURATION ---
APP_NAME="faceauth"
INSTALL_DIR="/opt/FaceAuth"
SERVICE_NAME="faceauth.service"
POLICY_NAME="org.faceauth.policy"
ONNX_VERSION="1.18.0"

# Colors & Formatting
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${GREEN}============================================================${NC}"
echo -e "${GREEN}   FaceAuth Enterprise Installer (System-Wide v4.5)         ${NC}"
echo -e "${GREEN}============================================================${NC}"

# 1. Root Privilege Check
if [ "$EUID" -ne 0 ]; then
  echo -e "${RED}[ERROR] Access denied. Please run as root (sudo).${NC}"
  exit 1
fi

# 2. Stop Existing Service
echo -e "${YELLOW}[INFO] Stopping active services...${NC}"
systemctl stop ${SERVICE_NAME} 2>/dev/null

# 3. Check Dependencies
echo -e "${YELLOW}[INFO] Verifying system dependencies...${NC}"
apt-get update -qq
apt-get install -y -qq build-essential cmake git libopencv-dev libssl-dev wget tar libpam0g-dev

# 4. Install ONNX Runtime (if missing)
if ! ldconfig -p | grep -q libonnxruntime; then
    echo -e "${YELLOW}[INFO] Installing ONNX Runtime (v${ONNX_VERSION})...${NC}"
    cd /tmp
    wget -q --show-progress "https://github.com/microsoft/onnxruntime/releases/download/v${ONNX_VERSION}/onnxruntime-linux-x64-${ONNX_VERSION}.tgz"
    tar -xf "onnxruntime-linux-x64-${ONNX_VERSION}.tgz"
    
    # Install Libraries
    mkdir -p /usr/local/include/onnxruntime
    cp -r onnxruntime-linux-x64-${ONNX_VERSION}/include/* /usr/local/include/onnxruntime/
    cp -P onnxruntime-linux-x64-${ONNX_VERSION}/lib/libonnxruntime.so* /usr/local/lib/
    ldconfig
    
    # Cleanup
    rm -rf onnxruntime-linux-x64-${ONNX_VERSION}*
    echo -e "${GREEN}[SUCCESS] ONNX Runtime installed successfully.${NC}"
fi

# 5. Build Project
echo -e "${YELLOW}[INFO] Building project (Release Mode)...${NC}"
rm -rf build_release
mkdir build_release
cd build_release

if [ ! -f "../CMakeLists.txt" ]; then
    echo -e "${RED}[ERROR] CMakeLists.txt not found! Are you in the project root?${NC}"
    exit 1
fi

cmake -DCMAKE_BUILD_TYPE=Release .. && make -j$(nproc)
if [ $? -ne 0 ]; then 
    echo -e "${RED}[ERROR] Compilation failed. Please check the source code.${NC}"
    exit 1
fi
cd ..

# 6. Deploy Files (DÜZELTİLEN KISIM)
echo -e "${YELLOW}[INFO] Deploying application files...${NC}"

# Create directories (preserves existing data)
mkdir -p ${INSTALL_DIR}/bin ${INSTALL_DIR}/models ${INSTALL_DIR}/data

# Copy Binaries Check
if [ -f "build_release/faceauthd" ]; then
    cp build_release/faceauthd ${INSTALL_DIR}/bin/
else
    echo -e "${RED}[ERROR] faceauthd binary missing! Build failed.${NC}"
    exit 1
fi

if [ -f "build_release/faceauth-cli" ]; then
    cp build_release/faceauth-cli ${INSTALL_DIR}/bin/
else
    echo -e "${RED}[ERROR] faceauth-cli binary missing! Build failed.${NC}"
    exit 1
fi

# Copy Models (Ignore error if no models found, just warn)
cp models/*.onnx ${INSTALL_DIR}/models/ 2>/dev/null || echo -e "${YELLOW}[WARN] No ONNX models found to copy.${NC}"

# Expose CLI to Global Path via Symlink (Fixes 'Not a directory' error)
echo -e "${YELLOW}[INFO] Configuring global access...${NC}"
if [ ! -d "/usr/local/bin" ]; then
    mkdir -p /usr/local/bin
fi

# Remove old file/link and create new symlink
rm -f /usr/local/bin/faceauth-cli
ln -sf ${INSTALL_DIR}/bin/faceauth-cli /usr/local/bin/faceauth-cli

# Secure Installation Directory
chown -R root:root ${INSTALL_DIR}

# 2. Genel klasörlere ve binary dosyalara 'okuma ve çalıştırma' izni ver (TAB'da görünmesini sağlar)
chmod -R 755 ${INSTALL_DIR}

# 3. Hassas verileri (Modeller ve Veritabanı) sadece root'a özel tut (Kritik nokta burası)
chmod -R 700 ${INSTALL_DIR}/data
chmod -R 700 ${INSTALL_DIR}/models

# 7. Install PAM Module
echo -e "${YELLOW}[INFO] Installing Authentication Module (PAM)...${NC}"
PAM_DIR="/lib/x86_64-linux-gnu/security"
[ ! -d "$PAM_DIR" ] && PAM_DIR="/lib/security"

if [ -f "build_release/pam_faceauth.so" ]; then
    cp build_release/pam_faceauth.so $PAM_DIR/
    chmod 644 $PAM_DIR/pam_faceauth.so
else
    echo -e "${RED}[ERROR] pam_faceauth.so was not built!${NC}"; exit 1;
fi

# 8. Configure System Authentication (Login + Sudo)
echo -e "${YELLOW}[INFO] Configuring system-wide authentication policy...${NC}"

# Clean up legacy 'sudo' specific config if exists
if grep -q "pam_faceauth.so" /etc/pam.d/sudo; then
    sed -i '/pam_faceauth.so/d' /etc/pam.d/sudo
    echo -e "   - Cleaned up legacy sudo configuration."
fi

# Inject into 'common-auth' (Affects Login, Sudo, Lock Screen)
COMMON_AUTH="/etc/pam.d/common-auth"
if ! grep -q "pam_faceauth.so" $COMMON_AUTH; then
    # Prepend to the top of the file
    sed -i '1s/^/auth sufficient pam_faceauth.so\n/' $COMMON_AUTH
    echo -e "${GREEN}[SUCCESS] Biometric authentication enabled for Login & Sudo.${NC}"
else
    echo -e "   - Configuration already active."
fi

# 9. Configure Polkit & Systemd
echo -e "${YELLOW}[INFO] Configuring services and permissions...${NC}"

# Polkit Policy
cat <<EOF > /usr/share/polkit-1/actions/${POLICY_NAME}
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE policyconfig PUBLIC "-//freedesktop//DTD PolicyKit Policy Configuration 1.0//EN" "http://www.freedesktop.org/standards/PolicyKit/1/policyconfig.dtd">
<policyconfig>
  <vendor>FaceAuth Security</vendor>
  <action id="org.faceauth.enroll">
    <description>Enroll Biometric Data</description>
    <defaults><allow_any>auth_admin</allow_any><allow_inactive>auth_admin</allow_inactive><allow_active>auth_admin</allow_active></defaults>
  </action>
  <action id="org.faceauth.delete">
    <description>Delete Biometric Data</description>
    <defaults><allow_any>auth_admin</allow_any><allow_inactive>auth_admin</allow_inactive><allow_active>auth_admin</allow_active></defaults>
  </action>
</policyconfig>
EOF

# Systemd Service Unit
cat <<EOF > /etc/systemd/system/${SERVICE_NAME}
[Unit]
Description=FaceAuth Biometric Daemon
After=network.target

[Service]
Type=simple
User=root
ExecStart=${INSTALL_DIR}/bin/faceauthd
Restart=on-failure
RestartSec=5
# Allow IPC via /tmp for PAM communication
PrivateTmp=no

[Install]
WantedBy=multi-user.target
EOF

# 10. Start Service
systemctl daemon-reload
systemctl enable ${SERVICE_NAME}
systemctl restart ${SERVICE_NAME}

echo -e "${GREEN}============================================================${NC}"
echo -e "${GREEN}   INSTALLATION COMPLETE                                    ${NC}"
echo -e "${GREEN}============================================================${NC}"
echo -e "Usage:"
echo -e "  Enroll Face : sudo faceauth-cli enroll <username>"
echo -e "  Verify      : Try 'sudo -k && sudo ls' or Logout/Login"
