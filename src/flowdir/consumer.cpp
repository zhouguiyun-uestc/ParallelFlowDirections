
#include "consumer.h"
#include "consumer_2_producer.h"
#include <algorithm>
#include <iostream>
#include <paradem/gdal.h>
#include <paradem/raster.h>
#include <queue>

bool Consumer::processRound1( const GridInfo& gridInfo, TileInfo& tileInfo, IConsumer2Producer* pIC2P ) {
    NoFlat = false;
    FlatTile = false;
    Consumer2Producer* pC2P = ( Consumer2Producer* )pIC2P;
    pC2P->gridRow = tileInfo.gridRow;
    pC2P->gridCol = tileInfo.gridCol;
    gridRow = tileInfo.gridRow;
    gridCol = tileInfo.gridCol;
    gridHeight = gridInfo.gridHeight;
    gridWidth = gridInfo.gridWidth;
    std::string path = tileInfo.filename;
    timer_io.start();
    readGeoTIFF( path.c_str(), GDALDataType::GDT_Float32, dem );
    timer_io.stop();
    tileInfo.width = dem.getWidth();
    tileInfo.height = dem.getHeight();
    timer_calc.start();
    FlowAssign( flowdirs, tileInfo.d8Tile );
    LabelFlatArea( true, flowdirs, labels, flatHeight, highEdgeMask, lowEdgeMask, IdBounderyCells );  // label the flat area
    timer_calc.stop();
    pC2P->lowEdgeGraph = std::move( lowEdgeGraph );
    pC2P->highEdgeGraph = std::move( highEdgeGraph );
    pC2P->top_elevs = dem.getRowData( 0 );
    pC2P->bot_elevs = dem.getRowData( dem.getHeight() - 1 );
    pC2P->left_elevs = dem.getColData( 0 );
    pC2P->right_elevs = dem.getColData( dem.getWidth() - 1 );
    pC2P->nullTile = tileInfo.nullTile;
    int pstart, pend;
    pC2P->top_ID = std::vector< int >( IdBounderyCells.begin(), IdBounderyCells.begin() + tileInfo.width );
    pstart = tileInfo.width - 1;
    pend = +tileInfo.height - 1 + tileInfo.width;
    pC2P->right_ID = std::vector< int >( IdBounderyCells.begin() + pstart, IdBounderyCells.begin() + pend );
    pstart = tileInfo.width - 2 + tileInfo.height;
    pend = 2 * tileInfo.width - 2 + tileInfo.height;
    pC2P->bot_ID = std::vector< int >( IdBounderyCells.begin() + pstart, IdBounderyCells.begin() + pend );  //;
    std::reverse( pC2P->bot_ID.begin(), pC2P->bot_ID.end() );
    pstart = 2 * tileInfo.width - 3 + tileInfo.height;
    pC2P->left_ID = std::vector< int >( IdBounderyCells.begin() + pstart, IdBounderyCells.end() );
    pC2P->left_ID.push_back( IdBounderyCells.front() );
    std::reverse( pC2P->left_ID.begin(), pC2P->left_ID.end() );
    return true;
}

bool Consumer::processRound2( const GridInfo& gridInfo, const TileInfo& tileInfo, IProducer2Consumer* pIProducer2Consumer ) {
    timer_calc.start();
    gridRow = tileInfo.gridRow;
    gridCol = tileInfo.gridCol;
    gridHeight = gridInfo.gridHeight;
    gridWidth = gridInfo.gridWidth;
    std::string path = tileInfo.filename;
    timer_io.start();
    readGeoTIFF( path.c_str(), GDALDataType::GDT_Float32, dem );
    timer_io.stop();
    FlowAssign( flowdirs, tileInfo.d8Tile );
    LabelFlatArea( false, flowdirs, labels, flatHeight, highEdgeMask, lowEdgeMask, IdBounderyCells );
    if ( !NoFlat ) {
        RenewMask( tileInfo, pIProducer2Consumer, flowdirs, IdBounderyCells, highEdgeMask, lowEdgeMask, labels, flatHeight );
        D8FlowFlats( flowdirs, labels );
    }
    ModifyBoundaryFlowDir( tileInfo, pIProducer2Consumer, flowdirs, labels, IdBounderyCells, lowEdgeMask, flatHeight );
    timer_calc.stop();
    timer_io.start();
    SaveFlowDir( gridInfo, tileInfo, flowdirs );
    timer_io.stop();
    return true;
}

void Consumer::free() {
    delete this;
}

void Consumer::FlowAssign( Raster< int >& flowdirs, uint8_t d8Tile ) {
    flowdirs.init( dem.getHeight(), dem.getWidth() );
    flowdirs.setAllValues( -1 );
    flowdirs.NoDataValue = 255;
    int height = flowdirs.getHeight();
    int width = flowdirs.getWidth();
    for ( int row = 0; row < flowdirs.getHeight(); row++ ) {
        for ( int col = 0; col < flowdirs.getWidth(); col++ ) {
            if ( dem.isNoData( row, col ) ) {
                flowdirs.at( row, col ) = flowdirs.getNoDataValue();
            }
            else {
                flowdirs.at( row, col ) = D8FlowDir( row, col, height, width, d8Tile );
            }
        }
    }
}

int Consumer::D8FlowDir( const int& row, const int& col, const int& height, const int& width, uint8_t d8Tile ) {
    float minimum_dem = dem.at( row, col );
    int flowdir = -1;
    if ( gridRow == 0 || gridRow == gridHeight - 1 || gridCol == 0 || gridCol == gridWidth - 1 ) {
        if ( gridRow == 0 && gridCol == 0 && row == 0 && col == 0 ) {
            return 5;
        }
        else if ( gridRow == 0 && gridCol == gridWidth - 1 && row == 0 && col == width - 1 ) {
            return 7;
        }
        else if ( gridRow == gridHeight - 1 && gridCol == gridWidth - 1 && row == height - 1 && col == width - 1 ) {
            return 1;
        }
        else if ( gridRow == gridHeight - 1 && gridCol == 0 && row == height - 1 && col == 0 ) {
            return 3;
        }
        else if ( gridRow == 0 && row == 0 ) {
            return 6;
        }
        else if ( gridCol == gridWidth - 1 && col == width - 1 ) {
            return 0;
        }
        else if ( gridRow == gridHeight - 1 && row == height - 1 ) {
            return 2;
        }
        else if ( gridCol == 0 && col == 0 ) {
            return 4;
        }
    }
    if ( row == 0 || row == height - 1 || col == 0 || col == width - 1 ) {
        if ( row == 0 && col == 0 && ( d8Tile & TILE_LEFT_TOP ) ) {
            return 5;
        }
        else if ( row == 0 && col == width - 1 && ( d8Tile & TILE_RIGHT_TOP ) ) {
            return 7;
        }
        else if ( row == height - 1 && col == width - 1 && ( d8Tile & TILE_RIGHT_BOTTOM ) ) {
            return 1;
        }
        else if ( row == height - 1 && col == 0 && ( d8Tile & TILE_LEFT_BOTTOM ) ) {
            return 3;
        }
        else if ( row == 0 && ( d8Tile & TILE_TOP ) ) {
            return 6;
        }
        else if ( col == 0 && ( d8Tile & TILE_LEFT ) ) {
            return 4;
        }
        else if ( row == height - 1 && ( d8Tile & TILE_BOTTOM ) ) {
            return 2;
        }
        else if ( col == width - 1 && ( d8Tile & TILE_RIGHT ) ) {
            return 0;
        }
    }
    for ( int n = 0; n <= 7; n++ ) {
        if ( !dem.isInGrid( dem.getRow( n, row ), dem.getCol( n, col ) ) ) {
            continue;
        }
        if ( dem.at( dem.getRow( n, row ), dem.getCol( n, col ) ) < minimum_dem
             || dem.at( dem.getRow( n, row ), dem.getCol( n, col ) ) == minimum_dem && flowdir > -1 && flowdir % 2 == 1 && n % 2 == 0 ) {
            minimum_dem = dem.at( dem.getRow( n, row ), dem.getCol( n, col ) );
            flowdir = n;
        }
    }
    return flowdir;
}

void Consumer::LabelFlatArea( bool firstTime, Raster< int >& flowdirs, Raster< int >& labels, std::vector< int >& flatHeight, Raster< int >& highEdgeMask, Raster< int >& lowEdgeMask,
                              std::vector< int >& IdBounderyCells ) {
    std::deque< GridCell > low_edges;
    std::deque< GridCell > high_edges;
    std::deque< GridCell > uncertainEdges;
    labels.init( flowdirs.getHeight(), flowdirs.getWidth() );
    labels.setAllValues( 0 );

    highEdgeMask.init( flowdirs.getHeight(), flowdirs.getWidth() );
    highEdgeMask.setAllValues( 0 );

    lowEdgeMask.init( flowdirs.getHeight(), flowdirs.getWidth() );
    lowEdgeMask.setAllValues( 0 );
    FindFlatEdge( low_edges, high_edges, flowdirs, uncertainEdges );
    int group_number = 1;
    IdBounderyCells.resize( 2 * ( flowdirs.getHeight() - 1 ) + 2 * ( flowdirs.getWidth() - 1 ), 0 );
    if ( low_edges.size() == 0 && high_edges.size() == 0 && uncertainEdges.size() == 0 && flowdirs.at( 0, 0 ) == -1 ) {
        FlatTile = true;
        flatHeight.resize( group_number, 0 );
        BuildPartGraph( group_number, firstTime, IdBounderyCells, labels, lowEdgeMask, highEdgeMask, flowdirs, uncertainEdges );
    }
    else if ( low_edges.size() == 0 && high_edges.size() == 0 && uncertainEdges.size() == 0 ) {
        NoFlat = true;
        lowEdgeGraph.resize( 0 );
        highEdgeGraph.resize( 0 );
    }
    else {
        for ( auto i = low_edges.begin(); i != low_edges.end(); i++ ) {
            if ( labels.at( i->row, i->col ) == 0 ) {
                LabelCell( i->row, i->col, group_number++, labels );
            }
        }
        for ( auto i = high_edges.begin(); i != high_edges.end(); i++ ) {
            if ( labels.at( i->row, i->col ) == 0 ) {
                LabelCell( i->row, i->col, group_number++, labels );
            }
        }
        for ( auto i = uncertainEdges.begin(); i != uncertainEdges.end(); i++ ) {
            if ( labels.at( i->row, i->col ) == 0 ) {
                LabelCell( i->row, i->col, group_number++, labels );
            }
        }
        flatHeight.resize( group_number, 0 );
        BuildAwayGradient( high_edges, flowdirs, labels, highEdgeMask, flatHeight );
        BuildAwaySpillPnt( low_edges, flowdirs, labels, lowEdgeMask );
        BuildPartGraph( group_number, firstTime, IdBounderyCells, labels, lowEdgeMask, highEdgeMask, flowdirs, uncertainEdges );
    }
}

void Consumer::FindFlatEdge( std::deque< GridCell >& low_edges, std::deque< GridCell >& high_edges, Raster< int >& flowdirs, std::deque< GridCell >& uncertainEdges ) {
    int width = flowdirs.getWidth();
    int height = flowdirs.getHeight();
    for ( int col = 0; col < width; col++ ) {
        for ( int row = 0; row < height; row++ ) {
            if ( flowdirs.isNoData( row, col ) ) {
                continue;
            }

            for ( int n = 0; n <= 7; n++ ) {
                int nRow = flowdirs.getRow( n, row );
                int nCol = flowdirs.getCol( n, col );
                if ( !flowdirs.isInGrid( nRow, nCol ) || flowdirs.isNoData( nRow, nCol ) ) {
                    continue;
                }
                if ( flowdirs.at( row, col ) != -1 && flowdirs.at( nRow, nCol ) == -1 && dem.at( nRow, nCol ) == dem.at( row, col ) ) {
                    low_edges.push_back( GridCell( row, col ) );
                    break;
                }
                else if ( flowdirs.at( row, col ) == -1 && dem.at( row, col ) < dem.at( nRow, nCol ) ) {

                    if ( row != 0 && row != height - 1 && col != 0 && col != width - 1 ) {
                        high_edges.push_back( GridCell( row, col ) );
                    }
                    else {
                        uncertainEdges.push_back( GridCell( row, col ) );
                    }
                    break;
                }
            }
        }
    }
}

void Consumer::LabelCell( const int& row, const int& col, const int label, Raster< int >& labels ) {
    std::queue< GridCell > to_fill;
    to_fill.push( GridCell( row, col ) );
    const float target_dem = dem.at( row, col );
    while ( to_fill.size() > 0 ) {
        GridCell c = to_fill.front();
        to_fill.pop();
        if ( dem.at( c.row, c.col ) != target_dem || labels.at( c.row, c.col ) > 0 ) {
            continue;
        }
        labels.at( c.row, c.col ) = label;
        for ( int n = 0; n <= 7; n++ ) {
            if ( labels.isInGrid( dem.getRow( n, c.row ), dem.getCol( n, c.col ) ) ) {
                to_fill.push( GridCell( dem.getRow( n, c.row ), dem.getCol( n, c.col ) ) );
            }
        }
    }
}

void Consumer::BuildAwaySpillPnt( std::deque< GridCell > low_edges, Raster< int >& flowdirs, Raster< int >& labels, Raster< int >& lowEdgeMask ) {
    int loops = 1;
    GridCell iteration_maker( -1, -1 );
    low_edges.push_back( iteration_maker );
    while ( low_edges.size() != 1 ) {
        int row = low_edges.front().row;
        int col = low_edges.front().col;
        low_edges.pop_front();
        if ( row == -1 ) {
            loops++;
            low_edges.push_back( iteration_maker );
            continue;
        }
        if ( lowEdgeMask.at( row, col ) > 0 ) {
            continue;
        }
        lowEdgeMask.at( row, col ) = 2 * loops;
        for ( int n = 0; n <= 7; n++ ) {
            int nRow = flowdirs.getRow( n, row );
            int nCol = flowdirs.getCol( n, col );
            if ( !labels.isInGrid( nRow, nCol ) ) {
                continue;
            }
            if ( labels.at( nRow, nCol ) == labels.at( row, col ) && flowdirs.at( nRow, nCol ) == -1 ) {
                low_edges.push_back( GridCell( nRow, nCol ) );
            }
        }
    }
}

