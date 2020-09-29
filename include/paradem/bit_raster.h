
#ifndef PARADEM_BITRASTER_H
#define PARADEM_BITRASTER_H

#include <stdint.h>
#include <vector>

class BitRaster {
private:
    std::vector< uint8_t > flag_array;
    int height, width;
    const uint8_t value[ 8 ] = { 128, 64, 32, 16, 8, 4, 2, 1 };

public:
    BitRaster( int height, int width );
    void setFlag( int row, int col );
    bool isFlagged( int row, int col );
};

#endif
