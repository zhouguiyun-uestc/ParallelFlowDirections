#ifndef PARADEM_DEM_H
#define PARADEM_DEM_H
#include <paradem/grid.h>

#include <memory>

template <class T>
class Raster:public Grid<T>
{
public:
	T NoDataValue;
	std::shared_ptr<std::vector<double>> geoTransforms;
private:
	const int  dRow[8] = { 0, 1, 1, 1, 0,-1,-1,-1 };
	const int  dCol[8] = { 1, 1, 0,-1,-1,-1, 0, 1 };
public:
	double* getGeoTransformsPtr();
	Raster();
	~Raster();
	bool isNoData(int row, int col);
	bool isInGrid(int row, int col);
	int getRow(int dir, int row);
	int getCol(int dir, int col);
	std::vector<T> getRowData(int row);
	std::vector<T> getColData(int col);
	T getNoDataValue();

	//overload -
	Raster<T> operator-(Raster<T>& other);
	bool operator==(Raster<T>& other);
	Raster(const Raster<T>&) = delete;
	void operator=(const Raster<T>&) = delete;
	
	Raster(Raster<T>&&) = default; // forces a move constructor anyway

};

#endif

