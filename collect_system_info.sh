#!/bin/bash
# Script pour collecter les informations système pour le rapport de laboratoire

echo "======================================"
echo "INFORMATIONS SYSTÈME - RAPPORT DE LAB"
echo "======================================"
echo ""

# Système d'exploitation
echo "=== SYSTÈME D'EXPLOITATION ==="
echo "Distribution:"
if [ -f /etc/os-release ]; then
    cat /etc/os-release | grep -E "PRETTY_NAME|VERSION"
fi
uname -a
echo ""

# Version du noyau Linux
echo "=== NOYAU LINUX ==="
uname -r
echo ""

# Processeur
echo "=== PROCESSEUR ==="
lscpu | grep -E "Model name|Architecture|CPU\(s\)|Thread|Core"
echo ""

# Mémoire RAM
echo "=== MÉMOIRE RAM ==="
free -h
echo ""

# Carte graphique (si applicable)
echo "=== CARTE GRAPHIQUE ==="
lspci | grep -i vga
echo ""

# Version OpenCV
echo "=== OPENCV ==="
pkg-config --modversion opencv4 2>/dev/null || pkg-config --modversion opencv 2>/dev/null || echo "Version non détectée via pkg-config"
echo ""

# Version GCC/G++
echo "=== COMPILATEUR ==="
g++ --version | head -n 1
gcc --version | head -n 1
echo ""

# Python (si utilisé)
echo "=== PYTHON ==="
python3 --version 2>/dev/null || echo "Python3 non installé"
echo ""

# Caméra
echo "=== PÉRIPHÉRIQUES VIDÉO ==="
ls -l /dev/video* 2>/dev/null || echo "Aucun périphérique vidéo détecté"
echo ""
v4l2-ctl --list-devices 2>/dev/null || echo "v4l2-utils non installé (installer avec: sudo apt install v4l-utils)"
echo ""

# Information caméra détaillée (si v4l2 disponible)
echo "=== INFORMATIONS CAMÉRA (device 0) ==="
v4l2-ctl -d /dev/video0 --all 2>/dev/null || echo "Impossible d'accéder à /dev/video0"
echo ""

# Résolutions supportées
echo "=== RÉSOLUTIONS SUPPORTÉES ==="
v4l2-ctl -d /dev/video0 --list-formats-ext 2>/dev/null || echo "v4l2-utils requis"
echo ""

echo "======================================"
echo "Collection des informations terminée"
echo "======================================"
