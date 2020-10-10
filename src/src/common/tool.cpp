
#include <paradem/gdal.h>
#include <paradem/grid.h>
#include <paradem/i_consumer.h>
#include <paradem/i_object_factory.h>
#include <paradem/i_producer.h>
#include <paradem/object_deleter.h>
#include <paradem/tool.h>

#include <mpi.h>

#include <algorithm>
#include <assert.h>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

bool readTXTInfo( std::string filePathTXT, std::vector< TileInfo >& tileInfos, GridInfo& gridInfo ) {
    std::vector< std::vector< std::string > > fgrid;
    // Open file
    std::ifstream fin_layout( filePathTXT );
    if ( !fin_layout.is_open() ) {
        std::cerr << "cant open the file" << std::endl;
    }
    if ( !fin_layout.good() )
        throw std::runtime_error( "Problem opening layout file '" + filePathTXT + "'" );
    while ( fin_layout ) {
        // Get a new line from the file.
        std::string row;
        if ( !getline( fin_layout, row ) )
            break;
        fgrid.emplace_back();
        // Put the data somewhere we can process it
        std::istringstream ss( row );
        // While there is unprocessed data
        while ( ss ) {
            std::string item;
            if ( !getline( ss, item, ',' ) )
                break;
            if ( !item.empty() ) {
                item.erase( 0, item.find_first_not_of( " " ) );
                item.erase( item.find_last_not_of( " " ) + 1 );
            }
            fgrid.back().push_back( item );
        }
    }
    if ( !fin_layout.eof() )
        throw std::runtime_error( "Failed to read the entire layout file!" );

    // Let's find the longest row
    auto max_col_size = fgrid.front().size();
    for ( const auto& row : fgrid )
        max_col_size = std::max( max_col_size, row.size() );

    for ( auto& row : fgrid )
        if ( row.size() == max_col_size - 1 )  // If the line was one short of max, assume it ends blank
            row.emplace_back();
        else if ( row.size() < max_col_size )  // The line was more than one short of max: uh oh
            throw std::runtime_error( "Not all of the rows in the layout file had the same number of columns!" );
    auto max_row_size = fgrid.size();
    gridInfo.gridHeight = max_row_size;
    gridInfo.gridWidth = max_col_size;
    tileInfos.resize( max_row_size * max_col_size );
    for ( int tileRow = 0; tileRow < max_row_size; tileRow++ ) {
        for ( int tileCol = 0; tileCol < max_col_size; tileCol++ ) {
            TileInfo tileInfo;
            if ( fgrid[ tileRow ][ tileCol ].size() == 0 ) {
                tileInfo.nullTile = true;
            }
            else {
                tileInfo.filename = fgrid[ tileRow ][ tileCol ];
                tileInfo.nullTile = false;
            }
            tileInfo.gridRow = tileRow;
            tileInfo.gridCol = tileCol;
            tileInfos[ tileRow * max_col_size + tileCol ] = tileInfo;
        }
    }

    for ( int tileRow = 0; tileRow < max_row_size; tileRow++ ) {
        for ( int tileCol = 0; tileCol < max_col_size; tileCol++ ) {
            if ( tileInfos[ tileRow * max_col_size + tileCol ].nullTile ) {
                if ( tileCol != max_col_size - 1 ) {
                    tileInfos[ tileRow * max_col_size + tileCol + 1 ].d8Tile |= TILE_LEFT;
                    if ( tileRow > 0 ) {
                        tileInfos[ ( tileRow - 1 ) * max_col_size + tileCol + 1 ].d8Tile |= TILE_LEFT_BOTTOM;
                    }
                    if ( tileRow < max_row_size - 1 ) {
                        tileInfos[ ( tileRow + 1 ) * max_col_size + tileCol + 1 ].d8Tile |= TILE_LEFT_TOP;
                    }
                }
                if ( tileRow != 0 ) {
                    tileInfos[ ( tileRow - 1 ) * max_col_size + tileCol ].d8Tile |= TILE_BOTTOM;
                    if ( tileCol > 0 ) {
                        tileInfos[ ( tileRow - 1 ) * max_col_size + tileCol - 1 ].d8Tile |= TILE_RIGHT_BOTTOM;
                    }
                }
                if ( tileCol != 0 ) {
                    tileInfos[ tileRow * max_col_size + tileCol - 1 ].d8Tile |= TILE_RIGHT;
                    if ( tileRow < max_row_size - 1 ) {
                        tileInfos[ ( tileRow + 1 ) * max_col_size + tileCol - 1 ].d8Tile |= TILE_RIGHT_TOP;
                    }
                }
                if ( tileRow != max_row_size - 1 ) {
                    tileInfos[ ( tileRow + 1 ) * max_col_size + tileCol ].d8Tile |= TILE_TOP;
                }
            }
        }
    }
}

