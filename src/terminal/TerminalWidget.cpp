#include "TerminalWidget.h"
#include "../ssh/SSHSession.h"

#include <QPainter>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QFocusEvent>
#include <QResizeEvent>
#include <QFontDatabase>

TerminalWidget::TerminalWidget(QWidget* parent)
    : QWidget(parent)
{
    // Police monospace — on cherche dans l'ordre
    const QStringList candidates = {
        "Cascadia Code", "JetBrains Mono", "Consolas",
        "DejaVu Sans Mono", "Liberation Mono", "Courier New"
    };

    m_font = QFont("Courier New", 10); // fallback
    for (const QString& name : candidates) {
        if (QFontDatabase::families().contains(name)) {
            m_font = QFont(name, 10);
            break;
        }
    }
    m_font.setFixedPitch(true);
    m_font.setStyleHint(QFont::Monospace);

    // Calcul dimensions cellule
    QFontMetrics fm(m_font);
    m_cellW = fm.horizontalAdvance('W'); // largeur caractère le plus large
    m_cellH = fm.height();

    // Buffer initial 80x24
    m_buffer = new TerminalBuffer(m_cols, m_rows);
    m_parser = new AnsiParser(m_buffer, this);

    connect(m_parser, &AnsiParser::bufferChanged, this, [this]() {
        update(); // repaint Qt
        });
    connect(m_parser, &AnsiParser::titleChanged,
        this, &TerminalWidget::titleChanged);

    // Curseur clignotant
    m_blinkTimer = new QTimer(this);
    m_blinkTimer->setInterval(530);
    connect(m_blinkTimer, &QTimer::timeout, this, &TerminalWidget::blinkCursor);
    m_blinkTimer->start();

    // Focus clavier
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_InputMethodEnabled, false);

    // Fond noir
    QPalette p = palette();
    p.setColor(QPalette::Window, Qt::black);
    setPalette(p);
    setAutoFillBackground(true);
}

TerminalWidget::~TerminalWidget()
{
    detachSession();
}

// -----------------------------------------------------------------
// Session
// -----------------------------------------------------------------

void TerminalWidget::attachSession(SSHSession* session)
{
    detachSession();
    m_session = session;

    connect(m_session, &SSHSession::dataReceived,
        this, &TerminalWidget::onDataReceived);
    connect(m_session, &SSHSession::disconnected,
        this, &TerminalWidget::onSessionDisconnected);

    // Envoie la taille initiale du PTY
    recalcGrid();
}

void TerminalWidget::detachSession()
{
    if (!m_session) return;
    disconnect(m_session, nullptr, this, nullptr);
    m_session = nullptr;
}

// -----------------------------------------------------------------
// Données reçues du SSH
// -----------------------------------------------------------------

void TerminalWidget::onDataReceived(const QByteArray& data)
{
    m_parser->feed(data);
}

void TerminalWidget::onSessionDisconnected()
{
    m_session = nullptr;
    // Affiche un message dans le terminal
    m_parser->feed("\r\n\x1b[33m[Session fermée]\x1b[0m\r\n");
    emit sessionClosed();
}

// -----------------------------------------------------------------
// Resize — recalcule cols/rows et notifie le PTY
// -----------------------------------------------------------------

void TerminalWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    recalcGrid();
}

void TerminalWidget::recalcGrid()
{
    if (m_cellW <= 0 || m_cellH <= 0) return;

    int newCols = qMax(1, width() / m_cellW);
    int newRows = qMax(1, height() / m_cellH);

    if (newCols == m_cols && newRows == m_rows) return;

    m_cols = newCols;
    m_rows = newRows;

    m_buffer->resize(m_cols, m_rows);

    // Notifie le serveur SSH du nouveau PTY size
    if (m_session) {
        // libssh2_channel_request_pty_size est thread-safe si appelé
        // depuis le même thread que la session — on passe par writeData
        // qui est sûr, ou on expose une méthode dédiée dans SSHSession
        m_session->resizePty(m_cols, m_rows);
    }
}

// -----------------------------------------------------------------
// Rendu
// -----------------------------------------------------------------

void TerminalWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setFont(m_font);

    QFontMetrics fm(m_font);
    int baseline = fm.ascent(); // offset texte dans la cellule

    int totalRows = m_rows;

    for (int row = 0; row < totalRows; ++row) {
        for (int col = 0; col < m_cols; ++col) {

            // Si on est dans le scrollback
            int bufRow = row;
            const TerminalCell* cellPtr = nullptr;

            if (m_scrollOffset > 0) {
                int sbSize = m_buffer->scrollbackSize();
                int sbRow = sbSize - m_scrollOffset + row;
                if (sbRow >= 0 && sbRow < sbSize) {
                    cellPtr = &m_buffer->scrollback()[sbRow][qMin(col, (int)m_buffer->scrollback()[sbRow].size() - 1)];
                }
                else {
                    bufRow = row - (m_scrollOffset - sbSize);
                    if (bufRow >= 0 && bufRow < m_rows)
                        cellPtr = &m_buffer->cell(col, bufRow);
                }
            }
            else {
                cellPtr = &m_buffer->cell(col, bufRow);
            }

            if (!cellPtr) continue;
            const TerminalCell& cell = *cellPtr;

            int x = col * m_cellW;
            int y = row * m_cellH;

            // Fond
            painter.fillRect(x, y, m_cellW, m_cellH, cell.bg);

            // Curseur
            bool isCursor = (m_scrollOffset == 0)
                && (col == m_parser->cursorCol())
                && (row == m_parser->cursorRow())
                && m_cursorVisible
                && m_cursorBlink
                && hasFocus();

            if (isCursor)
                painter.fillRect(x, y, m_cellW, m_cellH, cell.fg);

            // Texte
            if (!cell.ch.isNull() && cell.ch != ' ') {
                QFont f = m_font;
                f.setBold(cell.bold);
                f.setItalic(cell.italic);
                f.setUnderline(cell.underline);
                painter.setFont(f);

                painter.setPen(isCursor ? cell.bg : cell.fg);
                painter.drawText(x, y + baseline, cell.ch);
            }
        }
    }
}

