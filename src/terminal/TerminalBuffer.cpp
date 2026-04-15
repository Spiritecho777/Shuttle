#include "TerminalBuffer.h"

TerminalBuffer::TerminalBuffer(int cols, int rows)
    : m_cols(cols), m_rows(rows)
    , m_scrollTop(0), m_scrollBottom(rows - 1)
{
    m_grid.resize(m_rows);
    for (auto& line : m_grid)
        line = makeEmptyLine();
}

void TerminalBuffer::resize(int cols, int rows)
{
    m_cols = cols;
    m_rows = rows;
    m_scrollTop = 0;
    m_scrollBottom = rows - 1;

    m_grid.resize(rows);
    for (auto& line : m_grid) {
        line.resize(cols);
        // Les nouvelles cellules créées par resize() sont déjà
        // initialisées via le constructeur par défaut de TerminalCell
    }

    markAllDirty();
}

TerminalCell& TerminalBuffer::cell(int col, int row)
{
    // Clamp défensif — l'AnsiParser peut envoyer des coords hors limites
    col = qBound(0, col, m_cols - 1);
    row = qBound(0, row, m_rows - 1);
    return m_grid[row][col];
}

const TerminalCell& TerminalBuffer::cell(int col, int row) const
{
    col = qBound(0, col, m_cols - 1);
    row = qBound(0, row, m_rows - 1);
    return m_grid[row][col];
}

void TerminalBuffer::clearLine(int row)
{
    if (row < 0 || row >= m_rows) return;
    m_grid[row] = makeEmptyLine();
}

void TerminalBuffer::clearAll()
{
    for (int r = 0; r < m_rows; ++r)
        m_grid[r] = makeEmptyLine();
}

void TerminalBuffer::scrollUp()
{
    int top = m_scrollTop;
    int bottom = m_scrollBottom;

    // Push la ligne du haut dans le scrollback
    if (m_scrollback.size() >= kMaxScrollback)
        m_scrollback.removeFirst();
    m_scrollback.append(m_grid[top]);

    // Décale les lignes de la scroll region vers le haut
    for (int r = top; r < bottom; ++r)
        m_grid[r] = m_grid[r + 1];

    // Nouvelle ligne vide en bas de la scroll region
    m_grid[bottom] = makeEmptyLine();

    markAllDirty();
}

void TerminalBuffer::setScrollRegion(int top, int bottom)
{
    m_scrollTop = qBound(0, top, m_rows - 1);
    m_scrollBottom = qBound(0, bottom, m_rows - 1);

    if (m_scrollTop > m_scrollBottom)
        m_scrollTop = m_scrollBottom;
}

void TerminalBuffer::markAllDirty()
{
    for (auto& line : m_grid)
        for (auto& c : line)
            c.dirty = true;
}

QVector<TerminalCell> TerminalBuffer::makeEmptyLine() const
{
    return QVector<TerminalCell>(m_cols); // TerminalCell() par défaut = espace blanc sur noir
}