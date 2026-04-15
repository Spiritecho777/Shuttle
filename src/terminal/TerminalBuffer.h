#pragma once

#include <QVector>
#include "TerminalCell.h"

class TerminalBuffer
{
public:
	explicit TerminalBuffer(int cols = 80, int rows = 24);;

	//Redimesionnement de la grille
	void resize(int cols, int rows);

	//Accès à une cellule
	TerminalCell& cell(int col, int row);
	const TerminalCell& cell(int col, int row) const;

	void clearLine(int row);

	void clearAll();

	void scrollUp();
	
	void setScrollRegion(int top, int bottom);
	int scrollRegionTop() const { return m_scrollTop; }
	int scrollRegionBottom() const { return m_scrollBottom; }

	const QVector<QVector<TerminalCell>>& scrollback() const { return m_scrollback; }
	int scrollbackSize() const { return m_scrollback.size(); }

	int cols() const { return m_cols; }
	int rows() const { return m_rows; }

	void markAllDirty();

private:
	int m_cols
		, m_rows
		, m_scrollTop = 0
		, m_scrollBottom = 0;
	
	QVector<QVector<TerminalCell>> m_grid;

	QVector<QVector<TerminalCell>> m_scrollback;
	static const int kMaxScrollback = 5000;

	QVector<TerminalCell> makeEmptyLine() const;
};