// -----------------------------------------------------------------
// Curseur clignotant
// -----------------------------------------------------------------

void TerminalWidget::blinkCursor()
{
    m_cursorBlink = !m_cursorBlink;
    // Repaint uniquement la cellule curseur
    int x = m_parser->cursorCol() * m_cellW;
    int y = m_parser->cursorRow() * m_cellH;
    update(x, y, m_cellW, m_cellH);
}

void TerminalWidget::focusInEvent(QFocusEvent* event)
{
    QWidget::focusInEvent(event);
    m_cursorVisible = true;
    m_blinkTimer->start();
    update();
}

void TerminalWidget::focusOutEvent(QFocusEvent* event)
{
    QWidget::focusOutEvent(event);
    m_cursorBlink = true; // curseur plein quand pas de focus
    m_blinkTimer->stop();
    update();
}

// -----------------------------------------------------------------
// Clavier
// -----------------------------------------------------------------

void TerminalWidget::keyPressEvent(QKeyEvent* event)
{
    if (!m_session) return;

    // Remettre en bas si on était dans le scrollback
    if (m_scrollOffset > 0) {
        m_scrollOffset = 0;
        update();
    }

    QByteArray seq = keyEventToSequence(event);
    if (!seq.isEmpty())
        sendToSession(seq);
}

QByteArray TerminalWidget::keyEventToSequence(QKeyEvent* event)
{
    // Ctrl+A → Ctrl+Z
    if (event->modifiers() & Qt::ControlModifier) {
        QString text = event->text();
        if (!text.isEmpty()) {
            char c = text[0].toLatin1();
            if (c >= 'a' && c <= 'z') return QByteArray(1, c - 'a' + 1);
            if (c >= 'A' && c <= 'Z') return QByteArray(1, c - 'A' + 1);
        }
        // Ctrl+[ = ESC
        if (event->key() == Qt::Key_BracketLeft) return "\x1b";
    }

    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:      return "\r";
    case Qt::Key_Backspace:  return "\x7f";
    case Qt::Key_Tab:        return "\t";
    case Qt::Key_Escape:     return "\x1b";
    case Qt::Key_Up:         return "\x1b[A";
    case Qt::Key_Down:       return "\x1b[B";
    case Qt::Key_Right:      return "\x1b[C";
    case Qt::Key_Left:       return "\x1b[D";
    case Qt::Key_Home:       return "\x1b[H";
    case Qt::Key_End:        return "\x1b[F";
    case Qt::Key_PageUp:     return "\x1b[5~";
    case Qt::Key_PageDown:   return "\x1b[6~";
    case Qt::Key_Delete:     return "\x1b[3~";
    case Qt::Key_Insert:     return "\x1b[2~";
    case Qt::Key_F1:         return "\x1bOP";
    case Qt::Key_F2:         return "\x1bOQ";
    case Qt::Key_F3:         return "\x1bOR";
    case Qt::Key_F4:         return "\x1bOS";
    case Qt::Key_F5:         return "\x1b[15~";
    case Qt::Key_F6:         return "\x1b[17~";
    case Qt::Key_F7:         return "\x1b[18~";
    case Qt::Key_F8:         return "\x1b[19~";
    case Qt::Key_F9:         return "\x1b[20~";
    case Qt::Key_F10:        return "\x1b[21~";
    case Qt::Key_F11:        return "\x1b[23~";
    case Qt::Key_F12:        return "\x1b[24~";
    default:
        // Caractère normal
        QString text = event->text();
        if (!text.isEmpty())
            return text.toUtf8();
        break;
    }
    return {};
}

void TerminalWidget::sendToSession(const QByteArray& data)
{
    if (m_session)
        m_session->writeData(data);
}

// -----------------------------------------------------------------
// Scroll molette — navigue dans le scrollback
// -----------------------------------------------------------------

void TerminalWidget::wheelEvent(QWheelEvent* event)
{
    int delta = event->angleDelta().y();
    int lines = delta / 40; // ~3 lignes par cran

    m_scrollOffset = qBound(0,
        m_scrollOffset - lines,
        m_buffer->scrollbackSize());
    update();
}

void TerminalWidget::setScrollOffset(int offset)
{
    m_scrollOffset = qBound(0, offset, m_buffer->scrollbackSize());
    update();
}