void Consumer::BuildAwayGradient( std::deque< GridCell > high_edges, Raster< int >& flowdirs, Raster< int >& labels, Raster< int >& highEdgeMask, std::vector< int >& flatHeight ) {
    int width = highEdgeMask.getWidth();
    int height = highEdgeMask.getHeight();
    int loops = 1;
    GridCell iteration_maker( -1, -1 );
    high_edges.push_back( iteration_maker );
    while ( high_edges.size() != 1 ) {
        int row = high_edges.front().row;
        int col = high_edges.front().col;
        high_edges.pop_front();
        if ( row == -1 ) {
            loops++;
            high_edges.push_back( iteration_maker );
            continue;
        }
        if ( highEdgeMask.at( row, col ) > 0 ) {
            continue;
        }
        highEdgeMask.at( row, col ) = loops;
        flatHeight[ labels.at( row, col ) ] = loops;
        for ( int n = 0; n <= 7; n++ ) {
            int nRow = labels.getRow( n, row );
            int nCol = labels.getCol( n, col );
            if ( !labels.isInGrid( nRow, nCol ) ) {
                continue;
            }
            if ( labels.at( nRow, nCol ) == labels.at( row, col ) && flowdirs.at( nRow, nCol ) == -1 && row != 0 && row != height - 1 && col != 0 && col != width - 1 ) {

                high_edges.push_back( GridCell( nRow, nCol ) );
            }
        }
    }
}

void Consumer::BuildPartGraph( const int groupNumber, bool firstTime, std::vector< int >& IdBounderyCells, Raster< int >& labels, Raster< int >& lowEdgeMask, Raster< int >& highEdgeMask,
                               Raster< int >& flowdirs, std::deque< GridCell >& uncertainEdges ) {
    int ID = 2;
    int width = labels.getWidth();
    int height = labels.getHeight();
    int pos;
    std::vector< std::vector< GridCell > > boundaryCell( groupNumber );
    std::vector< std::vector< int > > bulk_num( groupNumber, std::vector< int >( 1, 0 ) );
    int row = 0;
    for ( int col = 0; col < flowdirs.getWidth() - 2; col++ ) {
        if ( ( labels.at( row, col ) > 0 && flowdirs.at( row, col ) == -1 ) || FlatTile == true ) {
            if ( labels.at( row, col ) != labels.at( row, col + 1 ) || flowdirs.at( row, col + 1 ) > -1 ) {
                bulk_num[ labels.at( row, col ) ].push_back( boundaryCell[ labels.at( row, col ) ].size() + 1 );
            }
            boundaryCell[ labels.at( row, col ) ].push_back( GridCell( row, col ) );
            pos = xy2positionNum( row, col, width, height );
            IdBounderyCells[ pos ] = ID++;
        }
    }
    int col = flowdirs.getWidth() - 2;
    if ( ( labels.at( row, col ) > 0 && flowdirs.at( row, col ) == -1 ) || FlatTile == true ) {
        bulk_num[ labels.at( row, col ) ].push_back( boundaryCell[ labels.at( row, col ) ].size() + 1 );
        boundaryCell[ labels.at( row, col ) ].push_back( GridCell( row, col ) );
        pos = xy2positionNum( row, col, width, height );
        IdBounderyCells[ pos ] = ID++;
    }
    col = flowdirs.getWidth() - 1;
    for ( row = 0; row < flowdirs.getHeight() - 2; row++ ) {
        if ( ( labels.at( row, col ) > 0 && flowdirs.at( row, col ) == -1 ) || FlatTile == true ) {
            if ( labels.at( row, col ) != labels.at( row + 1, col ) || flowdirs.at( row + 1, col ) > -1 ) {
                bulk_num[ labels.at( row, col ) ].push_back( boundaryCell[ labels.at( row, col ) ].size() + 1 );
            }
            boundaryCell[ labels.at( row, col ) ].push_back( GridCell( row, col ) );
            pos = xy2positionNum( row, col, width, height );
            IdBounderyCells[ pos ] = ID++;
        }
    }
    row = flowdirs.getHeight() - 2;
    if ( ( labels.at( row, col ) > 0 && flowdirs.at( row, col ) == -1 ) || FlatTile == true ) {
        bulk_num[ labels.at( row, col ) ].push_back( boundaryCell[ labels.at( row, col ) ].size() + 1 );
        boundaryCell[ labels.at( row, col ) ].push_back( GridCell( row, col ) );
        pos = xy2positionNum( row, col, width, height );
        IdBounderyCells[ pos ] = ID++;
    }
    row = flowdirs.getHeight() - 1;
    for ( col = flowdirs.getWidth() - 1; col > 1; col-- ) {
        if ( ( labels.at( row, col ) > 0 && flowdirs.at( row, col ) == -1 ) || FlatTile == true ) {
            if ( labels.at( row, col - 1 ) != labels.at( row, col ) || flowdirs.at( row, col - 1 ) > -1 ) {
                bulk_num[ labels.at( row, col ) ].push_back( boundaryCell[ labels.at( row, col ) ].size() + 1 );
            }

            boundaryCell[ labels.at( row, col ) ].push_back( GridCell( row, col ) );
            pos = xy2positionNum( row, col, width, height );
            IdBounderyCells[ pos ] = ID++;
        }
    }
    col = 1;
    if ( ( labels.at( row, col ) > 0 && flowdirs.at( row, col ) == -1 ) || FlatTile == true ) {
        bulk_num[ labels.at( row, col ) ].push_back( boundaryCell[ labels.at( row, col ) ].size() + 1 );
        boundaryCell[ labels.at( row, col ) ].push_back( GridCell( row, col ) );
        pos = xy2positionNum( row, col, width, height );
        IdBounderyCells[ pos ] = ID++;
    }
    col = 0;
    for ( row = flowdirs.getHeight() - 1; row > 1; row-- ) {
        if ( ( labels.at( row, col ) > 0 && flowdirs.at( row, col ) == -1 ) || FlatTile == true ) {
            if ( row == 126 ) {
                int jdsfsd = 0;
            }

            if ( labels.at( row - 1, col ) != labels.at( row, col ) || flowdirs.at( row - 1, col ) > -1 ) {
                bulk_num[ labels.at( row, col ) ].push_back( boundaryCell[ labels.at( row, col ) ].size() + 1 );
            }
            boundaryCell[ labels.at( row, col ) ].push_back( GridCell( row, col ) );
            pos = xy2positionNum( row, col, width, height );
            IdBounderyCells[ pos ] = ID++;
        }
    }
    row = 1;
    if ( ( labels.at( row, col ) > 0 && flowdirs.at( row, col ) == -1 ) || FlatTile == true ) {
        bulk_num[ labels.at( row, col ) ].push_back( boundaryCell[ labels.at( row, col ) ].size() + 1 );
        boundaryCell[ labels.at( row, col ) ].push_back( GridCell( row, col ) );
        pos = xy2positionNum( row, col, width, height );
        IdBounderyCells[ pos ] = ID++;
    }

    if ( firstTime ) {
        lowEdgeGraph.resize( ID );
        highEdgeGraph.resize( ID );
        for ( int i = 0; i < groupNumber; i++ ) {
            if ( boundaryCell[ i ].size() == 0 ) {
                continue;
            }
            block2block( boundaryCell[ i ], bulk_num[ i ], IdBounderyCells, lowEdgeMask, highEdgeMask, labels );
        }
        while ( uncertainEdges.size() > 0 ) {
            GridCell q = uncertainEdges.front();
            uncertainEdges.pop_front();
            int pos = xy2positionNum( q.row, q.col, width, height );
            int my_iden = IdBounderyCells[ pos ];
            highEdgeGraph.at( 1 )[ my_iden ] = 0;
        }
    }
}

void Consumer::block2block( std::vector< GridCell >& boundarySet, std::vector< int >& bulk, std::vector< int >& IdBounderyCells, Raster< int >& lowEdgeMask, Raster< int >& highEdgeMask,
                            Raster< int >& labels ) {
    int width = labels.getWidth();
    int height = labels.getHeight();
    int number_bulk = bulk.size() - 1;
    for ( int i = 0; i < number_bulk; i++ ) {
        for ( int j = bulk[ i ]; j < bulk[ i + 1 ] - 1; j++ ) {
            GridCell source = boundarySet[ j ];
            int pos = xy2positionNum( source.row, source.col, width, height );
            int my_iden = IdBounderyCells[ pos ];
            if ( lowEdgeMask.at( source.row, source.col ) > 0 ) {
                lowEdgeGraph.at( 1 )[ my_iden ] = lowEdgeMask.at( source.row, source.col ) - 2;
            }
            if ( highEdgeGraph.at( 1 ).count( my_iden ) == 0 && highEdgeMask.at( source.row, source.col ) > 0 ) {
                highEdgeGraph.at( 1 )[ my_iden ] = highEdgeMask.at( source.row, source.col ) - 1;
            }
            lowEdgeGraph.at( my_iden )[ my_iden + 1 ] = 2;
            highEdgeGraph.at( my_iden )[ my_iden + 1 ] = 1;
        }
        GridCell source = boundarySet[ bulk[ i + 1 ] - 1 ];

        int pos = xy2positionNum( source.row, source.col, width, height );
        int my_iden = IdBounderyCells[ pos ];
        if ( lowEdgeMask.at( source.row, source.col ) > 0 ) {
            lowEdgeGraph.at( 1 )[ my_iden ] = lowEdgeMask.at( source.row, source.col ) - 2;
        }
        if ( highEdgeGraph.at( 1 ).count( my_iden ) == 0 && highEdgeMask.at( source.row, source.col ) > 0 ) {
            highEdgeGraph.at( 1 )[ my_iden ] = highEdgeMask.at( source.row, source.col ) - 1;
        }
    }

    if ( FlatTile && number_bulk == 4 ) {
        for ( int i = 0; i < number_bulk - 1; i++ ) {
            GridCell source = boundarySet[ bulk[ i + 1 ] - 1 ];
            int pos = xy2positionNum( source.row, source.col, width, height );
            int my_iden = IdBounderyCells[ pos ];
            auto isInsertSuccess = lowEdgeGraph.at( my_iden ).emplace( my_iden + 1, 2 );
            isInsertSuccess = highEdgeGraph.at( my_iden ).emplace( my_iden + 1, 1 );
            if ( bulk[ i + 2 ] - bulk[ i + 1 ] >= 2 ) {
                isInsertSuccess = highEdgeGraph.at( my_iden ).emplace( my_iden + 2, 1 );
                isInsertSuccess = lowEdgeGraph.at( my_iden ).emplace( my_iden + 2, 2 );
            }
            int j = 1;
            GridCell object = boundarySet[ bulk[ i + 2 ] - 1 ];
            pos = xy2positionNum( object.row, object.col, width, height );
            int ob_iden = IdBounderyCells[ pos ];
            my_iden--;
            j++;
            while ( my_iden > 1 && my_iden + 2 * j <= ob_iden ) {
                isInsertSuccess = highEdgeGraph.at( my_iden ).emplace( my_iden + 2 * j, j );
                isInsertSuccess = lowEdgeGraph.at( my_iden ).emplace( my_iden + 2 * j, 2 * j );
                j++;
                source = boundarySet[ bulk[ i + 1 ] - j ];
                pos = xy2positionNum( source.row, source.col, width, height );
                my_iden = IdBounderyCells[ pos ];
            }
        }
        GridCell source = boundarySet[ bulk[ 0 ] ];
        GridCell object = boundarySet[ bulk[ 4 ] - 1 ];
        int source_pos = xy2positionNum( source.row, source.col, width, height );
        int object_pos = xy2positionNum( object.row, object.col, width, height );
        int my_iden = IdBounderyCells[ source_pos ];
        int nEnd_iden = IdBounderyCells[ object_pos ];
        auto isInsertSuccess = highEdgeGraph.at( my_iden ).emplace( nEnd_iden, 1 );

        isInsertSuccess = lowEdgeGraph.at( my_iden ).emplace( nEnd_iden, 2 );
        if ( bulk[ 1 ] - bulk[ 0 ] >= 2 ) {
            isInsertSuccess = highEdgeGraph.at( my_iden + 1 ).emplace( nEnd_iden, 1 );
            isInsertSuccess = lowEdgeGraph.at( my_iden + 1 ).emplace( nEnd_iden, 2 );
        }
        int j = 2;
        object = boundarySet[ bulk[ 3 ] ];
        source = boundarySet[ bulk[ 1 ] - 1 ];
        int pos = xy2positionNum( object.row, object.col, width, height );
        int ob_iden = IdBounderyCells[ pos ];
        pos = xy2positionNum( source.row, source.col, width, height );
        int sour_iden = IdBounderyCells[ pos ];
        int n_iden = nEnd_iden;
        source = boundarySet[ bulk[ 0 ] + j ];
        pos = xy2positionNum( source.row, source.col, width, height );
        my_iden = IdBounderyCells[ pos ];
        n_iden = nEnd_iden - j + 1;
        while ( my_iden <= sour_iden && n_iden >= ob_iden ) {

            isInsertSuccess = highEdgeGraph.at( my_iden ).emplace( n_iden, j );
            isInsertSuccess = lowEdgeGraph.at( my_iden ).emplace( n_iden, 2 * j );
            j++;
            source = boundarySet[ bulk[ 0 ] + j ];
            pos = xy2positionNum( source.row, source.col, width, height );
            my_iden = IdBounderyCells[ pos ];
            n_iden = nEnd_iden - j + 1;
        }
        int i = 0;  // 1-3 2-4
        int row = 0, col = 0;
        int nRow = height - 1;
        while ( col < width - 1 ) {
            int minCol = std::max( 1, col - ( height - 1 ) );
            int maxCol = std::min( col + height - 1, width - 1 );
            int source_pos = xy2positionNum( row, col, width, height );
            int object_pos;
            int my_iden = IdBounderyCells[ source_pos ];
            int n_iden;
            for ( int nCol = minCol; nCol <= maxCol; nCol++ ) {
                object_pos = xy2positionNum( nRow, nCol, width, height );
                n_iden = IdBounderyCells[ object_pos ];
                isInsertSuccess = highEdgeGraph.at( my_iden ).emplace( n_iden, height - 1 );
                isInsertSuccess = lowEdgeGraph.at( my_iden ).emplace( n_iden, 2 * ( height - 1 ) );
            }
            col++;
        }
        i = 1;
        col = width - 1;
        int nCol = 0;
        row = 0;
        while ( row < height - 1 ) {
            int minRow = std::max( 1, row - ( width - 1 ) );
            int maxRow = std::min( height - 1, row + width - 1 );
            int source_pos = xy2positionNum( row, col, width, height );
            int object_pos;
            int my_iden = IdBounderyCells[ source_pos ];
            int n_iden;
            for ( int nRow = minRow; nRow <= maxRow; nRow++ ) {
                object_pos = xy2positionNum( nRow, nCol, width, height );
                n_iden = IdBounderyCells[ object_pos ];
                isInsertSuccess = lowEdgeGraph.at( my_iden ).emplace( n_iden, 2 * ( width - 1 ) );
            }
            row++;
        }
        return;
    }
    int maxNode_bulk = 0;
    int maxNode_num = bulk[ 1 ] - bulk[ 0 ];
    for ( int i = 1; i < number_bulk; i++ ) {
        if ( bulk[ i + 1 ] - bulk[ i ] > maxNode_num ) {
            maxNode_num = bulk[ i + 1 ] - bulk[ i ];
            maxNode_bulk = i;
        }
    }
    for ( int source_bulk = 0; source_bulk < number_bulk; source_bulk++ ) {
        if ( source_bulk == maxNode_bulk ) {
            continue;
        }
        int width = flowdirs.getWidth();
        int height = flowdirs.getHeight();
        int distancelen = width * 2 + height * 2 - 4;
        std::vector< std::vector< int32_t > > minimum_distances( distancelen, std::vector< int32_t >( distancelen, 0 ) );
        int cell_1 = 0;
        int pos_1 = xy2positionNum( boundarySet[ bulk[ source_bulk ] + cell_1 ].row, boundarySet[ bulk[ source_bulk ] + cell_1 ].col, width, height );
        bc2block( boundarySet[ bulk[ source_bulk ] + cell_1 ], minimum_distances[ pos_1 ], labels );
        int cell_n = bulk[ source_bulk + 1 ] - 1 - bulk[ source_bulk ];
        int pos_n = xy2positionNum( boundarySet[ bulk[ source_bulk ] + cell_n ].row, boundarySet[ bulk[ source_bulk ] + cell_n ].col, width, height );
        bc2block( boundarySet[ bulk[ source_bulk ] + cell_n ], minimum_distances[ pos_n ], labels );

        if ( source_bulk > maxNode_bulk ) {
            int object_bulk = maxNode_bulk;
            bool flag = true;
            GridCell qi = boundarySet[ bulk[ object_bulk ] ];
            GridCell zhong = boundarySet[ bulk[ object_bulk + 1 ] - 1 ];

            int pos_qi = xy2positionNum( qi.row, qi.col, width, height );
            int pos_zhong = xy2positionNum( zhong.row, zhong.col, width, height );

            do {

                int32_t jiange = minimum_distances[ pos_1 ][ pos_qi ] - minimum_distances[ pos_n ][ pos_qi ];

                if ( ( jiange == minimum_distances[ pos_1 ][ pos_zhong ] - minimum_distances[ pos_n ][ pos_zhong ] && abs( jiange ) == abs( cell_n - cell_1 ) ) || cell_1 + 1 == cell_n ) {
                    flag = false;
                }
                else {
                    if ( jiange <= 0 ) {
                        cell_1++;
                        pos_1++;
                        bc2block( boundarySet[ bulk[ source_bulk ] + cell_1 ], minimum_distances[ pos_1 ], labels );
                    }
                    else {
                        cell_n--;
                        pos_n--;
                        bc2block( boundarySet[ bulk[ source_bulk ] + cell_n ], minimum_distances[ pos_n ], labels );
                    }
                }
            } while ( flag );
        }

        for ( int object_bulk = source_bulk + 1; object_bulk < number_bulk; object_bulk++ ) {
            bool flag = true;
            GridCell qi = boundarySet[ bulk[ object_bulk ] ];
            GridCell zhong = boundarySet[ bulk[ object_bulk + 1 ] - 1 ];

            int pos_qi = xy2positionNum( qi.row, qi.col, width, height );
            int pos_zhong = xy2positionNum( zhong.row, zhong.col, width, height );

            do {

                int32_t jiange = minimum_distances[ pos_1 ][ pos_qi ] - minimum_distances[ pos_n ][ pos_qi ];

                if ( ( jiange == minimum_distances[ pos_1 ][ pos_zhong ] - minimum_distances[ pos_n ][ pos_zhong ] && abs( jiange ) == abs( cell_n - cell_1 ) ) || cell_n == cell_1 + 1 ) {
                    flag = false;
                }
                else {
                    if ( jiange <= 0 ) {
                        cell_1++;
                        pos_1++;
                        bc2block( boundarySet[ bulk[ source_bulk ] + cell_1 ], minimum_distances[ pos_1 ], labels );
                    }
                    else {
                        cell_n--;
                        pos_n--;
                        bc2block( boundarySet[ bulk[ source_bulk ] + cell_n ], minimum_distances[ pos_n ], labels );
                    }
                }
            } while ( flag );
        }

        if ( source_bulk > maxNode_bulk ) {
            int object_bulk = maxNode_bulk;
            ExternalGraph2( bulk, source_bulk, object_bulk, boundarySet, minimum_distances, cell_1, cell_n, IdBounderyCells, width, height );
        }

        for ( int object_bulk = source_bulk + 1; object_bulk < number_bulk; object_bulk++ ) {
            ExternalGraph2( bulk, source_bulk, object_bulk, boundarySet, minimum_distances, cell_1, cell_n, IdBounderyCells, width, height );
        }
    }
}

