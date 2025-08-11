#!/bin/bash
# OpenRV Theme Switcher
# Usage: ./switch_theme.sh [blue|green|purple|original]

THEME_DIR="src/lib/app/RvCommon"

case "$1" in
    "blue")
        echo "Switching to blue theme..."
        cp ${THEME_DIR}/rv_mac_dark_blue.qss ${THEME_DIR}/rv_mac_dark.qss
        ;;
    "green") 
        echo "Switching to green theme..."
        cp ${THEME_DIR}/rv_mac_dark_green.qss ${THEME_DIR}/rv_mac_dark.qss
        ;;
    "purple")
        echo "Switching to purple theme..."
        cp ${THEME_DIR}/rv_mac_dark_purple.qss ${THEME_DIR}/rv_mac_dark.qss
        ;;
    "original")
        echo "Switching to original theme..."
        cp ${THEME_DIR}/rv_mac_dark_original.qss ${THEME_DIR}/rv_mac_dark.qss
        ;;
    *)
        echo "Available themes: blue, green, purple, original"
        echo "Usage: $0 [theme_name]"
        exit 1
        ;;
esac

echo "Theme switched! Run 'make clean && make -j8' to rebuild." 
