#!/usr/bin/env bash
set -e

GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}=======================================${NC}"
echo -e "${BLUE}        OmaDo Uninstaller              ${NC}"
echo -e "${BLUE}=======================================${NC}"

if [ "$EUID" -eq 0 ]; then
    rm -f /usr/local/bin/omado
    rm -f /usr/local/share/icons/hicolor/scalable/apps/omado.svg
    rm -f /usr/local/share/applications/omado.desktop
    rm -f /usr/local/share/applications/omado-daemon.desktop
else
    rm -f "$HOME/.local/bin/omado"
    rm -f "$HOME/.local/share/icons/hicolor/scalable/apps/omado.svg"
    rm -f "$HOME/.local/share/applications/omado.desktop"
    rm -f "$HOME/.local/share/applications/omado-daemon.desktop"
    rm -f "$HOME/.config/systemd/user/omado.service"
fi

echo -e "\n${GREEN}✓ OmaDo se desinstaló correctamente.${NC}"