void Consumer::bc2block( GridCell source, std::vector< int32_t >& minimum_distance, Raster< int >& labels ) {

    //长度为边界长度大小
    int width = labels.getWidth();
    int height = labels.getHeight();

    std::deque< GridCell > this_edges;
    int sourceLabel = labels.at( source.row, source.col );
    GridCell iteration_marker( -1, -1 );
    int loops = 0;
    this_edges.push_back( source );
    this_edges.push_back( iteration_marker );

    Raster< int8_t > flag;
    flag.init( height, width );
    flag.setAllValues( 0 );

    while ( this_edges.size() != 1 ) {
        int row = this_edges.front().row;
        int col = this_edges.front().col;
        this_edges.pop_front();
        if ( row == -1 ) {
            loops++;
            this_edges.push_back( iteration_marker );
            continue;
        }
        if ( flag.at( row, col ) ) {
            continue;
        }
        flag.at( row, col ) = 1;
        if ( row == 0 || row == height - 1 || col == 0 || col == width - 1 )  //只有边界栅格才可以继续
        {
            int pos = xy2positionNum( row, col, width, height );
            if ( minimum_distance[ pos ] > 0 )
                continue;
            minimum_distance[ pos ] = loops;
        }
        for ( int n = 0; n <= 7; n++ ) {
            int nRow = labels.getRow( n, row );
            int nCol = labels.getCol( n, col );
            if ( labels.isInGrid( nRow, nCol ) && labels.at( nRow, nCol ) == labels.at( row, col ) )
                this_edges.push_back( GridCell( nRow, nCol ) );
        }
    }
}

void Consumer::block2blockminDis( std::deque< GridCell >& this_edges, std::vector< int >& minimum_distance, Raster< int >& labels ) {
    int height = labels.getHeight();
    int width = labels.getWidth();
    if ( this_edges.size() == 0 ) {
        return;
    }
    GridCell source = this_edges.front();
    int sourceLabel = labels.at( source.row, source.col );
    GridCell iteration_marker( -1, -1 );
    int loops = 0;
    this_edges.push_back( iteration_marker );

    Raster< int8_t > flag;
    flag.init( height, width );
    flag.setAllValues( 0 );

    while ( this_edges.size() != 1 ) {
        int row = this_edges.front().row;
        int col = this_edges.front().col;
        this_edges.pop_front();
        if ( row == -1 ) {
            loops++;
            this_edges.push_back( iteration_marker );
            continue;
        }
        if ( flag.at( row, col ) ) {
            continue;
        }
        flag.at( row, col ) = 1;
        if ( row == 0 || row == height - 1 || col == 0 || col == width - 1 )  //只有边界栅格才可以继续
        {
            int pos = xy2positionNum( row, col, width, height );
            if ( minimum_distance[ pos ] > 0 )
                continue;
            minimum_distance[ pos ] = loops;
        }
        for ( int n = 0; n <= 7; n++ ) {
            int nRow = labels.getRow( n, row );
            int nCol = labels.getCol( n, col );
            if ( labels.isInGrid( nRow, nCol ) && labels.at( nRow, nCol ) == labels.at( row, col ) )
                this_edges.push_back( GridCell( nRow, nCol ) );
        }
    }
}

