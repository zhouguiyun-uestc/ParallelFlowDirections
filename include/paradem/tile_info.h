
#ifndef PARADEM_TILECOORD_H
#define PARADEM_TILECOORD_H

#include <cereal/archives/binary.hpp>
#include <iterator>

const uint8_t TILE_RIGHT = 1;         ///< Indicates a NullTile is on the LHS of the tile
const uint8_t TILE_RIGHT_BOTTOM = 2;  ///< Indicates a tile is on the top of a tile
const uint8_t TILE_BOTTOM = 4;        ///< Indicates a tile is on the RHS of a tile
const uint8_t TILE_LEFT_BOTTOM = 8;   ///< Indicates a tile is on the bottom of a tile
const uint8_t TILE_LEFT = 16;         ///< Indicates a tile is on the bottom of a tile
const uint8_t TILE_LEFT_TOP = 32;     ///< Indicates a tile is on the bottom of a tile
const uint8_t TILE_TOP = 64;          ///< Indicates a tile is on the bottom of a tile
const uint8_t TILE_RIGHT_TOP = 128;

class TileInfo {
private:
    friend class cereal::access;
    template < class Archive > void serialize( Archive& ar ) {
        ar( gridRow, gridCol, height, width, nullTile, filename, d8Tile );
    }

public:
    int gridRow, gridCol;  // tile coordinates in the Grid
    int height, width;     // number of rows and cols in the tile, may be different from the standard tile sizes for border tiles
    bool nullTile;
    std::string filename;
    int d8Tile;

public:
    TileInfo();
    ~TileInfo() = default;
};

#endif
