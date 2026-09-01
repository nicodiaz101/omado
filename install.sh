#!/usr/bin/env bash
set -e

GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${BLUE}=======================================${NC}"
echo -e "${BLUE}         OmaDo Easy Installer          ${NC}"
echo -e "${BLUE}=======================================${NC}"

# Check for required tools
MISSING_DEPS=()
for dep in cmake g++ git; do
    if ! command -v $dep &> /dev/null; then
        MISSING_DEPS+=($dep)
    fi
done

if [ ${#MISSING_DEPS[@]} -ne 0 ]; then
    echo -e "${YELLOW}Faltan dependencias para compilar: ${MISSING_DEPS[*]}${NC}"
    if command -v pacman &> /dev/null; then
        echo -e "${BLUE}Instalando dependencias con pacman...${NC}"
        sudo pacman -S --needed --noconfirm cmake gcc git qt6-base qt6-declarative qt6-quickcontrols2 qtkeychain-qt6
    else
        echo -e "${RED}Por favor instalá las dependencias necesarias antes de continuar.${NC}"
        exit 1
    fi
fi

# Build
echo -e "\n${GREEN}==>${NC} Compilando OmaDo..."
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)

# Install
echo -e "\n${GREEN}==>${NC} Instalando en el sistema..."
if [ "$EUID" -eq 0 ]; then
    cmake --install . --prefix /usr/local
else
    cmake --install . --prefix "$HOME/.local"
    # Ensure systemd user service directory exists
    mkdir -p "$HOME/.config/systemd/user"
    cp ../systemd/omado.service "$HOME/.config/systemd/user/" 2>/dev/null || true
fi

echo -e "\n${GREEN}✓ ¡OmaDo se instaló correctamente!${NC}"
echo -e "Podés abrirlo desde el ${BLUE}menú de aplicaciones de Omarchy${NC} o ejecutando '${BLUE}omado${NC}' en la terminal."