void Consumer::ExternalGraph2( std::vector< int >& bulk, int& source_bulk, int& object_bulk, const std::vector< GridCell >& boundarySet, std::vector< std::vector< int32_t > >& minimum_distances,
                               const int cell_1, const int cell_n, std::vector< int >& IdBounderyCells, int width, int height ) {

    const int source_begin = 0;
    const int source_end = bulk[ source_bulk + 1 ] - 1 - bulk[ source_bulk ];
    int current_1 = 0;
    int current_n = source_end;

    int pos_1 = xy2positionNum( boundarySet[ bulk[ source_bulk ] + current_1 ].row, boundarySet[ bulk[ source_bulk ] + current_1 ].col, width, height );
    int pos_n = xy2positionNum( boundarySet[ bulk[ source_bulk ] + current_n ].row, boundarySet[ bulk[ source_bulk ] + current_n ].col, width, height );

    bool flag = true;
    GridCell object_qi = boundarySet[ bulk[ object_bulk ] ];
    int object_current_1 = bulk[ object_bulk ];
    GridCell object_zhong = boundarySet[ bulk[ object_bulk + 1 ] - 1 ];

    int pos_qi = xy2positionNum( object_qi.row, object_qi.col, width, height );
    int pos_zhong = xy2positionNum( object_zhong.row, object_zhong.col, width, height );

    int object_current_n = bulk[ object_bulk + 1 ] - 1;
    int pre_current_i;
    int after_current_i;
    int current_i, pos_i;

    do {
        int32_t jiange = minimum_distances[ pos_1 ][ pos_qi ] - minimum_distances[ pos_n ][ pos_qi ];
        int32_t jiange_zhong = minimum_distances[ pos_1 ][ pos_qi ] - minimum_distances[ pos_n ][ pos_zhong ];

        if ( jiange <= 0 ) {
            pre_current_i = current_1 - 1;
            after_current_i = current_1 + 1;
            current_i = current_1;
            pos_i = pos_1;
        }
        else {
            pre_current_i = current_n - 1;
            after_current_i = current_n + 1;
            current_i = current_n;
            pos_i = pos_n;
        }

        if ( ( jiange == minimum_distances[ pos_1 ][ pos_zhong ] - minimum_distances[ pos_n ][ pos_zhong ] && abs( jiange ) == abs( current_n - current_1 ) ) || current_n == current_1 ) {
            flag = false;
            bool flag_s1 = true;
            bool ifPreviousSource = InArray( pre_current_i, source_begin, source_end ) && ( current_i != current_n || current_1 == current_n );

            if ( ifPreviousSource ) {
                if ( minimum_distances[ pos_i ][ pos_qi ] > minimum_distances[ pos_i - 1 ][ pos_qi ] && minimum_distances[ pos_i ][ pos_zhong ] > minimum_distances[ pos_i - 1 ][ pos_zhong ] ) {
                    flag_s1 = false;
                }
            }
            bool flag_sn = true;
            bool ifAfterSource = InArray( after_current_i, source_begin, source_end ) && ( current_i != current_1 || current_1 == current_n ) && ( current_i != cell_1 );
            if ( ifAfterSource ) {
                if ( minimum_distances[ pos_i ][ pos_qi ] > minimum_distances[ pos_i + 1 ][ pos_qi ] && minimum_distances[ pos_i ][ pos_zhong ] > minimum_distances[ pos_i + 1 ][ pos_zhong ] ) {
                    flag_sn = false;
                }
            }
            if ( flag_s1 && flag_sn ) {
                int object_begin = bulk[ object_bulk ];
                int object_end = bulk[ object_bulk + 1 ] - 1;
                int object_current_1 = object_begin;
                int object_current_n = object_end;
                int pos_object_current_1 = xy2positionNum( boundarySet[ object_current_1 ].row, boundarySet[ object_current_1 ].col, width, height );
                int pos_object_current_n = xy2positionNum( boundarySet[ object_current_n ].row, boundarySet[ object_current_n ].col, width, height );
                bool object_flag = true;
                do {
                    int object_jiange = minimum_distances[ pos_i ][ pos_object_current_1 ] - minimum_distances[ pos_i ][ pos_object_current_n ];
                    int object_current_i = 0;
                    int pos_object_current_i;

                    if ( object_jiange <= 0 ) {
                        object_current_i = object_current_1;
                    }
                    else {
                        object_current_i = object_current_n;
                    }

                    pos_object_current_i = xy2positionNum( boundarySet[ object_current_i ].row, boundarySet[ object_current_i ].col, width, height );

                    if ( abs( object_jiange ) == abs( object_current_n - object_current_1 ) || object_current_n == object_current_1 ) {
                        object_flag = false;

                        bool flag_o1 = true;
                        if ( InArray( object_current_i - 1, object_begin, object_end ) ) {
                            if ( minimum_distances[ pos_i ][ pos_object_current_i ] > minimum_distances[ pos_i ][ pos_object_current_i - 1 ] ) {
                                flag_o1 = false;
                            }
                        }
                        bool flag_on = true;
                        if ( InArray( object_current_i + 1, object_begin, object_end ) ) {
                            if ( minimum_distances[ pos_i ][ pos_object_current_i ] > minimum_distances[ pos_i ][ pos_object_current_i + 1 ] ) {
                                flag_on = false;
                            }
                        }
                        if ( flag_o1 && flag_on ) {
                            bool ifPreviousObject = true;
                            if ( ifPreviousSource ) {
                                if ( minimum_distances[ pos_i ][ pos_object_current_i ] > minimum_distances[ pos_i - 1 ][ pos_object_current_i ] ) {
                                    ifPreviousObject = false;
                                }
                            }
                            bool ifAfterObject = true;
                            if ( ifAfterSource ) {
                                if ( minimum_distances[ pos_i ][ pos_object_current_i ] > minimum_distances[ pos_i + 1 ][ pos_object_current_i ] ) {
                                    ifAfterObject = false;
                                }
                            }
                            if ( ifPreviousObject && ifAfterObject ) {
                                int po = xy2positionNum( boundarySet[ bulk[ source_bulk ] + current_i ].row, boundarySet[ bulk[ source_bulk ] + current_i ].col, width, height );
                                int my_iden = IdBounderyCells[ po ];
                                po = xy2positionNum( boundarySet[ object_current_i ].row, boundarySet[ object_current_i ].col, width, height );
                                int n_iden = IdBounderyCells[ po ];
                                int va = minimum_distances[ pos_i ][ pos_object_current_i ];
                                if ( my_iden > n_iden ) {
                                    lowEdgeGraph.at( n_iden )[ my_iden ] = va * 2;
                                    highEdgeGraph.at( n_iden )[ my_iden ] = va;
                                }
                                else {
                                    lowEdgeGraph.at( my_iden )[ n_iden ] = va * 2;
                                    highEdgeGraph.at( my_iden )[ n_iden ] = va;
                                }
                            }
                        }
                    }
                    else {

                        bool flag_o1 = true;
                        if ( InArray( object_current_i - 1, object_begin, object_end ) ) {
                            if ( minimum_distances[ pos_i ][ pos_object_current_i ] > minimum_distances[ pos_i ][ pos_object_current_i - 1 ] ) {
                                flag_o1 = false;
                            }
                        }
                        bool flag_on = true;
                        if ( InArray( object_current_i + 1, object_begin, object_end ) ) {
                            if ( minimum_distances[ pos_i ][ pos_object_current_i ] > minimum_distances[ pos_i ][ pos_object_current_i + 1 ] ) {
                                flag_on = false;
                            }
                        }
                        if ( flag_o1 && flag_on ) {
                            bool ifPreviousObject = true;
                            if ( ifPreviousSource ) {
                                if ( minimum_distances[ pos_i ][ pos_object_current_i ] > minimum_distances[ pos_i - 1 ][ pos_object_current_i ] ) {
                                    ifPreviousObject = false;
                                }
                            }
                            bool ifAfterObject = true;
                            if ( ifAfterSource ) {
                                if ( minimum_distances[ pos_i ][ pos_object_current_i ] > minimum_distances[ pos_i + 1 ][ pos_object_current_i ] ) {
                                    ifAfterObject = false;
                                }
                            }
                            if ( ifPreviousObject && ifAfterObject ) {
                                int po = xy2positionNum( boundarySet[ bulk[ source_bulk ] + current_i ].row, boundarySet[ bulk[ source_bulk ] + current_i ].col, width, height );
                                int my_iden = IdBounderyCells[ po ];
                                po = xy2positionNum( boundarySet[ object_current_i ].row, boundarySet[ object_current_i ].col, width, height );
                                int n_iden = IdBounderyCells[ po ];
                                int va = minimum_distances[ pos_i ][ pos_object_current_i ];

                                if ( my_iden > n_iden ) {
                                    lowEdgeGraph.at( n_iden )[ my_iden ] = va * 2;
                                    highEdgeGraph.at( n_iden )[ my_iden ] = va;
                                }
                                else {
                                    lowEdgeGraph.at( my_iden )[ n_iden ] = va * 2;
                                    highEdgeGraph.at( my_iden )[ n_iden ] = va;
                                }
                            }
                        }

                        if ( object_current_i == object_current_1 ) {
                            object_current_1++;
                            pos_object_current_1++;
                        }
                        else {
                            object_current_n--;
                            pos_object_current_n--;
                        }
                    }
                } while ( object_flag );
            }
        }
        else {
            if ( current_i == cell_1 ) {

                current_i = current_n;
                pos_i = pos_n;
                pre_current_i = current_i - 1;
                after_current_i = current_i + 1;
            }
            else if ( current_i == cell_n ) {

                current_i = current_1;
                pre_current_i = current_i - 1;
                after_current_i = current_i + 1;
                pos_i = pos_1;
            }

            bool flag_s1 = true;
            bool ifPreviousSource = InArray( pre_current_i, source_begin, source_end );
            if ( ifPreviousSource ) {
                if ( minimum_distances[ pos_i ][ pos_qi ] > minimum_distances[ pos_i - 1 ][ pos_qi ] && minimum_distances[ pos_i ][ pos_zhong ] > minimum_distances[ pos_i - 1 ][ pos_zhong ] ) {
                    flag_s1 = false;
                }
            }
            bool flag_sn = true;
            bool ifAfterSource = InArray( after_current_i, source_begin, source_end );
            if ( ifAfterSource ) {
                if ( minimum_distances[ pos_i ][ pos_qi ] > minimum_distances[ pos_i + 1 ][ pos_qi ] && minimum_distances[ pos_i ][ pos_zhong ] > minimum_distances[ pos_i + 1 ][ pos_zhong ] ) {
                    flag_sn = false;
                }
            }
            if ( flag_s1 && flag_sn ) {
                int object_begin = bulk[ object_bulk ];
                int object_end = bulk[ object_bulk + 1 ] - 1;
                int object_current_1 = object_begin;
                int object_current_n = object_end;
                int pos_object_current_1 = xy2positionNum( boundarySet[ object_current_1 ].row, boundarySet[ object_current_1 ].col, width, height );
                int pos_object_current_n = xy2positionNum( boundarySet[ object_current_n ].row, boundarySet[ object_current_n ].col, width, height );
                bool object_flag = true;
                do {
                    int object_jiange = minimum_distances[ pos_i ][ pos_object_current_1 ] - minimum_distances[ pos_i ][ pos_object_current_n ];
                    int object_current_i = 0;
                    int pos_object_current_i;
                    if ( object_jiange <= 0 ) {
                        object_current_i = object_current_1;
                        pos_object_current_i = pos_object_current_1;
                    }
                    else {
                        object_current_i = object_current_n;
                        pos_object_current_i = pos_object_current_n;
                    }

                    if ( abs( object_jiange ) == abs( object_current_n - object_current_1 ) || object_current_n == object_current_1 ) {
                        object_flag = false;

                        bool flag_o1 = true;
                        if ( InArray( object_current_i - 1, object_begin, object_end ) ) {
                            if ( minimum_distances[ pos_i ][ pos_object_current_i ] > minimum_distances[ pos_i ][ pos_object_current_i - 1 ] ) {
                                flag_o1 = false;
                            }
                        }
                        bool flag_on = true;
                        if ( InArray( object_current_i + 1, object_begin, object_end ) ) {
                            if ( minimum_distances[ pos_i ][ pos_object_current_i ] > minimum_distances[ pos_i ][ pos_object_current_i + 1 ] ) {
                                flag_on = false;
                            }
                        }
                        if ( flag_o1 && flag_on ) {
                            bool ifPreviousObject = true;
                            if ( ifPreviousSource ) {
                                if ( minimum_distances[ pos_i ][ pos_object_current_i ] > minimum_distances[ pos_i - 1 ][ pos_object_current_i ] ) {
                                    ifPreviousObject = false;
                                }
                            }
                            bool ifAfterObject = true;
                            if ( ifAfterSource ) {
                                if ( minimum_distances[ pos_i ][ pos_object_current_i ] > minimum_distances[ pos_i + 1 ][ pos_object_current_i ] ) {
                                    ifAfterObject = false;
                                }
                            }
                            if ( ifPreviousObject && ifAfterObject ) {
                                int po = xy2positionNum( boundarySet[ bulk[ source_bulk ] + current_i ].row, boundarySet[ bulk[ source_bulk ] + current_i ].col, width, height );
                                int my_iden = IdBounderyCells[ po ];
                                po = xy2positionNum( boundarySet[ object_current_i ].row, boundarySet[ object_current_i ].col, width, height );
                                int n_iden = IdBounderyCells[ po ];
                                int va = minimum_distances[ pos_i ][ pos_object_current_i ];

                                if ( my_iden > n_iden ) {
                                    lowEdgeGraph.at( n_iden )[ my_iden ] = va * 2;
                                    highEdgeGraph.at( n_iden )[ my_iden ] = va;
                                }
                                else {
                                    lowEdgeGraph.at( my_iden )[ n_iden ] = va * 2;
                                    highEdgeGraph.at( my_iden )[ n_iden ] = va;
                                }
                            }
                        }
                    }
                    else {

                        bool flag_o1 = true;
                        if ( InArray( object_current_i - 1, object_begin, object_end ) ) {
                            if ( minimum_distances[ pos_i ][ pos_object_current_i ] > minimum_distances[ pos_i ][ pos_object_current_i - 1 ] ) {
                                flag_o1 = false;
                            }
                        }
                        bool flag_on = true;
                        if ( InArray( object_current_i + 1, object_begin, object_end ) ) {
                            if ( minimum_distances[ pos_i ][ pos_object_current_i ] > minimum_distances[ pos_i ][ pos_object_current_i + 1 ] ) {
                                flag_on = false;
                            }
                        }
                        if ( flag_o1 && flag_on ) {
                            bool ifPreviousObject = true;
                            if ( ifPreviousSource ) {
                                if ( minimum_distances[ pos_i ][ pos_object_current_i ] > minimum_distances[ pos_i - 1 ][ pos_object_current_i ] ) {
                                    ifPreviousObject = false;
                                }
                            }
                            bool ifAfterObject = true;
                            if ( ifAfterSource ) {
                                if ( minimum_distances[ pos_i ][ pos_object_current_i ] > minimum_distances[ pos_i + 1 ][ pos_object_current_i ] ) {
                                    ifAfterObject = false;
                                }
                            }
                            if ( ifPreviousObject && ifAfterObject ) {
                                int po = xy2positionNum( boundarySet[ bulk[ source_bulk ] + current_i ].row, boundarySet[ bulk[ source_bulk ] + current_i ].col, width, height );
                                int my_iden = IdBounderyCells[ po ];
                                po = xy2positionNum( boundarySet[ object_current_i ].row, boundarySet[ object_current_i ].col, width, height );
                                int n_iden = IdBounderyCells[ po ];
                                int va = minimum_distances[ pos_i ][ pos_object_current_i ];

                                if ( my_iden > n_iden ) {
                                    lowEdgeGraph.at( n_iden )[ my_iden ] = va * 2;
                                    highEdgeGraph.at( n_iden )[ my_iden ] = va;
                                }
                                else {
                                    lowEdgeGraph.at( my_iden )[ n_iden ] = va * 2;
                                    highEdgeGraph.at( my_iden )[ n_iden ] = va;
                                }
                            }
                        }

                        if ( object_current_i == object_current_1 ) {
                            object_current_1++;
                            pos_object_current_1++;
                        }
                        else {
                            object_current_n--;
                            pos_object_current_n--;
                        }
                    }
                } while ( object_flag );
            }

            if ( current_i == current_1 ) {
                current_1++;
                pos_1++;
            }
            else {
                current_n--;
                pos_n--;
            }
        }

    } while ( flag );
}

bool Consumer::InArray( int current, int qi, int zhong ) {
    if ( current < qi || current > zhong ) {
        return false;
    }
    return true;
}

