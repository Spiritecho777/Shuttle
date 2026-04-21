#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QColor>
#include <QVector>

#include "TerminalBuffer.h"
#include "TerminalCell.h"

// État de la state machine
enum class ParserState {
    Normal,       // Caractère normal
    Escape,       // On a reçu ESC (0x1B)
    CSI,          // On a reçu ESC [ — Control Sequence Introducer
    OSC,          // On a reçu ESC ] — Operating System Command (titre fenêtre etc.)
};

class AnsiParser : public QObject
{
    Q_OBJECT

public:
    explicit AnsiParser(TerminalBuffer* buffer, QObject* parent = nullptr);

    // Point d'entrée principal — appelé depuis TerminalWidget::onDataReceived()
    void feed(const QByteArray& data);

    // Position courante du curseur
    int cursorCol() const { return m_cursorCol; }
    int cursorRow() const { return m_cursorRow; }

signals:
    // Émis quand le buffer a changé et qu'un repaint est nécessaire
    void bufferChanged();

    // Émis quand le titre de la fenêtre change (séquence OSC 0/2)
    void titleChanged(const QString& title);

private:
    void processChar(unsigned char c);

    // Handlers par type de séquence
    void handleCSI();       // ESC [ ... lettre finale
    void handleSGR();       // ESC [ ... m  — attributs couleur/style
    void handleOSC();       // ESC ] ... ST — titre fenêtre

    // Mouvements curseur
    void moveCursor(int col, int row);
    void clampCursor();

    // Écriture d'un caractère à la position curseur
    void writeChar(QChar ch);

    // Scroll
    void lineFeed();        // \n ou ESC D
    void carriageReturn();  // \r

    // Efface
    void eraseInDisplay(int mode);  // ED  — mode 0=curseur→fin, 1=début→curseur, 2=tout
    void eraseInLine(int mode);     // EL  — mode 0=curseur→fin ligne, 1=début→curseur, 2=ligne entière

    // Couleur 256 (ESC [ 38;5;n m  ou  ESC [ 48;5;n m)
    static QColor color256(int index);

    // Attributs courants (appliqués à chaque writeChar)
    QColor m_fg = kDefaultFg;
    QColor m_bg = kDefaultBg;
    bool   m_bold = false;
    bool   m_italic = false;
    bool   m_underline = false;

	char m_csiFinal = 0; // Lettre finale de la séquence CSI en cours (m, H, J, etc.)

    // Curseur
    int m_cursorCol = 0;
    int m_cursorRow = 0;

    // Curseur sauvegardé (ESC 7 / ESC 8)
    int m_savedCol = 0;
    int m_savedRow = 0;

    // State machine
    ParserState      m_state = ParserState::Normal;
    QByteArray       m_csiParams;   // accumule les paramètres CSI
    QByteArray       m_oscBuffer;   // accumule le contenu OSC
	QByteArray       m_utf8Accum;  // accumule les octets d'un caractère UTF-8 incomplet

    TerminalBuffer* m_buffer;
};