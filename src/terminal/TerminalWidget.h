#pragma once

#include <QWidget>
#include <QTimer>
#include <QFont>
#include <QFontMetrics>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QMenu>

#include "TerminalBuffer.h"
#include "AnsiParser.h"
#include "../ssh/SSHSession.h"
#include "../ssh/SessionProfile.h"

class TerminalWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TerminalWidget(QWidget* parent = nullptr);
    ~TerminalWidget();

    // Attache une session SSH au widget
    void attachSession(SSHSession* session);
    void detachSession();

    // Taille en colonnes/lignes (pour le PTY resize)
    int termCols() const { return m_cols; }
    int termRows() const { return m_rows; }

    // Fait défiler le scrollback
    void setScrollOffset(int offset);
    SSHSession* currentSession() const { return m_session; }

    // SFTP
    void setProfile(const SessionProfile& p);
	const SessionProfile& profile() const { return m_profile; }

signals:
    void titleChanged(const QString& title);
    void sessionClosed();

protected:
    void paintEvent(QPaintEvent* event)     override;
    void resizeEvent(QResizeEvent* event)   override;
    void keyPressEvent(QKeyEvent* event)    override;
    void focusInEvent(QFocusEvent* event)   override;
    void focusOutEvent(QFocusEvent* event)  override;
    void wheelEvent(QWheelEvent* event)     override;

	void mousePressEvent(QMouseEvent* event)    override;
	void mouseReleaseEvent(QMouseEvent* event)  override;
	void mouseMoveEvent(QMouseEvent* event)     override;

private slots:
    void onDataReceived(const QByteArray& data);
    void onSessionDisconnected();
    void blinkCursor();

private:
    void recalcGrid();
    void sendToSession(const QByteArray& data);
    void paintCell(QPainter& painter, int col, int row, int scrollOffset);
    
    void renderToBuffer();
	void renderCell(QPainter& painter, int col, int row);

    QByteArray keyEventToSequence(QKeyEvent* event);

	QImage m_backBuffer;   // pour double-buffering du rendu

    // Layout
    int m_cols = 80;
    int m_rows = 24;

    // Police monospace
    QFont        m_font;
    int          m_cellW = 0;   // largeur d'une cellule en pixels
    int          m_cellH = 0;   // hauteur d'une cellule en pixels

    // Buffer + parser
    TerminalBuffer* m_buffer = nullptr;
    AnsiParser* m_parser = nullptr;

    // Session attachée
    SSHSession* m_session = nullptr;

    // Curseur
    bool  m_cursorVisible = true;
    bool  m_cursorBlink = true;   // état courant du blink
    QTimer* m_blinkTimer = nullptr;

    // Scroll
    int m_scrollOffset = 0;   // 0 = bas (position normale), >0 = dans le scrollback

    //SFTP
	SessionProfile m_profile;

	// Sélection de texte
    QPoint m_selStart = { -1, -1 };
    QPoint m_selEnd = { -1, -1 };
	bool m_selecting = false;

	QPoint pixelToCell(const QPoint& px) const;
	QString selectedText() const;
	void clearSelection();

	// Menu contextuel
	void showContextMenu(const QPoint& pos);
};
