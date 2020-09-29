#include "outBoundary.h"

OutBoundary::OutBoundary( std::vector< float > dem, std::vector< int > low, std::vector< int > high, std::vector< int > ID ) {
    this->dem  = dem;
    this->low  = low;
    this->high = high;
    this->ID   = ID;
}