bool generateTiles( const char* filePath, int tileHeight, int tileWidth, const char* outputFolder ) {
    std::string inputFilePath = filePath;
    std::string output = outputFolder;
    GDALAllRegister();
    GDALDataset* fin = ( GDALDataset* )GDALOpen( inputFilePath.c_str(), GA_ReadOnly );
    if ( fin == NULL )
        throw std::runtime_error( "Could not open file '" + inputFilePath + "' to get dimensions." );
    GDALRasterBand* band = fin->GetRasterBand( 1 );
    GDALDataType type = band->GetRasterDataType();
    // file type is assumed to be float
    int grandHeight = band->GetYSize();
    int grandWidth = band->GetXSize();
    std::vector< double > geotransform( 6 );
    fin->GetGeoTransform( &geotransform[ 0 ] );
    int height, width;
    int gridHeight = std::ceil( ( double )grandHeight / tileHeight );
    int gridWidth = std::ceil( ( double )grandWidth / tileWidth );
    for ( int tileRow = 0; tileRow < gridHeight; tileRow++ ) {
        for ( int tileCol = 0; tileCol < gridWidth; tileCol++ ) {
            std::string outputFileName = std::to_string( tileRow ) + "_" + std::to_string( tileCol ) + ".tif";
            std::string path = output + "/" + outputFileName;
            height = ( grandHeight - tileHeight * tileRow >= tileHeight ) ? tileHeight : ( grandHeight - tileHeight * tileRow );
            width = ( grandWidth - tileWidth * tileCol >= tileWidth ) ? tileWidth : ( grandWidth - tileWidth * tileCol );
            Raster< float > tile;
            if ( !tile.init( height, width ) ) {
                GDALClose( ( GDALDatasetH )fin );
                return false;
            }
            band->RasterIO( GF_Read, tileWidth * tileCol, tileHeight * tileRow, tile.getWidth(), tile.getHeight(), ( void* )&tile, tile.getWidth(), tile.getHeight(), type, 0, 0 );
            std::vector< double > tileGeotransform( geotransform );
            tileGeotransform[ 0 ] = geotransform[ 0 ] + tileWidth * tileCol * geotransform[ 1 ] + tileHeight * tileRow * geotransform[ 2 ];
            tileGeotransform[ 3 ] = geotransform[ 3 ] + tileHeight * tileRow * geotransform[ 5 ] + tileWidth * tileCol * geotransform[ 4 ];
            WriteGeoTIFF( path.data(), tile.getHeight(), tile.getWidth(), &tile, type, &tileGeotransform[ 0 ], nullptr, nullptr, nullptr, nullptr, tile.NoDataValue );
        }
    }
    GDALClose( ( GDALDatasetH )fin );

    std::string txtPath = output + "/" + "tileInfo.txt";
    std::ofstream fout;
    fout.open( txtPath );
    if ( fout.fail() ) {
        std::cerr << "Open " << txtPath << " error!" << std::endl;
        return false;
    }
    for ( int tileRow = 0; tileRow < gridHeight; tileRow++ ) {
        for ( int tileCol = 0; tileCol < gridWidth; tileCol++ ) {
            std::string outputFileName = std::to_string( tileRow ) + "_" + std::to_string( tileCol ) + ".tif";
            std::string path = output + "/" + outputFileName;
            fout << path << ",";
        }
        fout << std::endl;
    }
    return true;
}

bool mergeTiles( GridInfo& gridInfo, const char* outputFilePath ) {
    int grandHeight = gridInfo.grandHeight;
    int grandWidth = gridInfo.grandWidth;
    int gridHeight = gridInfo.gridHeight;
    int gridWidth = gridInfo.gridWidth;
    int tileHeight = gridInfo.tileHeight;
    int tileWidth = gridInfo.tileWidth;
    std::string inputFolder = gridInfo.outputFolder;
    std::cout << "mergeTiles's inputFolder : " << inputFolder << std::endl;
    Raster< float > tiles;
    if ( !tiles.init( grandHeight, grandWidth ) )
        return false;
    std::vector< double > geotransform( 6 );
    for ( int tileRow = 0; tileRow < gridHeight; tileRow++ ) {
        for ( int tileCol = 0; tileCol < gridWidth; tileCol++ ) {
            std::string fileName = inputFolder + "/" + std::to_string( tileRow ) + "_" + std::to_string( tileCol ) + "flowdir.tif";
            Raster< float > tile;
            if ( !readGeoTIFF( fileName.data(), GDALDataType::GDT_Int32, tile ) ) {
                std::cout << fileName << " not exist!" << std::endl;
                return false;
            }
            if ( tileRow == 0 && tileCol == 0 ) {
                for ( int i = 0; i < 6; i++ ) {
                    geotransform[ i ] = tile.geoTransforms->at( i );
                }
            }
            int height = tile.getHeight();
            int width = tile.getWidth();
            int startRow = tileHeight * tileRow;
            int startCol = tileWidth * tileCol;
            for ( int row = 0; row < height; row++ ) {
                for ( int col = 0; col < width; col++ ) {
                    tiles.at( startRow + row, startCol + col ) = tile.at( row, col );
                }
            }
        }
    }
    WriteGeoTIFF( outputFilePath, tiles.getHeight(), tiles.getWidth(), &tiles, GDALDataType::GDT_Int32, &geotransform[ 0 ], nullptr, nullptr, nullptr, nullptr, tiles.NoDataValue );
    return true;
}

// this file is yet to be tested
bool createDiffFile( const char* filePath1, const char* filePath2, const char* diffFilePath ) {
    Raster< float > dem1, dem2;
    if ( !readGeoTIFF( filePath1, GDALDataType::GDT_Float32, dem1 ) )
        return false;
    if ( !readGeoTIFF( filePath2, GDALDataType::GDT_Float32, dem2 ) )
        return false;
    Raster< float > diff = dem1 - dem2;
    WriteGeoTIFF( "d:\\temp\\filled.tif", diff.getHeight(), diff.getWidth(), &diff, GDALDataType::GDT_Float32, diff.getGeoTransformsPtr(), nullptr, nullptr, nullptr, nullptr, diff.NoDataValue );
    return true;
}

