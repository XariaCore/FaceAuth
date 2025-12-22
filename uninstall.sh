#!/usr/bin/env bash
set -euo pipefail

# ==================================================
# CONFIGURATION
# ==================================================
INSTALL_DIR="/opt/FaceAuth"
SERVICE_NAME="faceauth.service"
POLICY_NAME="org.faceauth.policy"

# ==================================================
# COLORS
# ==================================================
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

log()  { echo -e "${GREEN}[OK]${NC} $*"; }
info() { echo -e "${YELLOW}[INFO]${NC} $*"; }
warn() { echo -e "${YELLOW}[WARN]${NC} $*"; }
err()  { echo -e "${RED}[ERROR]${NC} $*"; exit 1; }

echo -e "${RED}==== FaceAuth Uninstaller (Factory Reset) ====${NC}"

# ==================================================
# ROOT CHECK
# ==================================================
[[ $EUID -eq 0 ]] || err "This uninstaller must be run as root (sudo)."

# ==================================================
# STOP & REMOVE SERVICE
# ==================================================
info "Stopping and removing systemd service..."
systemctl stop "$SERVICE_NAME" 2>/dev/null || true
systemctl disable "$SERVICE_NAME" 2>/dev/null || true
rm -f "/etc/systemd/system/${SERVICE_NAME}"
systemctl daemon-reload

# ==================================================
# REMOVE APPLICATION FILES
# ==================================================
info "Removing application files..."
rm -rf "$INSTALL_DIR"

# Remove global binaries/symlinks
rm -f /usr/local/bin/faceauth-cli
rm -f /usr/local/bin/faceauthd 2>/dev/null || true

# ==================================================
# REMOVE PAM MODULE
# ==================================================
info "Removing PAM module..."
for PAM_PATH in \
  /usr/lib/x86_64-linux-gnu/security \
  /lib/x86_64-linux-gnu/security \
  /lib/security
do
  rm -f "$PAM_PATH/pam_faceauth.so"
done

# ==================================================
# REVERT PAM CONFIGURATION
# ==================================================
info "Reverting PAM configuration..."
for PAM_FILE in /etc/pam.d/sudo /etc/pam.d/common-auth; do
  [[ -f "$PAM_FILE" ]] || continue

  if grep -q pam_faceauth.so "$PAM_FILE"; then
    cp "$PAM_FILE" "${PAM_FILE}.bak_faceauth_uninstall"
    sed -i '/pam_faceauth.so/d' "$PAM_FILE"
    log "Cleaned $PAM_FILE"
  else
    info "No FaceAuth entries found in $PAM_FILE"
  fi
done

# ==================================================
# REMOVE POLKIT POLICY
# ==================================================
info "Removing Polkit policy..."
rm -f "/usr/share/polkit-1/actions/${POLICY_NAME}"

# ==================================================
# CLEAN BUILD ARTIFACTS
# ==================================================
info "Cleaning local build artifacts..."
rm -rf build_release cmake-build-debug 2>/dev/null || true

# ==================================================
# DONE
# ==================================================
echo -e "${GREEN}==== Uninstallation Complete ====${NC}"
echo "System has been restored to its previous state."