void Consumer::RenewMask( const TileInfo& tile, IProducer2Consumer* Ip2c, Raster< int >& flowdirs, std::vector< int >& IdBounderyCells, Raster< int >& highEdgeMask, Raster< int >& lowEdgeMask,
                          Raster< int >& labels, std::vector< int >& flatHeight ) {
    Producer2Consumer* p2c = ( Producer2Consumer* )Ip2c;

    std::priority_queue< Mask, std::vector< Mask >, cmp > highSet;
    std::priority_queue< Mask, std::vector< Mask >, cmp > lowSet;
    int height = labels.getHeight();
    int width = labels.getWidth();

    for ( int row = 0; row < flowdirs.getHeight(); row = row + flowdirs.getHeight() - 1 ) {
        for ( int col = 0; col < flowdirs.getWidth(); col++ ) {
            int po = xy2positionNum( row, col, width, height );
            if ( IdBounderyCells[ po ] > 0 ) {
                lowEdgeMask.at( row, col ) = p2c->c_low[ IdBounderyCells[ po ] ];
                lowSet.push( { lowEdgeMask.at( row, col ), GridCell( row, col ) } );

                if ( lowEdgeMask.at( row, col ) != 2 ) {
                    highEdgeMask.at( row, col ) = p2c->c_high[ IdBounderyCells[ po ] ];
                    highSet.push( { highEdgeMask.at( row, col ), GridCell( row, col ) } );
                }

                if ( flatHeight[ labels.at( row, col ) ] < highEdgeMask.at( row, col ) ) {
                    flatHeight[ labels.at( row, col ) ] = highEdgeMask.at( row, col );
                }
            }
        }
    }

    for ( int32_t row = 1; row < flowdirs.getHeight() - 1; row++ ) {
        for ( int32_t col = 0; col < flowdirs.getWidth(); col = col + flowdirs.getWidth() - 1 ) {
            int po = xy2positionNum( row, col, width, height );
            if ( IdBounderyCells[ po ] > 0 ) {
                lowEdgeMask.at( row, col ) = p2c->c_low[ IdBounderyCells[ po ] ];
                lowSet.push( { lowEdgeMask.at( row, col ), GridCell( row, col ) } );
                if ( lowEdgeMask.at( row, col ) != 2 ) {
                    highEdgeMask.at( row, col ) = p2c->c_high[ IdBounderyCells[ po ] ];
                    highSet.push( { highEdgeMask.at( row, col ), GridCell( row, col ) } );
                }
                if ( flatHeight[ labels.at( row, col ) ] < highEdgeMask.at( row, col ) ) {
                    flatHeight[ labels.at( row, col ) ] = highEdgeMask.at( row, col );
                }
            }
        }
    }

    while ( highSet.size() > 0 ) {

        int col = highSet.top().cell.col;
        int row = highSet.top().cell.row;
        highSet.pop();
        for ( int n = 0; n <= 7; n++ ) {
            int nRow = labels.getRow( n, row );
            int nCol = labels.getCol( n, col );
            if ( !labels.isInGrid( nRow, nCol ) ) {
                continue;
            }
            if ( labels.at( nRow, nCol ) == labels.at( row, col ) && flowdirs.at( nRow, nCol ) == -1 && lowEdgeMask.at( nRow, nCol ) != 2
                 && ( highEdgeMask.at( nRow, nCol ) > highEdgeMask.at( row, col ) + 1 || highEdgeMask.at( nRow, nCol ) == 0 ) ) {
                highEdgeMask.at( nRow, nCol ) = highEdgeMask.at( row, col ) + 1;
                highSet.push( { highEdgeMask.at( nRow, nCol ), GridCell( nRow, nCol ) } );
                if ( flatHeight[ labels.at( row, col ) ] < highEdgeMask.at( nRow, nCol ) ) {
                    flatHeight[ labels.at( row, col ) ] = highEdgeMask.at( nRow, nCol );
                }
            }
        }
    }

    while ( lowSet.size() > 0 ) {
        int col = lowSet.top().cell.col;
        int row = lowSet.top().cell.row;
        lowSet.pop();

        for ( int n = 0; n <= 7; n++ ) {
            int nRow = labels.getRow( n, row );
            int nCol = labels.getCol( n, col );
            if ( !labels.isInGrid( nRow, nCol ) ) {
                continue;
            }
            if ( labels.at( nRow, nCol ) == labels.at( row, col ) && flowdirs.at( nRow, nCol ) == -1
                 && ( lowEdgeMask.at( nRow, nCol ) > lowEdgeMask.at( row, col ) + 2 || lowEdgeMask.at( nRow, nCol ) == 0 ) ) {
                lowEdgeMask.at( nRow, nCol ) = lowEdgeMask.at( row, col ) + 2;
                lowSet.push( { lowEdgeMask.at( nRow, nCol ), GridCell( nRow, nCol ) } );
            }
        }
    }
    flatMask.init( flowdirs.getHeight(), flowdirs.getWidth() );
    flatMask.setAllValues( 0 );

    for ( int32_t row = 0; row < flowdirs.getHeight(); row++ ) {
        for ( int32_t col = 0; col < flowdirs.getWidth(); col++ ) {
            if ( labels.at( row, col ) > 0 || FlatTile ) {
                if ( lowEdgeMask.at( row, col ) == 2 ) {
                    flatMask.at( row, col ) = lowEdgeMask.at( row, col );
                }
                else {
                    flatMask.at( row, col ) = lowEdgeMask.at( row, col ) - highEdgeMask.at( row, col ) + flatHeight[ labels.at( row, col ) ];
                }
            }
        }
    }
}

void Consumer::D8FlowFlats( Raster< int >& flowdirs, Raster< int >& labels ) {
    for ( int row = 1; row < flowdirs.getHeight() - 1; row++ ) {
        for ( int col = 1; col < flowdirs.getWidth() - 1; col++ ) {
            if ( flowdirs.at( row, col ) == -1 ) {
                flowdirs.at( row, col ) = D8MaskedFlowDir( row, col, labels );
            }
        }
    }
}

int8_t Consumer::D8MaskedFlowDir( const int row, const int col, Raster< int >& labels ) {
    int minimum_elevation = flatMask.at( row, col );
    int8_t flowdir = -1;
    for ( int n = 0; n <= 7; n++ ) {
        int nRow = labels.getRow( n, row );
        int nCol = labels.getCol( n, col );
        if ( labels.at( nRow, nCol ) != labels.at( row, col ) ) {
            continue;
        }
        if ( flatMask.at( nRow, nCol ) < minimum_elevation || flatMask.at( nRow, nCol ) == minimum_elevation && flowdir > -1 && flowdir % 2 == 1 && n % 2 == 0 ) {
            minimum_elevation = flatMask.at( nRow, nCol );
            flowdir = n;
        }
    }

    return flowdir;
}