void processTileGrid( GridInfo& gridInfo, std::vector< TileInfo >& tileInfos, IObjectFactory* pIObjFactory ) {
    Grid< std::shared_ptr< IConsumer2Producer > > gridIConsumer2Producer;
    gridIConsumer2Producer.init( gridInfo.gridHeight, gridInfo.gridWidth );  //初始化，全部为空

    for ( int i = 0; i < tileInfos.size(); i++ )  // tileInfos.size()
    {
        TileInfo& tileInfo = tileInfos[ i ];
        std::shared_ptr< IConsumer2Producer > pIConsumer2Producer = pIObjFactory->createConsumer2Producer();
        std::shared_ptr< IConsumer > pIConsumer = pIObjFactory->createConsumer();
        std::cout << "th " << i << "is processing！" << std::endl;
        if ( tileInfos[ i ].nullTile ) {
            continue;
        }
        pIConsumer->processRound1( gridInfo, tileInfo, pIConsumer2Producer.get() );
        gridIConsumer2Producer.at( tileInfo.gridRow, tileInfo.gridCol ) = pIConsumer2Producer;
    }
    std::shared_ptr< IProducer > pIProducer = pIObjFactory->createProducer();
    pIProducer->process( tileInfos, gridIConsumer2Producer );

    for ( int i = 0; i < tileInfos.size(); i++ ) {
        TileInfo& tileInfo = tileInfos[ i ];
        if ( tileInfo.nullTile ) {
            continue;
        }
        std::shared_ptr< IProducer2Consumer > pIProducer2Consumer = pIProducer->toConsumer( tileInfo, gridIConsumer2Producer );
        std::shared_ptr< IConsumer > pIConsumer = pIObjFactory->createConsumer();

        pIConsumer->processRound2( gridInfo, tileInfo, pIProducer2Consumer.get() );
    }
}

bool readGridInfo( const char* filePath, GridInfo& gridInfo ) {
    std::ifstream infile;
    infile.open( filePath );
    if ( !infile.is_open() ) {
        std::cerr << "Open gridInfo error!" << std::endl;
        return false;
    }
    std::string s;
    std::vector< std::string > input( 9 );
    int k = 0;
    while ( getline( infile, s ) )
        input[ k++ ] = s;

    size_t pos;
    gridInfo.tileHeight = stod( input[ 0 ], &pos );
    gridInfo.tileWidth = stod( input[ 1 ], &pos );
    gridInfo.gridHeight = stod( input[ 2 ], &pos );
    gridInfo.gridWidth = stod( input[ 3 ], &pos );
    gridInfo.grandHeight = stod( input[ 4 ], &pos );
    gridInfo.grandWidth = stod( input[ 5 ], &pos );

    infile.close();
    return true;
}

void createTileInfoArray( GridInfo& gridInfo, std::vector< TileInfo >& tileInfos ) {
    int gridHeight = gridInfo.gridHeight;
    int gridWidth = gridInfo.gridWidth;
    tileInfos.resize( gridHeight * gridWidth );

    int grandHeight = gridInfo.grandHeight;
    int grandWidth = gridInfo.grandWidth;
    int tileHeight = gridInfo.tileHeight;
    int tileWidth = gridInfo.tileWidth;
    int height, width;
    for ( int tileRow = 0; tileRow < gridHeight; tileRow++ ) {
        for ( int tileCol = 0; tileCol < gridWidth; tileCol++ ) {
            height = ( grandHeight - tileHeight * tileRow >= tileHeight ) ? tileHeight : ( grandHeight - tileHeight * tileRow );
            width = ( grandWidth - tileWidth * tileCol >= tileWidth ) ? tileWidth : ( grandWidth - tileWidth * tileCol );
            TileInfo tileInfo;
            tileInfo.gridRow = tileRow;
            tileInfo.gridCol = tileCol;
            tileInfo.height = height;
            tileInfo.width = width;
            std::string path = gridInfo.inputFolder + "\\" + std::to_string( tileRow ) + "_" + std::to_string( tileCol ) + ".tif";
            const char* cPath = path.c_str();
            tileInfo.nullTile = false;
            tileInfo.filename = path;
            tileInfos[ tileRow * gridWidth + tileCol ] = tileInfo;
        }
    }
    for ( int tileRow = 0; tileRow < gridHeight; tileRow++ ) {
        for ( int tileCol = 0; tileCol < gridWidth; tileCol++ ) {
            if ( tileInfos[ tileRow * gridWidth + tileCol ].nullTile ) {
                if ( tileCol != gridWidth - 1 ) {
                    tileInfos[ tileRow * gridWidth + tileCol + 1 ].d8Tile |= TILE_LEFT;
                    if ( tileRow > 0 ) {
                        tileInfos[ ( tileRow - 1 ) * gridWidth + tileCol + 1 ].d8Tile |= TILE_LEFT_BOTTOM;
                    }
                    if ( tileRow < gridHeight - 1 ) {
                        tileInfos[ ( tileRow + 1 ) * gridWidth + tileCol + 1 ].d8Tile |= TILE_LEFT_TOP;
                    }
                }
                if ( tileRow != 0 ) {
                    tileInfos[ ( tileRow - 1 ) * gridWidth + tileCol ].d8Tile |= TILE_BOTTOM;
                    if ( tileCol > 0 ) {
                        tileInfos[ ( tileRow - 1 ) * gridWidth + tileCol - 1 ].d8Tile |= TILE_RIGHT_BOTTOM;
                    }
                }
                if ( tileCol != 0 ) {
                    tileInfos[ tileRow * gridWidth + tileCol - 1 ].d8Tile |= TILE_RIGHT;
                    if ( tileRow < gridHeight - 1 ) {
                        tileInfos[ ( tileRow + 1 ) * gridWidth + tileCol - 1 ].d8Tile |= TILE_RIGHT_TOP;
                    }
                }
                if ( tileRow != gridHeight - 1 ) {
                    tileInfos[ ( tileRow + 1 ) * gridWidth + tileCol ].d8Tile |= TILE_TOP;
                }
            }
        }
    }
}

