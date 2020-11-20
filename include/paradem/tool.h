
#ifndef PARADEM_TOOL_H
#define PARADEM_TOOL_H

#include "mpi.h"
#include <cereal/archives/binary.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/vector.hpp>
#include <paradem/GridCell.h>
#include <paradem/grid_info.h>
#include <paradem/i_object_factory.h>
#include <paradem/tile_info.h>

#include <assert.h>
#include <queue>
#include <vector>

bool readTXTInfo( std::string filePathTXT, std::vector< TileInfo >& tileInfos, GridInfo& gridInfo );
bool generateTiles( const char* filePath, int tileHeight, int tileWidth, const char* outputFolder );
bool mergeTiles( GridInfo& gridInfo, const char* outputFilePath );
bool createDiffFile( const char* filePath1, const char* filePath2, const char* diffFilePath );
bool readGridInfo( const char* filePath, GridInfo& gridInfo );
void createTileInfoArray( GridInfo& gridInfo, std::vector< TileInfo >& tileInfos );
void processTileGrid( GridInfo& gridInfo, std::vector< TileInfo >& tileInfos, IObjectFactory* pIObjFactory );

/*-------------create perling DEM---------*/
#define BO 0x100
#define BM 0xff
#define N 0x1000
#define NP 12 /* 2^N */
#define NM 0xfff
#define s_curve( t ) ( t * t * ( 3. - 2. * t ) )
#define lerp( t, a, b ) ( a + t * ( b - a ) )
#define setup( i, b0, b1, r0, r1 ) \
    t = vec[ i ] + N;              \
    b0 = ( ( int )t ) & BM;        \
    b1 = ( b0 + 1 ) & BM;          \
    r0 = t - ( int )t;             \
    r1 = r0 - 1.;

const unsigned char value[ 8 ] = { 128, 64, 32, 16, 8, 4, 2, 1 };

class Flag {
public:
    int width, height;
    unsigned char* flagArray;

public:
    ~Flag() {
        Free();
    }
    bool Init( int width, int height ) {
        flagArray = NULL;
        this->width = width;
        this->height = height;
        flagArray = new unsigned char[ ( width * height + 7 ) / 8 ]();
        return flagArray != NULL;
    }
    void Free() {
        delete[] flagArray;
        flagArray = NULL;
    }
    void SetFlag( int row, int col ) {
        int index = row * width + col;
        flagArray[ index / 8 ] |= value[ index % 8 ];
    }
    void SetFlags( int row, int col, Flag& flag ) {
        int index = row * width + col;
        int bIndex = index / 8;
        int bShift = index % 8;
        flagArray[ bIndex ] |= value[ bShift ];
        flag.flagArray[ bIndex ] |= value[ bShift ];
    }
    int IsProcessed( int row, int col ) {
        // if the cell is outside the DEM, is is regared as processed
        if ( row < 0 || row >= height || col < 0 || col >= width )
            return true;
        int index = row * width + col;
        return flagArray[ index / 8 ] & value[ index % 8 ];
    }
    int IsProcessedDirect( int row, int col ) {
        if ( row < 0 || row >= height || col < 0 || col >= width )
            return true;
        int index = row * width + col;
        return flagArray[ index / 8 ] & value[ index % 8 ];
    }
};

class Node {
public:
    int row;
    int col;
    float spill;
    int n;
    Node() {
        row = 0;
        col = 0;
        spill = -9999.0;
        n = -1;
    }

    struct Greater : public std::binary_function< Node, Node, bool > {
        bool operator()( const Node n1, const Node n2 ) const {
            return n1.spill > n2.spill;
        }
    };

    bool operator==( const Node& a ) {
        return ( this->col == a.col ) && ( this->row == a.row );
    }
    bool operator!=( const Node& a ) {
        return ( this->col != a.col ) || ( this->row != a.row );
    }
    bool operator<( const Node& a ) {
        return this->spill < a.spill;
    }
    bool operator>( const Node& a ) {
        return this->spill > a.spill;
    }
    bool operator>=( const Node& a ) {
        return this->spill >= a.spill;
    }
    bool operator<=( const Node& a ) {
        return this->spill <= a.spill;
    }
};

typedef std::vector< Node > NodeVector;
typedef std::priority_queue< Node, NodeVector, Node::Greater > PriorityQueue;

int randomi( int m, int n );
float noise2( float vec[ 2 ] );
void InitPriorityQue( Raster< float >& dem, Flag& flag, PriorityQueue& priorityQueue );
void ProcessPit( Raster< float >& dem, Flag& flag, std::queue< Node >& depressionQue, std::queue< Node >& traceQueue, PriorityQueue& priorityQueue );
void ProcessTraceQue( Raster< float >& dem, Flag& flag, std::queue< Node >& traceQueue, PriorityQueue& priorityQueue );
void calculateStatistics( Raster< float >& dem, double* min, double* max, double* mean, double* stdDev );
void createPerlinNoiseDEM( std::string outputFilePath, int height, int width );

/*--------sequential flow direction-----------*/

void resolve_flats( Raster< float >& dem, Raster< int >& flowdirs, Raster< int32_t >& flatMask, Raster< int32_t >& labels );
void d8_flow_flats( Raster< int32_t >& flat_mask, Raster< int32_t >& labels, Raster< int >& flowdirs );
void PerformAlgorithm( std::string filename, std::string outputname );

/*---------compare results-----------*/
bool comPareResults( std::string seqTif, std::string paraTif );

#endif