#!/bin/bash
APP_NAME="NexusUtils"
EXECUTABLE="/usr/local/bin/NexusUtils"
INSTALL_DIR="/opt/$APP_NAME"
BIN_DIR="/usr/local/bin"
ICON_DIR="/usr/share/icons/hicolor/apps"
DESKTOP_FILE="/usr/share/applications/$APP_NAME.desktop"

# Créer le dossier d’installation
sudo mkdir -p "$INSTALL_DIR"
sudo cp $APP_NAME "$INSTALL_DIR/"
sudo chmod +x "$INSTALL_DIR/$APP_NAME"

# Lien symbolique dans /usr/local/bin
sudo ln -sf "$INSTALL_DIR/$APP_NAME" "$BIN_DIR/$APP_NAME"

# Installer l’icône
sudo mkdir -p "$ICON_DIR"
sudo cp Icone.png "$ICON_DIR/$APP_NAME.png"

# Créer le fichier .desktop
sudo tee "$DESKTOP_FILE" > /dev/null <<EOF
[Desktop Entry]
Name=$APP_NAME
Exec=$APP_NAME
Icon=$ICON_DIR/$APP_NAME.png
Type=Application
Categories=Utility;
StartupNotify=true
EOF

sudo cp "$DESKTOP_FILE" ../$APP_NAME.desktop
sudo chmod +x ../$APP_NAME.desktop

# Mise à jour caches
sudo update-desktop-database /usr/share/applications
sudo gtk-update-icon-cache /usr/share/icons/hicolor

# Détection de la distribution
if [ -f /etc/os-release ]; then
    source /etc/os-release
    DISTRO=$ID
    DISTRO_LIKE=$ID_LIKE
elif command -v lsb_release >/dev/null 2>&1; then
    DISTRO=$(lsb_release -si | tr '[:upper:]' '[:lower:]')
    DISTRO_LIKE=""
else
    echo "Impossible de détecter la distribution"
    exit 1
fi

# Normalisation via ID_LIKE
if [[ "$DISTRO_LIKE" == *"ubuntu"* ]]; then
    DISTRO="ubuntu"
elif [[ "$DISTRO_LIKE" == *"debian"* ]]; then
    DISTRO="debian"
elif [[ "$DISTRO_LIKE" == *"fedora"* ]]; then
    DISTRO="fedora"
elif [[ "$DISTRO_LIKE" == *"suse"* ]]; then
    DISTRO="opensuse"
elif [[ "$DISTRO_LIKE" == *"arch"* ]]; then
    DISTRO="arch"
fi

echo "Distribution détectée : $DISTRO"

# Liste des dépendances communes (exemple)
DEPSU="libxtst6 libxtst-dev x11proto-record-dev qt6-webengine"
DEPSA="libxtst xorg-recordproto qt6-webengine"
DEPSAA=""
DEPSF="libXtst libXtst-devel xorg-x11-proto-devel qt6-webengine"
DEPSO="libXtst6 libXtst-devel xorg-x11-devel qt6-webengine"

# Installation selon la distribution
case "$DISTRO" in
    ubuntu|debian)
        echo "Installation via apt..."
        sudo apt update
        sudo apt install -y $DEPSU
        ;;
    fedora)
        echo "Installation via dnf..."
        sudo dnf install -y $DEPSF
        ;;
    arch)
        echo "Installation via pacman..."
        sudo pacman -Sy --noconfirm $DEPSA
        if [ -n "$DEPSAA" ]; then
            sudo yay -S --noconfirm $DEPSAA
        fi
        ;;
    opensuse*|suse)
        echo "Installation via zypper..."
        sudo zypper install -y $DEPSO
        ;;
    *)
        echo "Distribution non supportée : $DISTRO"
        exit 1
        ;;
esac