/*---------create PerlinDEM---------*/
int randomi( int m, int n ) {
    int pos, dis;
    srand( ( unsigned )time( NULL ) );
    if ( m == n ) {
        return m;
    }
    else if ( m > n ) {
        pos = n;
        dis = m - n + 1;
        return rand() % dis + pos;
    }
    else {
        pos = m;
        dis = n - m + 1;
        return rand() % dis + pos;
    }
}

static void normalize2( float v[ 2 ] ) {
    float s;

    s = sqrt( v[ 0 ] * v[ 0 ] + v[ 1 ] * v[ 1 ] );
    v[ 0 ] = v[ 0 ] / s;
    v[ 1 ] = v[ 1 ] / s;
}

static void normalize3( float v[ 3 ] ) {
    float s;
    s = sqrt( v[ 0 ] * v[ 0 ] + v[ 1 ] * v[ 1 ] + v[ 2 ] * v[ 2 ] );
    v[ 0 ] = v[ 0 ] / s;
    v[ 1 ] = v[ 1 ] / s;
    v[ 2 ] = v[ 2 ] / s;
}

static void init( void ) {
    int i, j, k;

    for ( i = 0; i < B; i++ ) {
        p[ i ] = i;

        g1[ i ] = ( float )( ( rand() % ( B + B ) ) - B ) / B;

        for ( j = 0; j < 2; j++ )
            g2[ i ][ j ] = ( float )( ( rand() % ( B + B ) ) - B ) / B;
        normalize2( g2[ i ] );

        for ( j = 0; j < 3; j++ )
            g3[ i ][ j ] = ( float )( ( rand() % ( B + B ) ) - B ) / B;
        normalize3( g3[ i ] );
    }

    while ( --i ) {
        k = p[ i ];
        p[ i ] = p[ j = rand() % B ];
        p[ j ] = k;
    }

    for ( i = 0; i < B + 2; i++ ) {
        p[ B + i ] = p[ i ];
        g1[ B + i ] = g1[ i ];
        for ( j = 0; j < 2; j++ )
            g2[ B + i ][ j ] = g2[ i ][ j ];
        for ( j = 0; j < 3; j++ )
            g3[ B + i ][ j ] = g3[ i ][ j ];
    }
}

float noise2( float vec[ 2 ] ) {
    int bx0, bx1, by0, by1, b00, b10, b01, b11;
    float rx0, rx1, ry0, ry1, *q, sx, sy, a, b, t, u, v;
    register int i, j;

    if ( start ) {
        start = 0;
        init();
    }

    setup( 0, bx0, bx1, rx0, rx1 );
    setup( 1, by0, by1, ry0, ry1 );

    i = p[ bx0 ];
    j = p[ bx1 ];

    b00 = p[ i + by0 ];
    b10 = p[ j + by0 ];
    b01 = p[ i + by1 ];
    b11 = p[ j + by1 ];

    sx = s_curve( rx0 );
    sy = s_curve( ry0 );

#define at2( rx, ry ) ( rx * q[ 0 ] + ry * q[ 1 ] )

    q = g2[ b00 ];
    u = at2( rx0, ry0 );
    q = g2[ b10 ];
    v = at2( rx1, ry0 );
    a = lerp( sx, u, v );

    q = g2[ b01 ];
    u = at2( rx0, ry1 );
    q = g2[ b11 ];
    v = at2( rx1, ry1 );
    b = lerp( sx, u, v );

    return lerp( sy, a, b );
}

