#!/bin/bash
# Setup script for LaTeX thesis compilation on Arch Linux
# Installs all required packages for pdflatex compilation

set -e

echo "=== LaTeX Thesis Setup for Arch Linux ==="
echo ""

# Check if running as root for pacman
if [ "$EUID" -eq 0 ]; then
    PACMAN="pacman"
else
    PACMAN="sudo pacman"
fi

echo "[1/4] Installing TeX Live packages..."
$PACMAN -S --needed --noconfirm \
    texlive-latex \
    texlive-latexextra \
    texlive-latexrecommended \
    texlive-fontsrecommended \
    texlive-fontsextra \
    texlive-bibtexextra \
    texlive-pictures \
    texlive-binextra \
    texlive-plaingeneric \
    texlive-formatsextra \
    texlive-langgerman \
    texlive-basic

echo "[2/4] Installing build tools..."
$PACMAN -S --needed --noconfirm \
    python-pygments \
    inkscape \
    okular

echo "[3/4] Verifying installation..."
command -v pdflatex >/dev/null 2>&1 && echo "  pdflatex:  OK" || echo "  pdflatex:  MISSING"
command -v biber >/dev/null 2>&1 && echo "  biber:     OK" || echo "  biber:     MISSING"
command -v makeglossaries >/dev/null 2>&1 && echo "  makeglossaries: OK" || echo "  makeglossaries: MISSING"
command -v pygmentize >/dev/null 2>&1 && echo "  pygmentize: OK" || echo "  pygmentize: MISSING"
command -v inkscape >/dev/null 2>&1 && echo "  inkscape:  OK" || echo "  inkscape:  MISSING"
command -v okular >/dev/null 2>&1 && echo "  okular:    OK" || echo "  okular:    MISSING"
command -v latexmk >/dev/null 2>&1 && echo "  latexmk:   OK" || echo "  latexmk:   MISSING"

echo "[4/4] Done!"
echo ""
echo "To compile the thesis, run:"
echo "cd ../docs && make clean && make"