void Consumer::ModifyBoundaryFlowDir( const TileInfo& tileInfo, IProducer2Consumer* Ip2c, Raster< int >& flowdirs, Raster< int >& labels, std::vector< int >& IdBounderyCells,
                                      Raster< int >& lowEdgeMask, std::vector< int >& flatHeight ) {
    int gridRow = tileInfo.gridRow;
    int gridCol = tileInfo.gridCol;
    int width = labels.getWidth();
    int height = labels.getHeight();
    Producer2Consumer* p2c = ( Producer2Consumer* )Ip2c;
    if ( gridCol != 0 )  // the tile is not on the left border
    {
        int col = 0;
        for ( int row = 1; row < flowdirs.getHeight() - 1; row++ )  // the cells belong to the left border
        {
            if ( flowdirs.at( row, col ) == 255 ) {
                continue;
            }
            if ( p2c->left.dem.size() == 0 ) {
                int n = -1;
                int nRow = 0;
                int nCol = 0;
                float minimum_elevation = dem.at( row, col );
                for ( int t = 0; t <= 2; t++ ) {
                    nCol = flowdirs.getCol( t, col );
                    nRow = flowdirs.getRow( t, row );
                    if ( minimum_elevation > dem.at( nRow, nCol ) || minimum_elevation == dem.at( nRow, nCol ) && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                        minimum_elevation = dem.at( nRow, nCol );
                        n = t;
                    }
                }
                if ( minimum_elevation > dem.NoDataValue || minimum_elevation == dem.NoDataValue && n > -1 && n % 2 == 1 ) {
                    minimum_elevation = dem.NoDataValue;
                    n = 4;
                }

                flowdirs.at( row, col ) = n;
                continue;
            }
            int po = xy2positionNum( row, col, width, height );
            if ( IdBounderyCells[ po ] == 0 || lowEdgeMask.at( row, col ) == 2 )  // find the minimum elevation
            {
                float minimum_elevation = dem.at( row, col );
                int n = -1;
                int nRow = 0;
                int nCol = 0;
                for ( int t = 0; t <= 2; t++ )  // 0-2
                {
                    nCol = flowdirs.getCol( t, col );
                    nRow = flowdirs.getRow( t, row );
                    if ( minimum_elevation > dem.at( nRow, nCol ) || minimum_elevation == dem.at( nRow, nCol ) && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                        minimum_elevation = dem.at( nRow, nCol );
                        n = t;
                    }
                }
                for ( int t = 3; t <= 5; t++ )  // 4-5
                {
                    nRow = flowdirs.getRow( t, row );
                    if ( minimum_elevation > p2c->left.dem[ nRow ] || ( minimum_elevation == p2c->left.dem[ nRow ] && n > -1 && n % 2 == 1 && t % 2 == 0 ) ) {
                        minimum_elevation = p2c->left.dem[ nRow ];
                        n = t;
                    }
                }
                for ( int t = 6; t <= 7; t++ )  // 6-7
                {
                    nCol = flowdirs.getCol( t, col );
                    nRow = flowdirs.getRow( t, row );
                    if ( minimum_elevation > dem.at( nRow, nCol ) || minimum_elevation == dem.at( nRow, nCol ) && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                        minimum_elevation = dem.at( nRow, nCol );
                        n = t;
                    }
                }
                flowdirs.at( row, col ) = n;
            }
            else {
                int minimum_elevation = flatMask.at( row, col );
                int n = -1;
                int nRow = 0;
                int nCol = 0;
                for ( int t = 0; t <= 2; t++ ) {
                    nRow = flowdirs.getRow( t, row );
                    nCol = flowdirs.getCol( t, col );
                    if ( labels.at( row, col ) != labels.at( nRow, nCol ) ) {
                        continue;
                    }
                    if ( minimum_elevation > flatMask.at( nRow, nCol ) || minimum_elevation == flatMask.at( nRow, nCol ) && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                        minimum_elevation = flatMask.at( nRow, nCol );
                        n = t;
                    }
                }
                for ( int t = 3; t <= 5; t++ ) {
                    nRow = flowdirs.getRow( t, row );

                    if ( dem.at( row, col ) == p2c->left.dem[ nRow ] ) {
                        int id = p2c->left.ID[ nRow ];
                        if ( id == 0 || p2c->left.low[ id ] == 2 ) {
                            if ( minimum_elevation > 2 || ( minimum_elevation == 2 && n > 0 && n % 2 == 1 && t % 2 == 0 ) ) {
                                minimum_elevation = 2;
                                n = t;
                            }
                        }
                        else {
                            if ( minimum_elevation > p2c->left.low[ id ] - p2c->left.high[ id ] + flatHeight[ labels.at( row, col ) ]
                                 || ( minimum_elevation == p2c->left.low[ id ] - p2c->left.high[ id ] + flatHeight[ labels.at( row, col ) ] && n > 0 && n % 2 == 1 && t % 2 == 0 ) ) {
                                minimum_elevation = p2c->left.low[ id ] - p2c->left.high[ id ] + flatHeight[ labels.at( row, col ) ];
                                n = t;
                            }
                        }
                    }
                }
                for ( int t = 6; t <= 7; t++ ) {
                    nRow = flowdirs.getRow( t, row );
                    nCol = flowdirs.getCol( t, col );
                    if ( labels.at( row, col ) != labels.at( nRow, nCol ) ) {
                        continue;
                    }
                    if ( minimum_elevation > flatMask.at( nRow, nCol ) || minimum_elevation == flatMask.at( nRow, nCol ) && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                        minimum_elevation = flatMask.at( nRow, nCol );
                        n = t;
                    }
                }
                flowdirs.at( row, col ) = n;
            }
        }
    }
    if ( gridRow != 0 )  // is not the top border
    {
        int row = 0;
        for ( int col = 1; col < flowdirs.getWidth() - 1; col++ )  // the cells are on the top border
        {
            if ( flowdirs.at( row, col ) == 255 ) {
                continue;
            }
            if ( p2c->top.dem.size() == 0 ) {
                float minimum_elevation = dem.at( row, col );
                int n = -1;
                int nCol = 0;
                int nRow = 0;
                for ( int t = 0; t <= 4; t++ )  // 0-4
                {
                    nCol = flowdirs.getCol( t, col );
                    nRow = flowdirs.getRow( t, row );
                    if ( minimum_elevation > dem.at( nRow, nCol ) || minimum_elevation == dem.at( nRow, nCol ) && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                        minimum_elevation = dem.at( nRow, nCol );
                        n = t;
                    }
                }

                if ( minimum_elevation > dem.NoDataValue || minimum_elevation == dem.NoDataValue && n > -1 && n % 2 == 1 ) {
                    n = 6;
                }
                flowdirs.at( row, col ) = n;
                continue;
            }
            int po = xy2positionNum( row, col, width, height );
            if ( IdBounderyCells[ po ] == 0 || lowEdgeMask.at( row, col ) == 2 ) {
                float minimum_elevation = dem.at( row, col );
                int n = -1;
                int nCol = 0;
                int nRow = 0;
                for ( int t = 0; t <= 4; t++ )  // 0-4
                {
                    nCol = flowdirs.getCol( t, col );
                    nRow = flowdirs.getRow( t, row );
                    if ( minimum_elevation > dem.at( nRow, nCol ) || minimum_elevation == dem.at( nRow, nCol ) && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                        minimum_elevation = dem.at( nRow, nCol );
                        n = t;
                    }
                }
                for ( int t = 5; t <= 7; t++ )  // 5-7
                {
                    nCol = flowdirs.getCol( t, col );
                    if ( minimum_elevation > p2c->top.dem[ nCol ] || minimum_elevation == p2c->top.dem[ nCol ] && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                        minimum_elevation = p2c->top.dem[ nCol ];
                        n = t;
                    }
                }

                flowdirs.at( row, col ) = n;
            }
            else {
                int minimum_elevation = flatMask.at( row, col );
                int n = -1;
                int nCol;
                int nRow;
                for ( int t = 0; t <= 4; t++ )  // 0-4
                {
                    nRow = flowdirs.getRow( t, row );
                    nCol = flowdirs.getCol( t, col );
                    if ( labels.at( row, col ) != labels.at( nRow, nCol ) ) {
                        continue;
                    }
                    if ( minimum_elevation > flatMask.at( nRow, nCol ) || minimum_elevation == flatMask.at( nRow, nCol ) && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                        minimum_elevation = flatMask.at( nRow, nCol );
                        n = t;
                    }
                }

                for ( int t = 5; t <= 7; t++ ) {
                    nCol = flowdirs.getCol( t, col );
                    int id = p2c->top.ID[ nCol ];
                    if ( dem.at( row, col ) == p2c->top.dem[ nCol ] ) {
                        if ( id > 0 && p2c->top.low[ id ] != 2 ) {
                            if ( minimum_elevation > p2c->top.low[ id ] - p2c->top.high[ id ] + flatHeight[ labels.at( row, col ) ]
                                 || minimum_elevation == p2c->top.low[ id ] - p2c->top.high[ id ] + flatHeight[ labels.at( row, col ) ] && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                                minimum_elevation = p2c->top.low[ id ] - p2c->top.high[ id ] + flatHeight[ labels.at( row, col ) ];
                                n = t;
                            }
                        }
                        else {
                            if ( minimum_elevation > 2 || minimum_elevation == 2 && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                                minimum_elevation = 2;
                                n = t;
                            }
                        }
                    }
                }

                flowdirs.at( row, col ) = n;
            }
        }
    }
    if ( gridCol != gridWidth - 1 )  // is not the right border
    {
        int col = flowdirs.getWidth() - 1;
        for ( int row = 1; row < flowdirs.getHeight() - 1; row++ ) {
            if ( flowdirs.at( row, col ) == 255 ) {
                continue;
            }
            if ( p2c->right.dem.size() == 0 ) {
                flowdirs.at( row, col ) = 0;
                continue;
            }
            int nRow, nCol, n = -1;
            int po = xy2positionNum( row, col, width, height );
            if ( IdBounderyCells[ po ] == 0 || lowEdgeMask.at( row, col ) == 2 ) {
                float minimum_elevation = dem.at( row, col );
                for ( int t = 0; t <= 1; t++ )  // 0-1
                {
                    nRow = flowdirs.getRow( t, row );
                    if ( minimum_elevation > p2c->right.dem[ nRow ] || minimum_elevation == p2c->right.dem[ nRow ] && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                        minimum_elevation = p2c->right.dem[ nRow ];
                        n = t;
                    }
                }
                for ( int t = 2; t <= 6; t++ )  // 2-6
                {
                    nCol = flowdirs.getCol( t, col );
                    nRow = flowdirs.getRow( t, row );
                    if ( minimum_elevation > dem.at( nRow, nCol ) || minimum_elevation == dem.at( nRow, nCol ) && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                        minimum_elevation = dem.at( nRow, nCol );
                        n = t;
                    }
                }

                nRow = flowdirs.getRow( 7, row );
                if ( minimum_elevation > p2c->right.dem[ nRow ] ) {
                    minimum_elevation = p2c->right.dem[ nRow ];
                    n = 7;
                }

                flowdirs.at( row, col ) = n;
            }
            else {
                int minimum_elevation = flatMask.at( row, col );
                for ( int t = 0; t <= 1; t++ )  // 0-1
                {
                    nRow = flowdirs.getRow( t, row );
                    int id = p2c->right.ID[ nRow ];
                    if ( dem.at( row, col ) == p2c->right.dem[ nRow ] ) {
                        if ( id > 0 && p2c->right.low[ id ] != 2 ) {
                            if ( minimum_elevation > p2c->right.low[ id ] - p2c->right.high[ id ] + flatHeight[ labels.at( row, col ) ]
                                 || minimum_elevation == p2c->right.low[ id ] - p2c->right.high[ id ] + flatHeight[ labels.at( row, col ) ] && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                                minimum_elevation = p2c->right.low[ id ] - p2c->right.high[ id ] + flatHeight[ labels.at( row, col ) ];
                                n = t;
                            }
                        }
                        else {
                            if ( minimum_elevation > 2 || minimum_elevation == 2 && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                                minimum_elevation = 2;
                                n = t;
                            }
                        }
                    }
                }
                for ( int t = 2; t <= 6; t++ )  // 2-6
                {
                    nCol = flowdirs.getCol( t, col );
                    nRow = flowdirs.getRow( t, row );
                    if ( labels.at( nRow, nCol ) != labels.at( row, col ) ) {
                        continue;
                    }
                    if ( minimum_elevation > flatMask.at( nRow, nCol ) || minimum_elevation == flatMask.at( nRow, nCol ) && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                        minimum_elevation = flatMask.at( nRow, nCol );
                        n = t;
                    }
                }

                int t = 7;
                nRow = flowdirs.getRow( t, row );

                if ( dem.at( row, col ) == p2c->right.dem[ nRow ] ) {
                    int id = p2c->right.ID[ nRow ];
                    if ( id > 0 && p2c->right.low[ id ] != 2 ) {
                        if ( minimum_elevation > p2c->right.low[ id ] - p2c->right.high[ id ] + flatHeight[ labels.at( row, col ) ] ) {
                            n = t;
                        }
                    }
                    else {
                        if ( minimum_elevation > 2 ) {
                            n = t;
                        }
                    }
                }

                flowdirs.at( row, col ) = n;
            }
        }
    }
    if ( gridRow != gridHeight - 1 ) {
        int row = flowdirs.getHeight() - 1;
        for ( int col = 1; col < flowdirs.getWidth() - 1; col++ ) {
            if ( flowdirs.at( row, col ) == 255 ) {
                continue;
            }

            if ( p2c->bottom.dem.size() == 0 ) {
                int n = -1;
                float minimum_elevation = dem.at( row, col );
                if ( minimum_elevation > dem.at( row, col + 1 ) ) {
                    n = 0;
                    minimum_elevation = dem.at( row, col + 1 );
                }
                if ( minimum_elevation > dem.NoDataValue || minimum_elevation == dem.NoDataValue && n > -1 && n % 2 == 1 ) {
                    n = 2;
                }
                flowdirs.at( row, col ) = n;
                continue;
            }
            int nCol, nRow, n = -1;
            int po = xy2positionNum( row, col, width, height );
            if ( IdBounderyCells[ po ] == 0 || lowEdgeMask.at( row, col ) == 2 ) {
                float minimum_elevation = dem.at( row, col );
                int t = 0;
                nCol = flowdirs.getCol( t, col );
                nRow = flowdirs.getRow( t, row );
                if ( minimum_elevation > dem.at( nRow, nCol ) )  // 0
                {
                    minimum_elevation = dem.at( nRow, nCol );
                    n = t;
                }
                for ( int t = 1; t <= 3; t++ ) {
                    nCol = flowdirs.getCol( t, col );
                    if ( minimum_elevation > p2c->bottom.dem[ nCol ] || minimum_elevation == p2c->bottom.dem[ nCol ] && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                        minimum_elevation = p2c->bottom.dem[ nCol ];
                        n = t;
                    }
                }
                for ( int t = 4; t <= 7; t++ )  // 4-7
                {
                    nCol = flowdirs.getCol( t, col );
                    nRow = flowdirs.getRow( t, row );
                    if ( minimum_elevation > dem.at( nRow, nCol ) || minimum_elevation == dem.at( nRow, nCol ) && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                        minimum_elevation = dem.at( nRow, nCol );
                        n = t;
                    }
                }

                flowdirs.at( row, col ) = n;
            }
            else {
                int minimum_elevation = flatMask.at( row, col );
                int t = 0;
                nCol = flowdirs.getCol( t, col );
                nRow = flowdirs.getRow( t, row );
                if ( labels.at( row, col ) == labels.at( nRow, nCol ) && minimum_elevation > flatMask.at( nRow, nCol ) )  // 0
                {
                    minimum_elevation = flatMask.at( nRow, nCol );
                    n = t;
                }
                for ( int t = 1; t <= 3; t++ ) {
                    nCol = flowdirs.getCol( t, col );
                    int id = p2c->bottom.ID[ nCol ];
                    if ( dem.at( row, col ) == p2c->bottom.dem[ nCol ] ) {
                        if ( id > 0 && p2c->bottom.low[ id ] != 2 ) {
                            if ( minimum_elevation > p2c->bottom.low[ id ] - p2c->bottom.high[ id ] + flatHeight[ labels.at( row, col ) ]
                                 || minimum_elevation == p2c->bottom.low[ id ] - p2c->bottom.high[ id ] + flatHeight[ labels.at( row, col ) ] && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                                minimum_elevation = p2c->bottom.low[ id ] - p2c->bottom.high[ id ] + flatHeight[ labels.at( row, col ) ];
                                n = t;
                            }
                        }
                        else {
                            if ( minimum_elevation > 2 || minimum_elevation == 2 && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                                minimum_elevation = 2;
                                n = t;
                            }
                        }
                    }
                }
                for ( int t = 4; t <= 7; t++ ) {
                    nCol = flowdirs.getCol( t, col );
                    nRow = flowdirs.getRow( t, row );
                    if ( labels.at( row, col ) != labels.at( nRow, nCol ) ) {
                        continue;
                    }
                    if ( minimum_elevation > flatMask.at( nRow, nCol ) || minimum_elevation == flatMask.at( nRow, nCol ) && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                        minimum_elevation = flatMask.at( nRow, nCol );
                        n = t;
                    }
                }

                flowdirs.at( row, col ) = n;
            }
        }
    }
    if ( gridCol != 0 && gridRow != 0 && flowdirs.at( 0, 0 ) != 255 ) {
        int col = 0, row = 0;
        int nCol, nRow, n = -1;
        int po = xy2positionNum( row, col, width, height );
        if ( p2c->left_top.dem.size() == 0 || p2c->top.dem.size() == 0 || p2c->left.dem.size() == 0 ) {
            float minimum_elevation = dem.at( row, col );
            for ( int t = 0; t <= 2; t++ )  // 0-2
            {
                nCol = flowdirs.getCol( t, col );
                nRow = flowdirs.getRow( t, row );
                if ( minimum_elevation > dem.at( nRow, nCol ) || minimum_elevation == dem.at( nRow, nCol ) && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                    minimum_elevation = dem.at( nRow, nCol );
                    n = t;
                }
            }
            if ( p2c->left.dem.size() == 0 ) {
                if ( minimum_elevation > dem.NoDataValue || minimum_elevation == dem.NoDataValue && n > -1 && n % 2 == 1 ) {
                    minimum_elevation = dem.NoDataValue;
                    n = 4;
                }
            }
            else {
                for ( int t = 3; t <= 4; t++ )  // 3-4
                {
                    nRow = dem.getRow( t, row );
                    if ( minimum_elevation > p2c->left.dem[ nRow ] || ( minimum_elevation == p2c->left.dem[ nRow ] && n > -1 && n % 2 == 1 && t % 2 == 0 ) ) {
                        minimum_elevation = p2c->left.dem[ nRow ];
                        n = t;
                    }
                }
            }

            if ( p2c->left_top.dem.size() == 0 ) {
                if ( minimum_elevation > dem.NoDataValue ) {
                    n = 5;
                    minimum_elevation = dem.NoDataValue;
                }
            }
            else {
                if ( minimum_elevation > p2c->left_top.dem[ 0 ] ) {
                    minimum_elevation = p2c->left_top.dem[ 0 ];
                    n = 5;
                }
            }

            if ( p2c->top.dem.size() == 0 ) {
                if ( minimum_elevation > dem.NoDataValue || minimum_elevation == dem.NoDataValue && n > -1 && n % 2 == 1 ) {
                    n = 6;
                }
            }
            else {
                for ( int t = 6; t <= 7; t++ )  // 3-4
                {
                    nCol = flowdirs.getCol( t, col );
                    if ( minimum_elevation > p2c->top.dem[ nCol ] || minimum_elevation == p2c->top.dem[ nCol ] && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                        minimum_elevation = p2c->top.dem[ nCol ];
                        n = t;
                    }
                }
            }
        }
        else if ( IdBounderyCells[ po ] == 0 || lowEdgeMask.at( row, col ) == 2 ) {
            float minimum_elevation = dem.at( row, col );
            for ( int t = 0; t <= 2; t++ )  // 0-2
            {
                nCol = flowdirs.getCol( t, col );
                nRow = flowdirs.getRow( t, row );
                if ( minimum_elevation > dem.at( nRow, nCol ) || minimum_elevation == dem.at( nRow, nCol ) && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                    minimum_elevation = dem.at( nRow, nCol );
                    n = t;
                }
            }
            for ( int t = 3; t <= 4; t++ )  // 3-4
            {
                nRow = dem.getRow( t, row );
                if ( minimum_elevation > p2c->left.dem[ nRow ] || ( minimum_elevation == p2c->left.dem[ nRow ] && n > -1 && n % 2 == 1 && t % 2 == 0 ) ) {
                    minimum_elevation = p2c->left.dem[ nRow ];
                    n = t;
                }
            }

            if ( minimum_elevation > p2c->left_top.dem[ 0 ] )

            {
                minimum_elevation = p2c->left_top.dem[ 0 ];
                n = 5;
            }
            for ( int t = 6; t <= 7; t++ )  // 3-4
            {
                nCol = flowdirs.getCol( t, col );
                if ( minimum_elevation > p2c->top.dem[ nCol ] || minimum_elevation == p2c->top.dem[ nCol ] && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                    minimum_elevation = p2c->top.dem[ nCol ];
                    n = t;
                }
            }

            flowdirs.at( row, col ) = n;
        }
        else {
            int minimum_elevation = flatMask.at( row, col );
            for ( int t = 0; t <= 2; t++ )  // 0-2
            {
                nCol = flowdirs.getCol( t, col );
                nRow = flowdirs.getRow( t, row );
                if ( labels.at( row, col ) != labels.at( nRow, nCol ) ) {
                    continue;
                }
                if ( minimum_elevation > flatMask.at( nRow, nCol ) || minimum_elevation == flatMask.at( nRow, nCol ) && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                    minimum_elevation = flatMask.at( nRow, nCol );
                    n = t;
                }
            }
            for ( int t = 3; t <= 4; t++ ) {
                nRow = flowdirs.getRow( t, row );
                if ( dem.at( row, col ) == p2c->left.dem[ nRow ] ) {
                    int id = p2c->left.ID[ nRow ];
                    if ( id > 0 && p2c->left.low[ id ] != 2 ) {
                        if ( minimum_elevation > p2c->left.low[ id ] - p2c->left.high[ id ] + flatHeight[ labels.at( row, col ) ]
                             || ( minimum_elevation == p2c->left.low[ id ] - p2c->left.high[ id ] + flatHeight[ labels.at( row, col ) ] && n > -1 && n % 2 == 1 && t % 2 == 0 ) ) {
                            minimum_elevation = p2c->left.low[ id ] - p2c->left.high[ id ] + flatHeight[ labels.at( row, col ) ];
                            n = t;
                        }
                    }
                    else {
                        if ( minimum_elevation > 2 || ( minimum_elevation == 2 && n > -1 && n % 2 == 1 && t % 2 == 0 ) ) {
                            minimum_elevation = 2;
                            n = t;
                        }
                    }
                }
            }
            if ( dem.at( row, col ) == p2c->left_top.dem[ 0 ] )  // 5
            {
                int id = p2c->left_top.ID[ 0 ];
                if ( id > 0 && p2c->left_top.low[ id ] != 2 ) {
                    if ( minimum_elevation > p2c->left_top.low[ id ] - p2c->left_top.high[ id ] + flatHeight[ labels.at( row, col ) ] ) {
                        minimum_elevation = p2c->left_top.low[ id ] - p2c->left_top.high[ id ] + flatHeight[ labels.at( row, col ) ];
                        n = 5;
                    }
                }
                else {
                    if ( minimum_elevation > 2 )  // 2
                    {
                        minimum_elevation = 2;
                        n = 5;
                    }
                }
            }

            for ( int t = 6; t <= 7; t++ )  // 6-7
            {
                nCol = flowdirs.getCol( t, col );
                if ( dem.at( row, col ) == p2c->top.dem[ nCol ] ) {
                    int id = p2c->top.ID[ nCol ];
                    if ( id > 0 && p2c->top.low[ id ] != 2 ) {
                        if ( minimum_elevation > p2c->top.low[ id ] - p2c->top.high[ id ] + flatHeight[ labels.at( row, col ) ]
                             || minimum_elevation == p2c->top.low[ id ] - p2c->top.high[ id ] + flatHeight[ labels.at( row, col ) ] && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                            minimum_elevation = p2c->top.low[ id ] - p2c->top.high[ id ] + flatHeight[ labels.at( row, col ) ];
                            n = t;
                        }
                    }
                    else {
                        if ( minimum_elevation > 2 || minimum_elevation == 2 && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                            minimum_elevation = 2;
                            n = t;
                        }
                    }
                }
            }
        }
        flowdirs.at( row, col ) = n;
    }
    int row = 0;
    int col = flowdirs.getWidth() - 1;
    if ( gridCol != gridWidth - 1 && gridRow != 0 && flowdirs.at( row, col ) != 255 )  // is not the right top border
    {
        int po = xy2positionNum( row, col, width, height );
        if ( p2c->right.dem.size() == 0 || p2c->top.dem.size() == 0 || p2c->right_top.dem.size() == 0 )  // right  || top
        {
            float minimum_elevation = dem.at( row, col );
            int n = -1, nRow, nCol;
            nRow = 0;
            nCol = col - 1;
            if ( p2c->right.dem.size() == 0 ) {
                minimum_elevation = dem.NoDataValue;
                n = 0;
            }
            else {
                for ( int t = 0; t <= 1; t++ )  // 0-1
                {
                    nRow = flowdirs.getRow( t, row );
                    if ( minimum_elevation > p2c->right.dem[ nRow ] || minimum_elevation == p2c->right.dem[ nRow ] && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                        minimum_elevation = p2c->right.dem[ nRow ];
                        n = t;
                    }
                }
            }
            for ( int t = 2; t <= 4; t++ ) {
                nCol = flowdirs.getCol( t, col );
                nRow = flowdirs.getRow( t, row );
                if ( minimum_elevation > dem.at( nRow, nCol ) || minimum_elevation == dem.at( nRow, nCol ) && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                    minimum_elevation = dem.at( nRow, nCol );
                    n = t;
                }
            }
            if ( p2c->top.dem.size() == 0 ) {
                if ( minimum_elevation > dem.NoDataValue || minimum_elevation == dem.NoDataValue && n > -1 && n % 2 == 1 ) {
                    minimum_elevation = dem.NoDataValue;
                    n = 6;
                }
            }
            else {
                for ( int t = 5; t <= 6; t++ )  // 5-6
                {
                    nCol = flowdirs.getCol( t, col );
                    if ( minimum_elevation > p2c->top.dem[ nCol ] || minimum_elevation == p2c->top.dem[ nCol ] && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                        minimum_elevation = p2c->top.dem[ nCol ];
                        n = t;
                    }
                }
            }
            if ( p2c->right_top.dem.size() == 0 ) {
                if ( minimum_elevation > dem.NoDataValue ) {
                    n = 7;
                }
            }
            else {
                if ( minimum_elevation > p2c->right_top.dem[ 0 ] )  // 7
                {
                    n = 7;
                }
            }
            flowdirs.at( row, col ) = n;
        }
        else if ( IdBounderyCells[ po ] == 0 || lowEdgeMask.at( row, col ) == 2 ) {
            float minimum_elevation = dem.at( row, col );
            int n = -1, nRow, nCol;
            nRow = 0;
            nCol = col - 1;

            for ( int t = 0; t <= 1; t++ )  // 0-1
            {
                nRow = flowdirs.getRow( t, row );
                if ( minimum_elevation > p2c->right.dem[ nRow ] || minimum_elevation == p2c->right.dem[ nRow ] && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                    minimum_elevation = p2c->right.dem[ nRow ];
                    n = t;
                }
            }
            for ( int t = 2; t <= 4; t++ ) {
                nCol = flowdirs.getCol( t, col );
                nRow = flowdirs.getRow( t, row );
                if ( minimum_elevation > dem.at( nRow, nCol ) || minimum_elevation == dem.at( nRow, nCol ) && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                    minimum_elevation = dem.at( nRow, nCol );
                    n = t;
                }
            }
            for ( int t = 5; t <= 6; t++ )  // 5-6
            {
                nCol = flowdirs.getCol( t, col );
                if ( minimum_elevation > p2c->top.dem[ nCol ] || minimum_elevation == p2c->top.dem[ nCol ] && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                    minimum_elevation = p2c->top.dem[ nCol ];
                    n = t;
                }
            }
            if ( minimum_elevation > p2c->right_top.dem[ 0 ] )  // 7
            {
                n = 7;
            }

            flowdirs.at( row, col ) = n;
        }
        else {
            int minimum_elevation = flatMask.at( row, col );
            int n = -1, nRow, nCol;

            for ( int t = 0; t <= 1; t++ )  // 0-1
            {

                nRow = flowdirs.getRow( t, row );
                if ( dem.at( row, col ) != p2c->right.dem[ nRow ] ) {
                    continue;
                }
                int id = p2c->right.ID[ nRow ];
                if ( id > 0 && p2c->right.low[ id ] != 2 ) {
                    if ( minimum_elevation > p2c->right.low[ id ] - p2c->right.high[ id ] + flatHeight[ labels.at( row, col ) ]
                         || minimum_elevation == p2c->right.low[ id ] - p2c->right.high[ id ] + flatHeight[ labels.at( row, col ) ] && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                        minimum_elevation = p2c->right.low[ id ] - p2c->right.high[ id ] + flatHeight[ labels.at( row, col ) ];
                        n = t;
                    }
                }
                else {
                    if ( minimum_elevation > 2 || minimum_elevation == 2 && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                        minimum_elevation = 2;
                        n = t;
                    }
                }
            }
            for ( int t = 2; t <= 4; t++ ) {
                nCol = flowdirs.getCol( t, col );
                nRow = flowdirs.getRow( t, row );
                if ( labels.at( row, col ) != labels.at( nRow, nCol ) ) {
                    continue;
                }
                if ( minimum_elevation > flatMask.at( nRow, nCol ) || minimum_elevation == flatMask.at( nRow, nCol ) && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                    minimum_elevation = flatMask.at( nRow, nCol );
                    n = t;
                }
            }

            for ( int t = 5; t <= 6; t++ )  // 5-6
            {
                nCol = flowdirs.getCol( t, col );
                if ( dem.at( row, col ) != p2c->top.dem[ nCol ] ) {
                    continue;
                }
                int id = p2c->top.ID[ nCol ];
                if ( id > 0 && p2c->top.low[ id ] != 2 ) {
                    if ( minimum_elevation > p2c->top.low[ id ] - p2c->top.high[ id ] + flatHeight[ labels.at( row, col ) ]
                         || minimum_elevation == p2c->top.low[ id ] - p2c->top.high[ id ] + flatHeight[ labels.at( row, col ) ] && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                        minimum_elevation = p2c->top.low[ id ] - p2c->top.high[ id ] + flatHeight[ labels.at( row, col ) ];
                        n = t;
                    }
                }
                else {
                    if ( minimum_elevation > 2 || minimum_elevation == 2 && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                        minimum_elevation = 2;
                        n = t;
                    }
                }
            }
            int id = p2c->right_top.ID[ 0 ];
            if ( dem.at( row, col ) == p2c->right_top.dem[ 0 ] ) {
                if ( id > 0 && p2c->right_top.low[ id ] != 2 ) {
                    if ( minimum_elevation > p2c->right_top.low[ id ] - p2c->right_top.high[ id ] + flatHeight[ labels.at( row, col ) ] )  // 7
                    {

                        n = 7;
                    }
                }
                else {
                    if ( minimum_elevation > 2 ) {

                        n = 7;
                    }
                }
            }
            flowdirs.at( row, col ) = n;
        }
    }
    col = flowdirs.getWidth() - 1;
    row = flowdirs.getHeight() - 1;
    if ( gridRow != gridHeight - 1 && gridCol != gridWidth - 1 && flowdirs.at( row, col ) != 255 )  // is not on the right bottom
    {
        int n = -1, nCol, nRow;
        int po = xy2positionNum( row, col, width, height );
        if ( p2c->right.dem.size() == 0 || p2c->bottom_right.dem.size() == 0 || p2c->bottom.dem.size() == 0 )  // right || bottom
        {
            float minimun_elevation = dem.at( row, col );
            if ( p2c->right.dem.size() == 0 ) {
                minimun_elevation = dem.NoDataValue;
                n = 0;
            }
            else {
                int t = 0;
                nRow = flowdirs.getRow( t, row );
                if ( minimun_elevation > p2c->right.dem[ nRow ] ) {
                    minimun_elevation = p2c->right.dem[ nRow ];
                    n = t;
                }
            }
            if ( p2c->bottom_right.dem.size() == 0 ) {
                if ( minimun_elevation > dem.NoDataValue ) {
                    minimun_elevation = dem.NoDataValue;
                    n = 1;
                }
            }
            else {
                if ( minimun_elevation > p2c->bottom_right.dem[ 0 ] ) {
                    minimun_elevation = p2c->bottom_right.dem[ 0 ];
                    n = 1;
                }
            }
            if ( p2c->bottom.dem.size() == 0 )  //下边的nullTile
            {
                if ( minimun_elevation > dem.NoDataValue || minimun_elevation == dem.NoDataValue && n > -1 && n % 2 == 1 ) {
                    minimun_elevation = dem.NoDataValue;
                    n = 2;
                }
            }
            else {
                for ( int t = 2; t <= 3; t++ ) {
                    nCol = flowdirs.getCol( t, col );
                    if ( minimun_elevation > p2c->bottom.dem[ nCol ] || minimun_elevation == p2c->bottom.dem[ nCol ] && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                        minimun_elevation = p2c->bottom.dem[ nCol ];
                        n = t;
                    }
                }
            }
            for ( int t = 4; t <= 6; t++ )  // 4-6
            {
                nRow = flowdirs.getRow( t, row );
                nCol = flowdirs.getCol( t, col );
                if ( minimun_elevation > dem.at( nRow, nCol ) || minimun_elevation == dem.at( nRow, nCol ) && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                    minimun_elevation = dem.at( nRow, nCol );
                    n = t;
                }
            }
            if ( p2c->right.dem.size() != 0 ) {
                int t = 7;
                nRow = flowdirs.getRow( t, row );
                if ( minimun_elevation > p2c->right.dem[ nRow ] ) {
                    n = t;
                }
            }
            flowdirs.at( row, col ) = n;
        }
        else if ( IdBounderyCells[ po ] == 0 || lowEdgeMask.at( row, col ) == 2 ) {
            float minimun_elevation = dem.at( row, col );

            int t = 0;
            nRow = flowdirs.getRow( t, row );
            if ( minimun_elevation > p2c->right.dem[ nRow ] ) {
                minimun_elevation = p2c->right.dem[ nRow ];
                n = t;
            }
            if ( minimun_elevation > p2c->bottom_right.dem[ 0 ] ) {
                minimun_elevation = p2c->bottom_right.dem[ 0 ];
                n = 1;
            }
            for ( int t = 2; t <= 3; t++ ) {
                nCol = flowdirs.getCol( t, col );
                if ( minimun_elevation > p2c->bottom.dem[ nCol ] || minimun_elevation == p2c->bottom.dem[ nCol ] && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                    minimun_elevation = p2c->bottom.dem[ nCol ];
                    n = t;
                }
            }
            for ( int t = 4; t <= 6; t++ )  // 4-6
            {
                nRow = flowdirs.getRow( t, row );
                nCol = flowdirs.getCol( t, col );
                if ( minimun_elevation > dem.at( nRow, nCol ) || minimun_elevation == dem.at( nRow, nCol ) && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                    minimun_elevation = dem.at( nRow, nCol );
                    n = t;
                }
            }
            t = 7;
            nRow = flowdirs.getRow( t, row );
            if ( minimun_elevation > p2c->right.dem[ nRow ] ) {
                n = t;
            }
            flowdirs.at( row, col ) = n;
        }
        else {
            int minimun_elevation = flatMask.at( row, col );

            int t = 0;
            nRow = flowdirs.getRow( t, row );
            if ( dem.at( row, col ) == p2c->right.dem[ nRow ] ) {
                int id = p2c->right.ID[ nRow ];
                if ( id > 0 && p2c->right.low[ id ] != 2 ) {
                    if ( minimun_elevation > p2c->right.low[ id ] - p2c->right.high[ id ] + flatHeight[ labels.at( row, col ) ] ) {
                        minimun_elevation = p2c->right.low[ id ] - p2c->right.high[ id ] + flatHeight[ labels.at( row, col ) ];
                        n = t;
                    }
                }
                else {
                    if ( minimun_elevation > 2 ) {
                        minimun_elevation = 2;
                        n = t;
                    }
                }
            }

            if ( dem.at( row, col ) == p2c->bottom_right.dem[ 0 ] )  // 1
            {
                int id = p2c->bottom_right.ID[ 0 ];
                if ( id > 0 && p2c->bottom_right.low[ id ] != 2 ) {
                    if ( minimun_elevation > p2c->bottom_right.low[ id ] - p2c->bottom_right.high[ id ] + flatHeight[ labels.at( row, col ) ] ) {
                        minimun_elevation = p2c->bottom_right.low[ id ] - p2c->bottom_right.high[ id ] + flatHeight[ labels.at( row, col ) ];
                        n = 1;
                    }
                }
                else {
                    if ( minimun_elevation > 2 ) {
                        minimun_elevation = 2;
                        n = 1;
                    }
                }
            }
            for ( int t = 2; t <= 3; t++ ) {
                nCol = flowdirs.getCol( t, col );
                if ( dem.at( row, col ) != p2c->bottom.dem[ nCol ] ) {
                    continue;
                }
                int id = p2c->bottom.ID[ nCol ];
                if ( id > 0 && p2c->bottom.low[ id ] != 2 ) {
                    if ( minimun_elevation > p2c->bottom.low[ id ] - p2c->bottom.high[ id ] + flatHeight[ labels.at( row, col ) ]
                         || minimun_elevation == p2c->bottom.low[ id ] - p2c->bottom.high[ id ] + flatHeight[ labels.at( row, col ) ] && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                        minimun_elevation = p2c->bottom.low[ id ] - p2c->bottom.high[ id ] + flatHeight[ labels.at( row, col ) ];
                        n = t;
                    }
                }
                else {
                    if ( minimun_elevation > 2 || minimun_elevation == 2 && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                        minimun_elevation = 2;
                        n = t;
                    }
                }
            }
            for ( int t = 4; t <= 6; t++ )  // 4-6
            {
                nRow = flowdirs.getRow( t, row );
                nCol = flowdirs.getCol( t, col );
                if ( labels.at( row, col ) != labels.at( nRow, nCol ) ) {
                    continue;
                }
                if ( minimun_elevation > flatMask.at( nRow, nCol ) || minimun_elevation == flatMask.at( nRow, nCol ) && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                    minimun_elevation = flatMask.at( nRow, nCol );
                    n = t;
                }
            }

            t = 7;
            nRow = flowdirs.getRow( t, row );
            if ( dem.at( row, col ) == p2c->right.dem[ nRow ] )  // 7
            {
                int id = p2c->right.ID[ nRow ];
                if ( id > 0 && p2c->right.low[ id ] != 2 ) {
                    if ( minimun_elevation > p2c->right.low[ id ] - p2c->right.high[ id ] + flatHeight[ labels.at( row, col ) ] ) {
                        n = t;
                    }
                }
                else {
                    if ( minimun_elevation > 2 ) {
                        n = t;
                    }
                }
            }
            flowdirs.at( row, col ) = n;
        }
    }
    row = flowdirs.getHeight() - 1;
    col = 0;
    if ( gridCol != 0 && gridRow != gridHeight - 1 && flowdirs.at( row, col ) != 255 )  // is not the left bottom border
    {
        int po = xy2positionNum( row, col, width, height );
        if ( p2c->left.dem.size() == 0 || p2c->bottom.dem.size() == 0 || p2c->bottom_left.dem.size() == 0 )  // left || bottom
        {
            float minimum_elevation = dem.at( row, col );
            int n = -1, nCol, nRow;
            int t = 0;
            nCol = flowdirs.getCol( t, col );
            nRow = flowdirs.getRow( t, row );
            if ( minimum_elevation > dem.at( nRow, nCol ) ) {
                minimum_elevation = dem.at( nRow, nCol );
                n = t;
            }
            if ( p2c->bottom.dem.size() == 0 ) {
                if ( minimum_elevation > dem.NoDataValue ) {
                    minimum_elevation = dem.NoDataValue;
                    n = 2;
                }
            }
            else {
                for ( int t = 1; t <= 2; t++ )  // 1-2
                {
                    nCol = flowdirs.getCol( t, col );
                    if ( minimum_elevation > p2c->bottom.dem[ nCol ] || minimum_elevation == p2c->bottom.dem[ nCol ] && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                        minimum_elevation = p2c->bottom.dem[ nCol ];
                        n = t;
                    }
                }
            }
            if ( p2c->bottom_left.dem.size() == 0 ) {
                if ( minimum_elevation > dem.NoDataValue ) {
                    minimum_elevation = dem.NoDataValue;
                    n = 3;
                }
            }
            else {
                if ( minimum_elevation > p2c->bottom_left.dem[ 0 ] ) {
                    minimum_elevation = p2c->bottom_left.dem[ 0 ];
                    n = 3;
                }
            }
            if ( p2c->left.dem.size() == 0 ) {
                if ( minimum_elevation > dem.NoDataValue || minimum_elevation == dem.NoDataValue && n > -1 && n % 2 == 1 ) {
                    minimum_elevation = dem.NoDataValue;
                    n = 4;
                }
            }
            else {
                for ( int t = 4; t <= 5; t++ )  // 4-5
                {
                    nRow = flowdirs.getRow( t, row );
                    if ( minimum_elevation > p2c->left.dem[ nRow ] || ( minimum_elevation == p2c->left.dem[ nRow ] && n > -1 && n % 2 == 1 && t % 2 == 0 ) ) {
                        minimum_elevation = p2c->left.dem[ nRow ];
                        n = t;
                    }
                }
            }
            for ( int t = 6; t <= 7; t++ )  // 6-7
            {
                nCol = flowdirs.getCol( t, col );
                nRow = flowdirs.getRow( t, row );
                if ( minimum_elevation > dem.at( nRow, nCol ) || minimum_elevation == dem.at( nRow, nCol ) && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                    minimum_elevation = dem.at( nRow, nCol );
                    n = t;
                }
            }
            flowdirs.at( row, col ) = n;
        }
        else if ( IdBounderyCells[ po ] == 0 || lowEdgeMask.at( row, col ) == 2 ) {
            float minimum_elevation = dem.at( row, col );
            int n = -1, nCol, nRow;
            int t = 0;
            nCol = flowdirs.getCol( t, col );
            nRow = flowdirs.getRow( t, row );
            if ( minimum_elevation > dem.at( nRow, nCol ) ) {
                minimum_elevation = dem.at( nRow, nCol );
                n = t;
            }
            for ( int t = 1; t <= 2; t++ )  // 1-2
            {
                nCol = flowdirs.getCol( t, col );
                if ( minimum_elevation > p2c->bottom.dem[ nCol ] || minimum_elevation == p2c->bottom.dem[ nCol ] && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                    minimum_elevation = p2c->bottom.dem[ nCol ];
                    n = t;
                }
            }
            if ( minimum_elevation > p2c->bottom_left.dem[ 0 ] ) {
                minimum_elevation = p2c->bottom_left.dem[ 0 ];
                n = 3;
            }
            for ( int t = 4; t <= 5; t++ )  // 4-5
            {
                nRow = flowdirs.getRow( t, row );
                if ( minimum_elevation > p2c->left.dem[ nRow ] || ( minimum_elevation == p2c->left.dem[ nRow ] && n > -1 && n % 2 == 1 && t % 2 == 0 ) ) {
                    minimum_elevation = p2c->left.dem[ nRow ];
                    n = t;
                }
            }
            for ( int t = 6; t <= 7; t++ )  // 6-7
            {
                nCol = flowdirs.getCol( t, col );
                nRow = flowdirs.getRow( t, row );
                if ( minimum_elevation > dem.at( nRow, nCol ) || minimum_elevation == dem.at( nRow, nCol ) && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                    minimum_elevation = dem.at( nRow, nCol );
                    n = t;
                }
            }
            flowdirs.at( row, col ) = n;
        }
        else {
            int minimum_elevation = flatMask.at( row, col );
            int n = -1, nCol, nRow;

            int t = 0;
            nCol = flowdirs.getCol( t, col );
            nRow = flowdirs.getRow( t, row );
            if ( labels.at( row, col ) == labels.at( nRow, nCol )
                 && ( minimum_elevation > flatMask.at( nRow, nCol ) || minimum_elevation == flatMask.at( nRow, nCol ) && n > -1 && n % 2 == 1 && t % 2 == 0 ) ) {
                minimum_elevation = flatMask.at( nRow, nCol );
                n = t;
            }
            for ( int t = 1; t <= 2; t++ )  // 1-2
            {
                nCol = flowdirs.getCol( t, col );
                if ( dem.at( row, col ) != p2c->bottom.dem[ nCol ] ) {
                    continue;
                }
                int id = p2c->bottom.ID[ nCol ];
                if ( id > 0 && p2c->bottom.low[ id ] != 2 ) {
                    if ( minimum_elevation > p2c->bottom.low[ id ] - p2c->bottom.high[ id ] + flatHeight[ labels.at( row, col ) ]
                         || minimum_elevation == p2c->bottom.low[ id ] - p2c->bottom.high[ id ] + flatHeight[ labels.at( row, col ) ] && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                        minimum_elevation = p2c->bottom.low[ id ] - p2c->bottom.high[ id ] + flatHeight[ labels.at( row, col ) ];
                        n = t;
                    }
                }
                else {
                    if ( minimum_elevation > 2 || minimum_elevation == 2 && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                        minimum_elevation = 2;
                        n = t;
                    }
                }
            }
            if ( dem.at( row, col ) == p2c->bottom_left.dem[ 0 ] ) {
                int id = p2c->bottom_left.ID[ 0 ];
                if ( id > 0 && p2c->bottom_left.low[ id ] != 2 ) {
                    if ( minimum_elevation > p2c->bottom_left.low[ id ] - p2c->bottom_left.high[ id ] + flatHeight[ labels.at( row, col ) ] ) {
                        minimum_elevation = p2c->bottom_left.low[ id ] - p2c->bottom_left.high[ id ] + flatHeight[ labels.at( row, col ) ];
                        n = 3;
                    }
                }
                else {
                    if ( minimum_elevation > 2 ) {
                        minimum_elevation = 2;
                        n = 3;
                    }
                }
            }
            for ( int t = 4; t <= 5; t++ )  // 1-2
            {
                nRow = flowdirs.getRow( t, row );
                if ( dem.at( row, col ) != p2c->left.dem[ nRow ] ) {
                    continue;
                }
                int id = p2c->left.ID[ nRow ];
                if ( id > 0 && p2c->left.low[ id ] != 2 ) {
                    if ( minimum_elevation > p2c->left.low[ id ] - p2c->left.high[ id ] + flatHeight[ labels.at( row, col ) ]
                         || ( minimum_elevation == p2c->left.low[ id ] - p2c->left.high[ id ] + flatHeight[ labels.at( row, col ) ] && n > -1 && n % 2 == 1 && t % 2 == 0 ) ) {
                        minimum_elevation = p2c->left.low[ id ] - p2c->left.high[ id ] + flatHeight[ labels.at( row, col ) ];
                        n = t;
                    }
                }
                else {
                    if ( minimum_elevation > 2 || ( minimum_elevation == 2 && n > -1 && n % 2 == 1 && t % 2 == 0 ) ) {
                        minimum_elevation = 2;
                        n = t;
                    }
                }
            }
            for ( int t = 6; t <= 7; t++ )  // 6-7
            {
                nCol = flowdirs.getCol( t, col );
                nRow = flowdirs.getRow( t, row );
                if ( labels.at( row, col ) != labels.at( nRow, nCol ) ) {
                    continue;
                }
                if ( minimum_elevation > flatMask.at( nRow, nCol ) || minimum_elevation == flatMask.at( nRow, nCol ) && n > -1 && n % 2 == 1 && t % 2 == 0 ) {
                    minimum_elevation = flatMask.at( nRow, nCol );
                    n = t;
                }
            }
            flowdirs.at( row, col ) = n;
        }
    }
}