void InitPriorityQue( Raster< float >& dem, Flag& flag, PriorityQueue& priorityQueue ) {
    int width = dem.getWidth();
    int height = dem.getHeight();
    Node tmpNode;
    int iRow, iCol, row, col;

    std::queue< Node > depressionQue;

    // push border cells into the PQ
    for ( row = 0; row < height; row++ ) {
        for ( col = 0; col < width; col++ ) {
            if ( flag.IsProcessedDirect( row, col ) )
                continue;

            if ( dem.isNoData( row, col ) ) {
                flag.SetFlag( row, col );
                for ( int i = 0; i < 8; i++ ) {
                    iRow = dem.getRow( i, row );
                    iCol = dem.getCol( i, col );
                    if ( flag.IsProcessed( iRow, iCol ) )
                        continue;
                    if ( !dem.isNoData( iRow, iCol ) ) {
                        tmpNode.row = iRow;
                        tmpNode.col = iCol;
                        tmpNode.spill = dem.at( iRow, iCol );
                        priorityQueue.push( tmpNode );
                        flag.SetFlag( iRow, iCol );
                    }
                }
            }
            else {
                if ( row == 0 || row == height - 1 || col == 0 || col == width - 1 ) {
                    // on the DEM border
                    tmpNode.row = row;
                    tmpNode.col = col;
                    tmpNode.spill = dem.at( row, col );
                    priorityQueue.push( tmpNode );
                    flag.SetFlag( row, col );
                }
            }
        }
    }
}

void ProcessPit( Raster< float >& dem, Flag& flag, std::queue< Node >& depressionQue, std::queue< Node >& traceQueue, PriorityQueue& priorityQueue ) {
    int iRow, iCol, i;
    float iSpill;
    Node No;
    Node node;
    int width = dem.getWidth();
    int height = dem.getHeight();
    while ( !depressionQue.empty() ) {
        node = depressionQue.front();
        depressionQue.pop();
        for ( i = 0; i < 8; i++ ) {
            iRow = dem.getRow( i, node.row );
            iCol = dem.getCol( i, node.col );
            if ( flag.IsProcessedDirect( iRow, iCol ) )
                continue;
            iSpill = dem.at( iRow, iCol );
            if ( iSpill > node.spill ) {  // slope cell
                No.row = iRow;
                No.col = iCol;
                No.spill = iSpill;
                flag.SetFlag( iRow, iCol );
                traceQueue.push( No );
                continue;
            }
            // depression cell
            flag.SetFlag( iRow, iCol );
            dem.at( iRow, iCol ) = node.spill;
            No.row = iRow;
            No.col = iCol;
            No.spill = node.spill;
            depressionQue.push( No );
        }
    }
}

void ProcessTraceQue( Raster< float >& dem, Flag& flag, std::queue< Node >& traceQueue, PriorityQueue& priorityQueue ) {
    bool HaveSpillPathOrLowerSpillOutlet;
    int i, iRow, iCol;
    int k, kRow, kCol;
    int noderow, nodecol;
    Node No, node;
    std::queue< Node > potentialQueue;
    int indexThreshold = 2;  // index threshold, default to 2
    while ( !traceQueue.empty() ) {
        node = traceQueue.front();
        traceQueue.pop();
        noderow = node.row;
        nodecol = node.col;
        bool Mask[ 5 ][ 5 ] = { { false }, { false }, { false }, { false }, { false } };
        for ( i = 0; i < 8; i++ ) {
            iRow = dem.getRow( i, noderow );
            iCol = dem.getCol( i, nodecol );
            if ( flag.IsProcessedDirect( iRow, iCol ) )
                continue;
            if ( dem.at( iRow, iCol ) > node.spill ) {
                No.col = iCol;
                No.row = iRow;
                No.spill = dem.at( iRow, iCol );
                traceQueue.push( No );
                flag.SetFlag( iRow, iCol );
            }
            else {
                // initialize all masks as false
                HaveSpillPathOrLowerSpillOutlet = false;  // whether cell i has a spill path or a lower spill outlet than node if i is a depression cell
                for ( k = 0; k < 8; k++ ) {
                    kRow = dem.getRow( k, iRow );
                    kCol = dem.getCol( k, iCol );
                    if ( ( Mask[ kRow - noderow + 2 ][ kCol - nodecol + 2 ] ) || ( flag.IsProcessedDirect( kRow, kCol ) && dem.at( kRow, kCol ) < node.spill ) ) {
                        Mask[ iRow - noderow + 2 ][ iCol - nodecol + 2 ] = true;
                        HaveSpillPathOrLowerSpillOutlet = true;
                        break;
                    }
                }
                if ( !HaveSpillPathOrLowerSpillOutlet ) {
                    if ( i < indexThreshold )
                        potentialQueue.push( node );
                    else
                        priorityQueue.push( node );
                    break;  // make sure node is not pushed twice into PQ
                }
            }
        }  // end of for loop
    }

    while ( !potentialQueue.empty() ) {
        node = potentialQueue.front();
        potentialQueue.pop();
        noderow = node.row;
        nodecol = node.col;

        // first case
        for ( i = 0; i < 8; i++ ) {
            iRow = dem.getRow( i, noderow );
            iCol = dem.getCol( i, nodecol );
            if ( flag.IsProcessedDirect( iRow, iCol ) )
                continue;
            else {
                priorityQueue.push( node );
                break;
            }
        }
    }
}

void calculateStatistics( Raster< float >& dem, double* min, double* max, double* mean, double* stdDev ) {
    int width = dem.getWidth();
    int height = dem.getHeight();

    int validElements = 0;
    double minValue, maxValue;
    double sum = 0.0;
    double sumSqurVal = 0.0;
    for ( int row = 0; row < height; row++ ) {
        for ( int col = 0; col < width; col++ ) {
            if ( !dem.isNoData( row, col ) ) {
                double value = dem.at( row, col );

                if ( validElements == 0 ) {
                    minValue = maxValue = value;
                }
                validElements++;
                if ( minValue > value ) {
                    minValue = value;
                }
                if ( maxValue < value ) {
                    maxValue = value;
                }

                sum += value;
                sumSqurVal += ( value * value );
            }
        }
    }

    double meanValue = sum / validElements;
    double stdDevValue = sqrt( ( sumSqurVal / validElements ) - ( meanValue * meanValue ) );
    *min = minValue;
    *max = maxValue;
    *mean = meanValue;
    *stdDev = stdDevValue;
}

