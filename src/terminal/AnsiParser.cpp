#include "AnsiParser.h"
#include <QDebug>

AnsiParser::AnsiParser(TerminalBuffer* buffer, QObject* parent)
    : QObject(parent), m_buffer(buffer)
{}

void AnsiParser::feed(const QByteArray& data)
{
    for (unsigned char c : data)
        processChar(c);

    emit bufferChanged();
}

void AnsiParser::processChar(unsigned char c)
{
    switch (m_state) {

        // -----------------------------------------------------------------
    case ParserState::Normal:
        if (c == 0x1B) {                        // ESC
            m_state = ParserState::Escape;
        }
        else if (c == '\r') {
            carriageReturn();
        }
        else if (c == '\n' || c == 0x0B || c == 0x0C) {
            lineFeed();
        }
        else if (c == '\b') {                 // Backspace
            if (m_cursorCol > 0) {
                m_cursorCol--;
                m_buffer->cell(m_cursorCol, m_cursorRow).reset(); // Marque la cellule comme dirty pour forcer le repaint
            }
        }
        else if (c == '\t') {                 // Tab — aligne sur multiple de 8
            int next = ((m_cursorCol / 8) + 1) * 8;
            m_cursorCol = qMin(next, m_buffer->cols() - 1);
        }
        else if (c == 0x07) {                 // BEL — ignoré
        }
        else if (c >= 0xC0){
            //Debut sequence UTF-8
			m_utf8Accum.clear();
			m_utf8Accum.append(static_cast<char>(c));
        }
		else if (c >= 0x80) {                 // Continuation d'une séquence UTF-8
            m_utf8Accum.append(static_cast<char>(c));
			QString s = QString::fromUtf8(m_utf8Accum);
            if (!s.isEmpty() && s[0] != QChar::ReplacementCharacter) {
                writeChar(s[0]);
                m_utf8Accum.clear();
			}
        }
        else if (c >= 0x20) {                 // Caractère imprimable
            writeChar(QChar(c));
        }
        break;

        // -----------------------------------------------------------------
    case ParserState::Escape:
        if (c == '[') {
            m_state = ParserState::CSI;
            m_csiParams.clear();
        }
        else if (c == ']') {
            m_state = ParserState::OSC;
            m_oscBuffer.clear();
        }
        else if (c == '7') {                  // Sauvegarde curseur
            m_savedCol = m_cursorCol;
            m_savedRow = m_cursorRow;
            m_state = ParserState::Normal;
        }
        else if (c == '8') {                  // Restaure curseur
            m_cursorCol = m_savedCol;
            m_cursorRow = m_savedRow;
            m_state = ParserState::Normal;
        }
        else if (c == 'D') {                  // Index (line feed)
            lineFeed();
            m_state = ParserState::Normal;
        }
        else if (c == 'M') {                  // Reverse index (scroll down)
            if (m_cursorRow == m_buffer->scrollRegionTop()) {
                // Insère une ligne en haut de la scroll region
                // (vim utilise ça pour scroller vers le bas)
                // Pour simplifier on remet juste le curseur
                m_cursorRow = qMax(0, m_cursorRow - 1);
            }
            else {
                m_cursorRow = qMax(0, m_cursorRow - 1);
            }
            m_state = ParserState::Normal;
        }
        else if (c == 'c') {                  // Reset terminal
            m_buffer->clearAll();
            m_cursorCol = 0;
            m_cursorRow = 0;
            m_fg = kDefaultFg;
            m_bg = kDefaultBg;
            m_bold = m_italic = m_underline = false;
            m_state = ParserState::Normal;
        }
        else {
            // Séquence ESC inconnue — on ignore
            m_state = ParserState::Normal;
        }
        break;

        // -----------------------------------------------------------------
    case ParserState::CSI:
        if (c >= 0x40 && c <= 0x7E) {          // Lettre finale — on traite
            m_csiFinal = static_cast<char>(c);           // Sentinel pour le parsing
            handleCSI();
            m_state = ParserState::Normal;
            m_csiParams.clear();
			m_csiFinal = 0;
        }
        else {
            m_csiParams.append(static_cast<char>(c));
        }
        break;

        // -----------------------------------------------------------------
    case ParserState::OSC:
        if (c == 0x07 || c == 0x1B) {          // BEL ou ESC termine l'OSC
            handleOSC();
            m_state = ParserState::Normal;
            m_oscBuffer.clear();
        }
        else {
            m_oscBuffer.append(static_cast<char>(c));
        }
        break;
    }
}

