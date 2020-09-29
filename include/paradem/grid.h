#ifndef PARADEM_GRID_H
#define PARADEM_GRID_H

#include <vector>

template < class T > class Grid {
protected:
    std::vector< T > data;
    int              height, width;

public:
    Grid();
    bool init( int height, int width );
    void setAllValues( T val );
    T&   at( int row, int col );
    T*   operator&();
    int  getHeight();
    int  getWidth();
    int  isNull();
    ~Grid();
};

#endif