void createPerlinNoiseDEM( std::string outputFilePath, int height, int width ) {
    Raster< float > dem;
    if ( !dem.init( height, width ) ) {
        std::cout << "Failed to allocate memory correctly!" << std::endl;
        return;
    }
    float fre = ( float )randomi( 0, 512 ) / 10;
    for ( int x = 0; x < height; x++ ) {
        for ( int y = 0; y < width; y++ ) {
            float vec2[] = { x / fre, y / fre };
            dem.at( x, y ) = noise2( vec2 ) * 100;
        }
    }
       //save unfilling_DEM.
    double min0, max0, mean0, stdDev0;
    calculateStatistics( dem, &min0, &max0, &mean0, &stdDev0 );
    int length = outputFilePath.length();
    std::string path = outputFilePath.substr( 0, length - 4 ) + "_unfilling.tif";
    WriteGeoTIFF( path.data(), dem.getHeight(), dem.getWidth(), &dem, GDALDataType::GDT_Float32, nullptr, &min0, &max0, &mean0, &stdDev0, -9999 );
    readGeoTIFF( path.data(), GDALDataType::GDT_Float32, dem );
    Flag flag;
    if ( !flag.Init( width, height ) ) {
        std::cout << "Failed to allocate memory for depression-filling !\n" << std::endl;
    }
    PriorityQueue priorityQueue;
    int iRow, iCol, row, col;
    float iSpill, spill;
    InitPriorityQue( dem, flag, priorityQueue );
    std::queue< Node > traceQueue;
    std::queue< Node > depressionQue;
    while ( !priorityQueue.empty() ) {
        Node tmpNode = priorityQueue.top();
        priorityQueue.pop();
        row = tmpNode.row;
        col = tmpNode.col;
        spill = tmpNode.spill;
        for ( int i = 0; i < 8; i++ ) {
            iRow = dem.getRow( i, row );
            iCol = dem.getCol( i, col );
            if ( flag.IsProcessed( iRow, iCol ) )
                continue;
            iSpill = dem.at( iRow, iCol );
            if ( iSpill <= spill ) {
                // depression cell
                dem.at( iRow, iCol ) = spill;
                flag.SetFlag( iRow, iCol );
                tmpNode.row = iRow;
                tmpNode.col = iCol;
                tmpNode.spill = spill;
                depressionQue.push( tmpNode );
                ProcessPit( dem, flag, depressionQue, traceQueue, priorityQueue );
            }
            else {
                // slope cell
                flag.SetFlag( iRow, iCol );
                tmpNode.row = iRow;
                tmpNode.col = iCol;
                tmpNode.spill = iSpill;
                traceQueue.push( tmpNode );
            }
            ProcessTraceQue( dem, flag, traceQueue, priorityQueue );
        }
    }
    double min, max, mean, stdDev;
    calculateStatistics( dem, &min, &max, &mean, &stdDev );
    WriteGeoTIFF( outputFilePath.data(), dem.getHeight(), dem.getWidth(), &dem, GDALDataType::GDT_Float32, nullptr, &min, &max, &mean, &stdDev, -9999 );
}

/*--------sequential flow direction-----------*/
static int d8_FlowDir( Raster< float >& dem, const int row, const int col ) {
    float minimum_elevation = dem.at( row, col );
    int flowdir = -1;
    int height = dem.getHeight();
    int width = dem.getWidth();
    // border cells
    if ( row == 0 && col == 0 ) {
        return 5;
    }
    else if ( row == 0 && col == width - 1 ) {
        return 7;
    }
    else if ( row == height - 1 && col == width - 1 ) {
        return 1;
    }
    else if ( row == height - 1 && col == 0 ) {
        return 3;
    }
    else if ( row == 0 ) {
        return 6;
    }
    else if ( col == 0 ) {
        return 4;
    }
    else if ( row == height - 1 ) {
        return 2;
    }
    else if ( col == width - 1 ) {
        return 0;
    }
    for ( int n = 0; n < 8; n++ ) {
        int iRow = dem.getRow( n, row );
        int iCol = dem.getCol( n, col );
        if ( dem.at( iRow, iCol ) < minimum_elevation || dem.at( iRow, iCol ) == minimum_elevation && flowdir > -1 && flowdir % 2 == 1 && n % 2 == 0 ) {
            minimum_elevation = dem.at( iRow, iCol );
            flowdir = n;
        }
    }
    return flowdir;
}