void Consumer::SaveFlowDir( const GridInfo& gridInfo, const TileInfo& tileInfo, Raster< int >& flowdirs ) {
    std::vector< double > geotransform( 6 );
    for ( int i = 0; i < 6; i++ ) {
        geotransform[ i ] = dem.geoTransforms->at( i );
    }
    int t = tileInfo.filename.find_last_of( "/" );
    int length = tileInfo.filename.length();
    std::string name = tileInfo.filename.substr( t + 1, length - t - 5 );
    std::string path = gridInfo.outputFolder + "/" + name + "flowdir.tif";
    WriteGeoTIFF( path.data(), flowdirs.getHeight(), flowdirs.getWidth(), &flowdirs, GDALDataType::GDT_Int32, &geotransform[ 0 ], nullptr, nullptr, nullptr, nullptr, flowdirs.NoDataValue );
}

int Consumer::xy2positionNum( const int x, const int y, const int width, const int height ) {
    if ( x == 0 )  // Top row
    {
        return y;
    }
    if ( y == width - 1 )  // right hand side
    {
        return ( width - 1 ) + x;
    }
    if ( x == height - 1 ) {
        return ( width - 1 ) + height - 1 + width - y - 1;
    }
    return 2 * ( width - 1 ) + 2 * ( height - 1 ) - x;  // left-hand side
}
void Consumer::SaveToRetain( TileInfo& tile, StorageType& storages ) {
    auto& temp = storages[ std::make_pair( tile.gridRow, tile.gridCol ) ];
    temp.NoFlat = NoFlat;
    temp.dem = std::move( dem );
    temp.flowdirs = std::move( flowdirs );
    temp.flatHeight = flatHeight;
    temp.IdBounderyCell = std::move( IdBounderyCells );
    temp.highEdgeMask = std::move( highEdgeMask );
    temp.lowEdgeMask = std::move( lowEdgeMask );
    temp.labels = std::move( labels );
    temp.flatHeight = std::move( flatHeight );
}
void Consumer::LoadFromRetain( TileInfo& tile, StorageType& storages ) {
    auto& temp = storages.at( std::make_pair( tile.gridRow, tile.gridCol ) );
    dem = std::move( temp.dem );
    flowdirs = std::move( temp.flowdirs );
    labels = std::move( temp.labels );
    highEdgeMask = std::move( temp.highEdgeMask );
    lowEdgeMask = std::move( temp.lowEdgeMask );
    IdBounderyCells = std::move( temp.IdBounderyCell );
    NoFlat = temp.NoFlat;
    flatHeight = std::move( temp.flatHeight );
}