// -----------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------

void AnsiParser::writeChar(QChar ch)
{
    if (m_cursorCol >= m_buffer->cols()) {
        // Wrap automatique en fin de ligne
        m_cursorCol = 0;
        lineFeed();
    }

    TerminalCell& c = m_buffer->cell(m_cursorCol, m_cursorRow);

	QColor newFg = m_bold ? m_fg.lighter(130) : m_fg;
    if (c.ch != ch || c.fg != newFg || c.bg != m_bg || c.bold != m_bold || c.italic != m_italic || c.underline != m_underline) {
        c.ch = ch;
        c.fg = newFg;
        c.bg = m_bg;
        c.bold = m_bold;
        c.italic = m_italic;
        c.underline = m_underline;
        c.dirty = true;
    }

    m_cursorCol++;
}

void AnsiParser::lineFeed()
{
    if (m_cursorRow >= m_buffer->scrollRegionBottom()) {
        m_buffer->scrollUp();
    }
    else {
        m_cursorRow++;
    }
}

void AnsiParser::carriageReturn()
{
    m_cursorCol = 0;
}

void AnsiParser::moveCursor(int col, int row)
{
    m_cursorCol = col;
    m_cursorRow = row;
    clampCursor();
}

void AnsiParser::clampCursor()
{
    m_cursorCol = qBound(0, m_cursorCol, m_buffer->cols() - 1);
    m_cursorRow = qBound(0, m_cursorRow, m_buffer->rows() - 1);
}

// -----------------------------------------------------------------
// Erase
// -----------------------------------------------------------------

void AnsiParser::eraseInDisplay(int mode)
{
    switch (mode) {
    case 0: // Curseur → fin de l'écran
        eraseInLine(0);
        for (int r = m_cursorRow + 1; r < m_buffer->rows(); ++r)
            m_buffer->clearLine(r);
        break;
    case 1: // Début → curseur
        for (int r = 0; r < m_cursorRow; ++r)
            m_buffer->clearLine(r);
        eraseInLine(1);
        break;
    case 2: // Tout l'écran
        m_buffer->clearAll();
		m_cursorCol = 0;
		m_cursorRow = 0;
        break;
    case 3: // Tout + scrollback (xterm extension)
        m_buffer->clearAll();
        break;
    }
}

void AnsiParser::eraseInLine(int mode)
{
    int col = m_cursorCol;
    int row = m_cursorRow;

    switch (mode) {
    case 0: // Curseur → fin de ligne
        for (int c = col; c < m_buffer->cols(); ++c)
            m_buffer->cell(c, row).reset();
        break;
    case 1: // Début → curseur
        for (int c = 0; c <= col; ++c)
            m_buffer->cell(c, row).reset();
        break;
    case 2: // Ligne entière
        m_buffer->clearLine(row);
        break;
    }
}

// -----------------------------------------------------------------
// CSI handler
// -----------------------------------------------------------------