static void find_flat_edges( std::deque< GridCell >& low_edges, std::deque< GridCell >& high_edges, Raster< int >& flowdirs, Raster< float >& dem ) {
    int height = flowdirs.getHeight();
    int width = flowdirs.getWidth();
    for ( int row = 0; row < height; row++ ) {
        for ( int col = 0; col < width; col++ ) {
            if ( flowdirs.isNoData( row, col ) )
                continue;
            for ( int n = 0; n <= 7; n++ ) {
                int nRow = dem.getRow( n, row );
                int nCol = dem.getCol( n, col );
                if ( !flowdirs.isInGrid( nRow, nCol ) )
                    continue;
                if ( flowdirs.isNoData( nRow, nCol ) )
                    continue;

                if ( flowdirs.at( row, col ) != -1 && flowdirs.at( nRow, nCol ) == -1 && dem.at( nRow, nCol ) == dem.at( row, col ) ) {
                    low_edges.push_back( GridCell( row, col ) );
                    break;
                }
                else if ( flowdirs.at( row, col ) == -1 && dem.at( row, col ) < dem.at( nRow, nCol ) ) {
                    high_edges.push_back( GridCell( row, col ) );
                    break;
                }
            }
        }
    }
}

static void label_this( int x0, int y0, const int label, Raster< int32_t >& labels, Raster< float >& dem ) {
    std::queue< GridCell > to_fill;
    to_fill.push( GridCell( x0, y0 ) );
    const float target_elevation = dem.at( x0, y0 );
    while ( to_fill.size() > 0 ) {
        GridCell c = to_fill.front();
        to_fill.pop();
        if ( dem.at( c.row, c.col ) != target_elevation )
            continue;
        if ( labels.at( c.row, c.col ) > 0 )
            continue;
        labels.at( c.row, c.col ) = label;
        for ( int n = 0; n <= 7; n++ )
            if ( labels.isInGrid( dem.getRow( n, c.row ), dem.getCol( n, c.col ) ) )
                to_fill.push( GridCell( dem.getRow( n, c.row ), dem.getCol( n, c.col ) ) );
    }
}

static void BuildAwayGradient( Raster< int >& flowdirs, Raster< int32_t >& flat_mask, std::deque< GridCell > edges, std::vector< int >& flat_height, Raster< int32_t >& labels ) {
    int loops = 1;
    GridCell iteration_marker( -1, -1 );
    Raster< int32_t > highmask;
    highmask.init( flat_mask.getHeight(), flat_mask.getWidth() );
    highmask.setAllValues( 0 );
    highmask.NoDataValue = -1;
    edges.push_back( iteration_marker );
    while ( edges.size() != 1 ) {
        int x = edges.front().row;
        int y = edges.front().col;
        edges.pop_front();
        if ( x == -1 ) {
            loops++;
            edges.push_back( iteration_marker );
            continue;
        }
        if ( flat_mask.at( x, y ) > 0 )
            continue;
        flat_mask.at( x, y ) = loops;
        highmask.at( x, y ) = loops;
        flat_height[ labels.at( x, y ) ] = loops;
        for ( int n = 0; n <= 7; n++ ) {
            int nx = flowdirs.getRow( n, x );
            int ny = flowdirs.getCol( n, y );
            if ( labels.isInGrid( nx, ny ) && labels.at( nx, ny ) == labels.at( x, y ) && flowdirs.at( nx, ny ) == -1 )
                edges.push_back( GridCell( nx, ny ) );
        }
    }
}

static void BuildTowardsCombinedGradient( Raster< int >& flowdirs, Raster< int32_t >& flat_mask, std::deque< GridCell > edges, std::vector< int >& flat_height, Raster< int32_t >& labels ) {
    Raster< int32_t > low_mask;
    low_mask.init( flat_mask.getHeight(), flat_mask.getWidth() );
    low_mask.setAllValues( 0 );
    low_mask.NoDataValue = -1;
    int loops = 1;
    GridCell iteration_marker( -1, -1 );
    for ( int col = 0; col < flat_mask.getWidth(); col++ )
        for ( int row = 0; row < flat_mask.getHeight(); row++ )
            flat_mask.at( row, col ) *= -1;
    edges.push_back( iteration_marker );
    while ( edges.size() != 1 ) {
        int x = edges.front().row;
        int y = edges.front().col;
        edges.pop_front();

        if ( x == -1 ) {
            loops++;
            edges.push_back( iteration_marker );
            continue;
        }
        if ( flat_mask.at( x, y ) > 0 )
            continue;
        if ( flat_mask.at( x, y ) != 0 ) {
            flat_mask.at( x, y ) = ( flat_height[ labels.at( x, y ) ] + flat_mask.at( x, y ) ) + 2 * loops;
            low_mask.at( x, y ) = 2 * loops;
        }
        else {
            flat_mask.at( x, y ) = 2 * loops;
            low_mask.at( x, y ) = 2 * loops;
        }
        for ( int n = 0; n <= 7; n++ ) {
            int nx = labels.getRow( n, x );
            int ny = labels.getCol( n, y );
            if ( labels.isInGrid( nx, ny ) && labels.at( nx, ny ) == labels.at( x, y ) && flowdirs.at( nx, ny ) == -1 )
                edges.push_back( GridCell( nx, ny ) );
        }
    }
}

