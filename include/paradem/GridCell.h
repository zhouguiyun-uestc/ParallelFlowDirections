#pragma once

class GridCell
{
public:
	int row;
	int col;
public:
	GridCell()=default;
	GridCell(int row, int col);
	~GridCell()=default;
};

