
#include <paradem/raster.h>

#include <cmath>
#include <cstdlib>
#include <iostream>

template < class T > 
Raster< T >::Raster() {
    NoDataValue = 0;
}

template < class T > 
Raster< T >::~Raster() {}

template < class T >
bool Raster< T >::isNoData( int row, int col ) {
    if ( fabs( Raster< T >::at( row, col ) - NoDataValue ) < 0.00001 )
        return true;
    return false;
}

template < class T > 
int Raster< T >::getRow( int dir, int row ) {
    return row + dRow[ dir ];
}

template < class T > 
int Raster< T >::getCol( int dir, int col ) {
    return ( col + dCol[ dir ] );
}

template < class T > 
bool Raster< T >::isInGrid( int row, int col ) {
    if ( ( row >= 0 && row < this->height ) && ( col >= 0 && col < this->width ) )
        return true;
    return false;
}

template < class T > 
T Raster< T >::getNoDataValue() {
    return NoDataValue;
}

template < class T > 
double* Raster< T >::getGeoTransformsPtr() {
    if ( geoTransforms == nullptr )
        return nullptr;
    return &geoTransforms->at( 0 );
}

template < class T > 
std::vector< T > Raster< T >::getRowData( int row ) {
    return std::vector< T >( &this->data[ 0 ] + row * this->width, &this->data[ 0 ] + ( row + 1 ) * this->width );
}

template < class T > 
std::vector< T > Raster< T >::getColData( int col ) {
    std::vector< T > vec( this->height );
    for ( int row = 0; row < this->height; row++ ) {
        vec[ row ] = Raster< T >::at( row, col );
    }
    return vec;
}

template < class T > 
bool Raster< T >::operator==( Raster< T >& other ) {
    if ( other.getWidth() != Raster< T >::getWidth() ) {
        std::cout << "different this->width" << std::endl;
        return false;
    }
    if ( other.getHeight() != Raster< T >::getHeight() ) {
        std::cout << "different this->height" << std::endl;
        return false;
    }
    for ( int row = 0; row < this->height; row++ ) {
        for ( int col = 0; col < this->width; col++ ) {
            if ( Raster< T >::isNoData( row, col ) && other.isNoData( row, col ) )
                continue;
            if ( Raster< T >::isNoData( row, col ) && !other.isNoData( row, col ) )
                return false;
            if ( !Raster< T >::isNoData( row, col ) && other.isNoData( row, col ) )
                return false;
        }
    }
    return true;
}

template < class T > 
Raster< T > Raster< T >::operator-( Raster< T >& other ) {
    Raster< T > diff;
    if ( other.getWidth() != Raster< T >::getWidth() ) {
        std::cout << "different this->width" << std::endl;
        return diff;
    }
    if ( other.getHeight() != Raster< T >::getHeight() ) {
        std::cout << "different this->height" << std::endl;
        return diff;
    }
    diff.init( Raster< T >::getHeight(), Raster< T >::getWidth() );
    for ( int row = 0; row < this->height; row++ ) {
        for ( int col = 0; col < this->width; col++ ) {
            if ( this->isNoData( row, col ) || other.isNoData( row, col ) )
                diff.Raster< T >::at( row, col ) = diff.NoDataValue;
            T dif                            = Raster< T >::at( row, col ) - other.Raster< T >::at( row, col );
            diff.Raster< T >::at( row, col ) = dif;
        }
    }
    return diff;
}

template class Raster< float >;
template class Raster< int32_t >;
template class Raster< uint8_t >;
template class Raster< int8_t >;
