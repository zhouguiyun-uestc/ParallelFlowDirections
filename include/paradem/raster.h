
#ifndef PARADEM_DEM_H
#define PARADEM_DEM_H

#include <memory>
#include <paradem/grid.h>

template < class T > class Raster : public Grid< T > {
public:
    T NoDataValue;
    std::shared_ptr< std::vector< double > > geoTransforms;

private:
    int dRow[ 8 ] = { 0, 1, 1, 1, 0, -1, -1, -1 };
    int dCol[ 8 ] = { 1, 1, 0, -1, -1, -1, 0, 1 };

public:
    double* getGeoTransformsPtr();
    Raster();
    ~Raster();
    bool isNoData( int row, int col );
    bool isInGrid( int row, int col );
    int getRow( int dir, int row );
    int getCol( int dir, int col );
    std::vector< T > getRowData( int row );
    std::vector< T > getColData( int col );
    T getNoDataValue();
    Raster< T > operator-( Raster< T >& other );
    bool operator==( Raster< T >& other );
};

#endif
