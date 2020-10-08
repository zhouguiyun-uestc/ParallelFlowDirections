
#ifndef PARADEM_PSTORAGE_H
#define PARADEM_PSTORAGE_H

#include "raster.h"

#include <map>

typedef struct storageTile {
    Raster< int >      flowdirs;
    Raster< int >      labels;
    std::vector< int > IdBounderyCell;
    Raster< int >      lowEdgeMask;
    Raster< int >      highEdgeMask;
    Raster< float >    dem;
    std::vector< int > flatHeight;
    bool               NoFlat;
};

#endif
