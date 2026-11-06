#!/usr/bin/env bash
set -e

VERSION="2.0.1"

GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}=======================================${NC}"
echo -e "${BLUE}        OmaDo Uninstaller v${VERSION}         ${NC}"
echo -e "${BLUE}=======================================${NC}"

if command -v systemctl &> /dev/null; then
    systemctl --user disable --now omado.service 2>/dev/null || true
fi

if [ "$EUID" -eq 0 ]; then
    rm -f /usr/local/bin/omado
    rm -f /usr/local/share/icons/hicolor/scalable/apps/omado.svg
    rm -f /usr/local/share/applications/omado.desktop
    rm -f /usr/local/share/applications/omado-daemon.desktop
    rm -f /usr/local/share/dbus-1/interfaces/io.omarchy.OmaDo.xml
    rm -f /usr/local/share/dbus-1/services/io.omarchy.OmaDo.service
    rm -f /etc/systemd/user/omado.service
    rm -f /etc/systemd/user-preset/50-omado.preset
    rm -f /etc/xdg/autostart/omado-daemon.desktop
else
    rm -f "$HOME/.local/bin/omado"
    rm -f "$HOME/.local/share/icons/hicolor/scalable/apps/omado.svg"
    rm -f "$HOME/.local/share/applications/omado.desktop"
    rm -f "$HOME/.local/share/applications/omado-daemon.desktop"
    rm -f "$HOME/.local/share/dbus-1/interfaces/io.omarchy.OmaDo.xml"
    rm -f "$HOME/.local/share/dbus-1/services/io.omarchy.OmaDo.service"
    rm -f "$HOME/.config/systemd/user/omado.service"
    rm -f "$HOME/.config/autostart/omado-daemon.desktop"
fi

if command -v systemctl &> /dev/null; then
    systemctl --user daemon-reload 2>/dev/null || true
fi

echo -e "\n${GREEN}✓ OmaDo v${VERSION} se desinstaló correctamente.${NC}"
