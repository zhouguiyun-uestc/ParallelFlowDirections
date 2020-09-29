
#include <paradem/bit_raster.h>

BitRaster::BitRaster( int height, int width ) {
    // allocate flag array
    int nums = ( width * height + 7 ) / 8;
    flag_array.resize( nums, 0 );
    this->height = height;
    this->width  = width;
}

void BitRaster::setFlag( int row, int col ) {
    int index       = row * width + col;
    int indexOfByte = index / 8;
    int indexInByte = index % 8;
    flag_array[ indexOfByte ] |= value[ indexInByte ];
}

bool BitRaster::isFlagged( int row, int col ) {
    int     index       = row * width + col;
    int     indexOfByte = index / 8;
    int     indexInByte = index % 8;
    uint8_t buf         = flag_array[ indexOfByte ];
    buf &= value[ indexInByte ];
    if ( buf > 0 )
        return true;
    return false;
}