void AnsiParser::handleCSI()
{
    // m_csiParams contient les paramètres bruts, m_csiFinal la lettre finale
    char finalByte = m_csiFinal;
    QByteArray raw = m_csiParams;

    // Parse les paramètres numériques
    QList<int> params;
    // Ignore le '?' préfixe (séquences privées DEC)
    QByteArray paramStr = raw;
    bool isPrivate = (!paramStr.isEmpty() && paramStr[0] == '?');
    if (isPrivate) paramStr = paramStr.mid(1);

    if (!paramStr.isEmpty()) {
        for (const QByteArray& p : paramStr.split(';')) {
            bool ok;
            int v = p.trimmed().toInt(&ok);
            params.append(ok ? v : 0);
        }
    }

    auto param = [&](int i, int def = 0) -> int {
        return (i < params.size()) ? params[i] : def;
        };

    // Séquences privées DEC (?...)
    if (isPrivate) {
        if (finalByte == 'h' || finalByte == 'l') {
            bool enable = (finalByte == 'h');
            for (int p : params) {
                if (p == 1049 || p == 1047 || p == 47) {
                    if (enable) {
                        m_savedCol = m_cursorCol;
                        m_savedRow = m_cursorRow;
                        m_buffer->clearAll();
                        m_cursorCol = 0;
                        m_cursorRow = 0;
                    }
                    else {
                        m_buffer->clearAll();
                        m_cursorCol = m_savedCol;
                        m_cursorRow = m_savedRow;
                    }
                }
                // ?25h/?25l (show/hide cursor) — ignoré pour l'instant
            }
        }
        return;
    }

    switch (finalByte) {

    case 'A': // CUU — curseur haut
        m_cursorRow = qMax(0, m_cursorRow - qMax(1, param(0, 1)));
        break;

    case 'B': // CUD — curseur bas
        m_cursorRow = qMin(m_buffer->rows() - 1, m_cursorRow + qMax(1, param(0, 1)));
        break;

    case 'C': // CUF — curseur droite
        m_cursorCol = qMin(m_buffer->cols() - 1, m_cursorCol + qMax(1, param(0, 1)));
        break;

    case 'D': // CUB — curseur gauche
        m_cursorCol = qMax(0, m_cursorCol - qMax(1, param(0, 1)));
        break;

    case 'E': // CNL — curseur ligne suivante
        m_cursorRow = qMin(m_buffer->rows() - 1, m_cursorRow + qMax(1, param(0, 1)));
        m_cursorCol = 0;
        break;

    case 'F': // CPL — curseur ligne précédente
        m_cursorRow = qMax(0, m_cursorRow - qMax(1, param(0, 1)));
        m_cursorCol = 0;
        break;

    case 'G': // CHA — curseur colonne absolue
        m_cursorCol = qBound(0, param(0, 1) - 1, m_buffer->cols() - 1);
        break;

    case 'H': // CUP — position absolue (row; col), 1-based
    case 'f':
        moveCursor(param(1, 1) - 1, param(0, 1) - 1);
        break;

    case 'J': // ED — erase in display
        eraseInDisplay(param(0, 0));
        break;

    case 'K': // EL — erase in line
        eraseInLine(param(0, 0));
        break;

    case 'L': // IL — insert lines
    {
        int n = qMax(1, param(0, 1));
        for (int i = 0; i < n; ++i) {
            // Décale les lignes vers le bas dans la scroll region
            for (int r = m_buffer->scrollRegionBottom(); r > m_cursorRow; --r)
                m_buffer->cell(0, r); // accès pour forcer dirty — simplification
            m_buffer->clearLine(m_cursorRow);
        }
        m_buffer->markAllDirty();
        break;
    }

    case 'M': // DL — delete lines
    {
        int n = qMax(1, param(0, 1));
        for (int i = 0; i < n; ++i)
            m_buffer->scrollUp();
        break;
    }

    case 'P': // DCH — delete characters
    {
        int n = qMax(1, param(0, 1));
        int row = m_cursorRow;
        for (int c = m_cursorCol; c < m_buffer->cols(); ++c) {
            int src = c + n;
            if (src < m_buffer->cols())
                m_buffer->cell(c, row) = m_buffer->cell(src, row);
            else
                m_buffer->cell(c, row).reset();
        }
        break;
    }

    case 'S': // SU — scroll up n lignes
    {
        int n = qMax(1, param(0, 1));
        for (int i = 0; i < n; ++i)
            m_buffer->scrollUp();
        break;
    }

    case 'd': // VPA — ligne absolue
        m_cursorRow = qBound(0, param(0, 1) - 1, m_buffer->rows() - 1);
        break;

    case 'h': // SM — set mode (on ignore la plupart, sauf ?25h curseur visible)
    case 'l': // RM — reset mode
    {   // Détecte ?1049h (alternate screen) et ?1049l (retour écran principal)
        // et ?1047h / ?47h — variantes
        QByteArray p = m_csiParams;
        p.chop(1); // retire sentinel
        if (!p.isEmpty()) p.chop(1); // retire h ou l
        if (p == "?1049" || p == "?1047" || p == "?47") {
            if (finalByte == 'h') {
                // Entrée alternate screen — sauvegarde curseur + efface
                m_savedCol = m_cursorCol;
                m_savedRow = m_cursorRow;
                m_buffer->clearAll();
                m_cursorCol = 0;
                m_cursorRow = 0;
            }
            else {
                // Retour écran principal — restore curseur + efface
                m_buffer->clearAll();
                m_cursorCol = m_savedCol;
                m_cursorRow = m_savedRow;
            }
        }
        break;
    }

    case 'm': // SGR — couleurs et attributs
        handleSGR();
        break;

    case 'r': // DECSTBM — set scroll region
    {
        int top = qMax(1, param(0, 1)) - 1;
        int bottom = qMax(1, param(1, m_buffer->rows())) - 1;
        m_buffer->setScrollRegion(top, bottom);
        moveCursor(0, 0);
        break;
    }

    case 's': // SCP — save cursor
        m_savedCol = m_cursorCol;
        m_savedRow = m_cursorRow;
        break;

    case 'u': // RCP — restore cursor
        m_cursorCol = m_savedCol;
        m_cursorRow = m_savedRow;
        break;

    default:
        break; // Séquence inconnue — on ignore silencieusement
    }
}

