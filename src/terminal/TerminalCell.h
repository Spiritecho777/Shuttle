#pragma once

#include <QChar>
#include <QColor>

// Couleurs ANSI 16 de base (index 0-15)
// Utilisées par l'AnsiParser pour mapper les codes SGR
static const QColor ansiColors[16] = {
    // Normal (0-7)
    QColor(0,   0,   0),  // 0  Black
    QColor(170,   0,   0),  // 1  Red
    QColor(0, 170,   0),  // 2  Green
    QColor(170, 170,   0),  // 3  Yellow
    QColor(0,   0, 170),  // 4  Blue
    QColor(170,   0, 170),  // 5  Magenta
    QColor(0, 170, 170),  // 6  Cyan
    QColor(170, 170, 170),  // 7  White
    // Bright (8-15)
    QColor(85,  85,  85),  // 8  Bright Black (Grey)
    QColor(255,  85,  85),  // 9  Bright Red
    QColor(85, 255,  85),  // 10 Bright Green
    QColor(255, 255,  85),  // 11 Bright Yellow
    QColor(85,  85, 255),  // 12 Bright Blue
    QColor(255,  85, 255),  // 13 Bright Magenta
    QColor(85, 255, 255),  // 14 Bright Cyan
    QColor(255, 255, 255),  // 15 Bright White
};

// Couleur par défaut du terminal
static const QColor kDefaultFg = QColor(204, 204, 204);
static const QColor kDefaultBg = QColor(0, 0, 0);

struct TerminalCell {
    QChar  ch = ' ';
    QColor fg = kDefaultFg;
    QColor bg = kDefaultBg;
    bool   bold = false;
    bool   italic = false;
    bool   underline = false;
    bool   dirty = true;    // true = doit être repeint au prochain paintEvent

    // Remet la cellule à son état par défaut
    void reset() {
        ch = ' ';
        fg = kDefaultFg;
        bg = kDefaultBg;
        bold = false;
        italic = false;
        underline = false;
        dirty = true;
    }

    // Deux cellules sont visuellement identiques ?
    // Utilisé pour éviter de repeindre inutilement
    bool sameAppearance(const TerminalCell& o) const {
        return ch == o.ch
            && fg == o.fg
            && bg == o.bg
            && bold == o.bold
            && italic == o.italic
            && underline == o.underline;
    }
};