void resolve_flats( Raster< float >& dem, Raster< int >& flowdirs, Raster< int32_t >& flatMask, Raster< int32_t >& labels ) {
    std::deque< GridCell > low_edges, high_edges;
    int height = dem.getHeight();
    int width = dem.getWidth();
    labels.init( height, width );
    labels.setAllValues( 0 );
    flatMask.init( height, width );
    flatMask.setAllValues( 0 );
    flatMask.NoDataValue = -1;
    find_flat_edges( low_edges, high_edges, flowdirs, dem );
    if ( low_edges.size() == 0 ) {
        if ( high_edges.size() > 0 ) {
            std::cout << "There were flats, but none of them had outlets!" << std::endl;
        }
        else {
            std::cout << "There were no flats!" << std::endl;
        }
        return;
    }
    int group_number = 1;
    for ( auto i = low_edges.begin(); i != low_edges.end(); ++i )
        if ( labels.at( i->row, i->col ) == 0 )
            label_this( i->row, i->col, group_number++, labels, dem );

    std::deque< GridCell > temp;
    for ( auto i = high_edges.begin(); i != high_edges.end(); ++i )
        if ( labels.at( i->row, i->col ) != 0 )
            temp.push_back( *i );
    if ( temp.size() < high_edges.size() )
        std::cout << "Not all flats have outlets; the DEM contains sinks/pits/depressions!";
    high_edges = temp;
    temp.clear();
    std::vector< int > flat_height( group_number );
    BuildAwayGradient( flowdirs, flatMask, high_edges, flat_height, labels );
    BuildTowardsCombinedGradient( flowdirs, flatMask, low_edges, flat_height, labels );
}

static int d8_masked_FlowDir( Raster< int32_t >& flat_mask, Raster< int32_t >& labels, const int row, const int col ) {
    int minimum_elevation = flat_mask.at( row, col );
    int flowdir = -1;
    for ( int n = 0; n <= 7; n++ ) {
        int nRow = labels.getRow( n, row );
        int nCol = labels.getCol( n, col );
        if ( labels.at( nRow, nCol ) != labels.at( row, col ) )
            continue;
        if ( flat_mask.at( nRow, nCol ) < minimum_elevation || ( flat_mask.at( nRow, nCol ) == minimum_elevation && flowdir > -1 && flowdir % 2 == 1 && n % 2 == 0 ) ) {
            minimum_elevation = flat_mask.at( nRow, nCol );
            flowdir = n;
        }
    }
    if ( flowdir == -1 ) {
        int anft = 0;
    }
    return flowdir;
}

void d8_flow_flats( Raster< int32_t >& flat_mask, Raster< int32_t >& labels, Raster< int >& flowdirs ) {
    for ( int row = 1; row < flat_mask.getHeight() - 1; row++ ) {
        for ( int col = 1; col < flat_mask.getWidth() - 1; col++ )
            if ( flat_mask.at( row, col ) == flat_mask.NoDataValue )
                continue;
            else if ( flowdirs.at( row, col ) == -1 )
                flowdirs.at( row, col ) = d8_masked_FlowDir( flat_mask, labels, row, col );
    }
}

void PerformAlgorithm( std::string filename, std::string outputname ) {
    Raster< float > dem;
    readGeoTIFF( filename.c_str(), GDALDataType::GDT_Float32, dem );
    dem.NoDataValue = -9999;
    Raster< int > flowdirs;
    flowdirs.init( dem.getHeight(), dem.getWidth() );
    flowdirs.setAllValues( -1 );
    flowdirs.NoDataValue = 255;
    for ( int row = 0; row < dem.getHeight(); row++ ) {
        for ( int col = 0; col < dem.getWidth(); col++ ) {
            if ( dem.isNoData( row, col ) ) {
                flowdirs.at( row, col ) = 255;
            }
            else {
                flowdirs.at( row, col ) = d8_FlowDir( dem, row, col );
            }
        }
    }
    // resolve flats
    Raster< int32_t > flatMask, labels;
    resolve_flats( dem, flowdirs, flatMask, labels );
    d8_flow_flats( flatMask, labels, flowdirs );
    // save flow directions
    std::vector< double > geotransform( 6 );
    for ( int i = 0; i < 6; i++ ) {
        geotransform[ i ] = dem.geoTransforms->at( i );
    }
    WriteGeoTIFF( outputname.data(), flowdirs.getHeight(), flowdirs.getWidth(), &flowdirs, GDALDataType::GDT_Int32, &geotransform[ 0 ], nullptr, nullptr, nullptr, nullptr, flowdirs.NoDataValue );
}

/*-----------------compare results------------*/
bool comPareResults( std::string seqTif, std::string paraTif ) {
    Raster< int > seqFlowdirections;
    Raster< int > paraFlowdirections;
    readflowTIFF( seqTif.c_str(), GDALDataType::GDT_Int32, seqFlowdirections );
    readflowTIFF( seqTif.c_str(), GDALDataType::GDT_Int32, paraFlowdirections );

    int h1 = seqFlowdirections.getHeight();
    int w1 = seqFlowdirections.getWidth();
    int h2 = paraFlowdirections.getHeight();
    int w2 = paraFlowdirections.getWidth();
    if ( h1 != h2 || w1 != w2 ) {
        std::cout << "-----------The sizes of the two pictures do not agree!---------------" << std::endl;
        return false;
    }
    for ( int row = 0; row < h1; row++ ) {
        for ( int col = 0; col < w1; col++ ) {
            if ( seqFlowdirections.at( row, col ) != paraFlowdirections.at( row, col ) ) {
                std::cout << "--------The value in " << row << "," << col << "do not agree!--------" << std::endl;
                return false;
            }
        }
    }
    std::cout << "The two pictures are the same!" << std::endl;
    return true;
}