// -----------------------------------------------------------------
// SGR — Set Graphic Rendition (couleurs et styles)
// -----------------------------------------------------------------

void AnsiParser::handleSGR()
{
    QList<int> params;
    if (!m_csiParams.isEmpty()) {
        for (const QByteArray& p : m_csiParams.split(';')) {
            bool ok;
            int v = p.trimmed().toInt(&ok);
            params.append(ok ? v : 0);
        }
    }
    if (params.isEmpty()) params.append(0);

    int i = 0;
    while (i < params.size()) {
        int p = params[i];

        if (p == 0) {                           // Reset tout
            m_fg = kDefaultFg;
            m_bg = kDefaultBg;
            m_bold = m_italic = m_underline = false;
        }
        else if (p == 1) {
            m_bold = true;
        }
        else if (p == 3) {
            m_italic = true;
        }
        else if (p == 4) {
            m_underline = true;
        }
        else if (p == 22) {
            m_bold = false;
        }
        else if (p == 23) {
            m_italic = false;
        }
        else if (p == 24) {
            m_underline = false;
        }
        else if (p >= 30 && p <= 37) {        // FG couleur 0-7
            m_fg = ansiColors[p - 30];
        }
        else if (p == 38) {                   // FG étendu
            if (i + 1 < params.size() && params[i + 1] == 5 && i + 2 < params.size()) {
                m_fg = color256(params[i + 2]);
                i += 2;
            }
            else if (i + 1 < params.size() && params[i + 1] == 2 && i + 4 < params.size()) {
                m_fg = QColor(params[i + 2], params[i + 3], params[i + 4]);
                i += 4;
            }
        }
        else if (p == 39) {                   // FG défaut
            m_fg = kDefaultFg;
        }
        else if (p >= 40 && p <= 47) {        // BG couleur 0-7
            m_bg = ansiColors[p - 40];
        }
        else if (p == 48) {                   // BG étendu
            if (i + 1 < params.size() && params[i + 1] == 5 && i + 2 < params.size()) {
                m_bg = color256(params[i + 2]);
                i += 2;
            }
            else if (i + 1 < params.size() && params[i + 1] == 2 && i + 4 < params.size()) {
                m_bg = QColor(params[i + 2], params[i + 3], params[i + 4]);
                i += 4;
            }
        }
        else if (p == 49) {                   // BG défaut
            m_bg = kDefaultBg;
        }
        else if (p >= 90 && p <= 97) {        // FG bright 8-15
            m_fg = ansiColors[p - 90 + 8];
        }
        else if (p >= 100 && p <= 107) {      // BG bright 8-15
            m_bg = ansiColors[p - 100 + 8];
        }

        i++;
    }
}

// -----------------------------------------------------------------
// OSC handler — titre de fenêtre
// -----------------------------------------------------------------

void AnsiParser::handleOSC()
{
    // Format: "0;titre" ou "2;titre"
    int semi = m_oscBuffer.indexOf(';');
    if (semi < 0) return;

    int cmd = m_oscBuffer.left(semi).toInt();
    if (cmd == 0 || cmd == 2) {
        QString title = QString::fromUtf8(m_oscBuffer.mid(semi + 1));
        emit titleChanged(title);
    }
}

// -----------------------------------------------------------------
// Palette 256 couleurs xterm
// -----------------------------------------------------------------

QColor AnsiParser::color256(int index)
{
    if (index < 0 || index > 255)
        return kDefaultFg;

    // 0-15 : couleurs ANSI standard
    if (index < 16)
        return ansiColors[index];

    // 16-231 : cube RGB 6x6x6
    if (index < 232) {
        int i = index - 16;
        int b = i % 6;
        int g = (i / 6) % 6;
        int r = i / 36;
        auto ch = [](int v) { return v ? 55 + v * 40 : 0; };
        return QColor(ch(r), ch(g), ch(b));
    }

    // 232-255 : dégradé de gris
    int gray = 8 + (index - 232) * 10;
    return QColor(gray, gray, gray);
}