
#include "producer.h"
#include "consumer_2_producer.h"
#include "outBoundary.h"
#include "producer_2_consumer.h"

#include <paradem/object_deleter.h>

#include <algorithm>
#include <assert.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <queue>

using namespace std;

void Producer::process( std::vector< TileInfo >& tileInfos, Grid< std::shared_ptr< IConsumer2Producer > >& gridIConsumer2Producer ) {
    timer_calc.start();
    Timer timer_mg_construct;
    timer_mg_construct.start();
    const int gridHeight = gridIConsumer2Producer.getHeight();
    const int gridWidth = gridIConsumer2Producer.getWidth();
    int32_t maxIDcells = 0;
    for ( int row = 0; row < gridHeight; row++ ) {
        for ( int col = 0; col < gridWidth; col++ ) {
            IConsumer2Producer* Ic2p = gridIConsumer2Producer.at( row, col ).get();
            if ( tileInfos[ row * gridWidth + col ].nullTile ) {
                Consumer2Producer pC2P;
                pC2P.nullTile = true;
                pC2P.gridRow = row;
                pC2P.gridCol = col;
                gridIConsumer2Producer.at( row, col ) = std::make_shared< Consumer2Producer >( pC2P );
                continue;
            }
            Consumer2Producer* c2p = ( Consumer2Producer* )Ic2p;
            maxIDcells += c2p->lowEdgeGraph.size();
        }
    }
    std::vector< std::map< int, int > > masterLowEdgeGraph( maxIDcells );
    std::vector< std::map< int, int > > masterHighEdgeGraph( maxIDcells );
    std::cerr << "!Total labels required: " << maxIDcells << std::endl;
    uint32_t idOffset = 0;
    uint32_t lowMap = 0;
    uint32_t highMap = 0;
    std::map< int, int >::iterator skey;
    for ( int row = 0; row < gridHeight; row++ ) {
        for ( int col = 0; col < gridWidth; col++ ) {

            IConsumer2Producer* Ic2p = gridIConsumer2Producer.at( row, col ).get();
            Consumer2Producer* c2p = ( Consumer2Producer* )Ic2p;
            if ( c2p->nullTile || tileInfos[ row * gridWidth + col ].nullTile ) {
                continue;
            }
            c2p->idOffset = idOffset;
            try {
                for ( int m = 0; m < ( int )c2p->lowEdgeGraph.size(); m++ ) {
                    for ( skey = c2p->lowEdgeGraph[ m ].begin(); skey != c2p->lowEdgeGraph[ m ].end(); skey++ ) {
                        int first_id = m;
                        int second_id = skey->first;
                        if ( first_id > 1 ) {
                            first_id += idOffset;
                        }
                        if ( second_id > 1 ) {
                            second_id += idOffset;
                        }
                        masterLowEdgeGraph.at( first_id )[ second_id ] = skey->second;
                        masterLowEdgeGraph.at( second_id )[ first_id ] = skey->second;
                        lowMap++;
                    }
                    for ( skey = c2p->highEdgeGraph[ m ].begin(); skey != c2p->highEdgeGraph[ m ].end(); skey++ ) {
                        int first_id = m;
                        int second_id = skey->first;
                        if ( first_id > 1 ) {
                            first_id += idOffset;
                        }
                        if ( second_id > 1 ) {
                            second_id += idOffset;
                        }
                        masterHighEdgeGraph.at( first_id )[ second_id ] = skey->second;
                        masterHighEdgeGraph.at( second_id )[ first_id ] = skey->second;
                        highMap++;
                    }
                }
            }
            catch ( const std::exception& ex ) {
                std::cerr << "sth wrong" << std::endl;
                std::cerr << ex.what() << "sth wrong 2" << std::endl;
            }
            idOffset += c2p->lowEdgeGraph.size();
            c2p->idIncrement = c2p->lowEdgeGraph.size();
            c2p->lowEdgeGraph.clear();
            c2p->highEdgeGraph.clear();
        }
    }

    std::cerr << " number of id" << masterLowEdgeGraph.size() << std::endl;
    std::cerr << "number of map in masterLowEdgeGraph " << lowMap << std::endl;
    std::cerr << "number of map in masterHighEdgeGraph " << highMap << std::endl;
    for ( int row = 0; row < gridHeight; row++ ) {
        for ( int col = 0; col < gridWidth; col++ ) {
            IConsumer2Producer* Ic2p = gridIConsumer2Producer.at( row, col ).get();
            Consumer2Producer* c = ( Consumer2Producer* )Ic2p;
            if ( c->nullTile ) {
                continue;
            }
            if ( row > 0 && col > 0 && !( ( Consumer2Producer* )&*gridIConsumer2Producer.at( row - 1, col - 1 ) )->nullTile ) {
                HandleCorner( c->top_elevs.front(), ( ( Consumer2Producer* )&*gridIConsumer2Producer.at( row - 1, col - 1 ) )->bot_elevs.back(), c->top_ID.front(),
                              ( ( Consumer2Producer* )&*gridIConsumer2Producer.at( row - 1, col - 1 ) )->bot_ID.back(), masterLowEdgeGraph, c->idOffset,
                              ( ( Consumer2Producer* )gridIConsumer2Producer.at( row - 1, col - 1 ).get() )->idOffset, masterHighEdgeGraph );
            }
            if ( row < gridHeight - 1 && col < gridWidth - 1 && !( ( Consumer2Producer* )&*gridIConsumer2Producer.at( row + 1, col + 1 ) )->nullTile ) {
                HandleCorner( c->bot_elevs.back(), ( ( Consumer2Producer* )&*gridIConsumer2Producer.at( row + 1, col + 1 ) )->top_elevs.front(), c->bot_ID.back(),
                              ( ( Consumer2Producer* )&*gridIConsumer2Producer.at( row + 1, col + 1 ) )->top_ID.front(), masterLowEdgeGraph, c->idOffset,
                              ( ( Consumer2Producer* )gridIConsumer2Producer.at( row + 1, col + 1 ).get() )->idOffset, masterHighEdgeGraph );
            }
            if ( row > 0 && col < gridWidth - 1 && !( ( Consumer2Producer* )&*gridIConsumer2Producer.at( row - 1, col + 1 ) )->nullTile ) {
                HandleCorner( c->top_elevs.back(), ( ( Consumer2Producer* )&*gridIConsumer2Producer.at( row - 1, col + 1 ) )->bot_elevs.front(), c->top_ID.back(),
                              ( ( Consumer2Producer* )&*gridIConsumer2Producer.at( row - 1, col + 1 ) )->bot_ID.front(), masterLowEdgeGraph, c->idOffset,
                              ( ( Consumer2Producer* )gridIConsumer2Producer.at( row - 1, col + 1 ).get() )->idOffset, masterHighEdgeGraph );
            }
            if ( col > 0 && row < gridHeight - 1 && !( ( Consumer2Producer* )&*gridIConsumer2Producer.at( row + 1, col - 1 ) )->nullTile ) {

                HandleCorner( c->bot_elevs.front(), ( ( Consumer2Producer* )&*gridIConsumer2Producer.at( row + 1, col - 1 ) )->top_elevs.back(), c->bot_ID.front(),
                              ( ( Consumer2Producer* )&*gridIConsumer2Producer.at( row + 1, col - 1 ) )->top_ID.back(), masterLowEdgeGraph, c->idOffset,
                              ( ( Consumer2Producer* )gridIConsumer2Producer.at( row + 1, col - 1 ).get() )->idOffset, masterHighEdgeGraph );
            }
            if ( row > 0 && !( ( Consumer2Producer* )&*gridIConsumer2Producer.at( row - 1, col ) )->nullTile ) {
                HandleEdge( c->top_elevs, ( ( Consumer2Producer* )&*gridIConsumer2Producer.at( row - 1, col ) )->bot_elevs, c->top_ID,
                            ( ( Consumer2Producer* )&*gridIConsumer2Producer.at( row - 1, col ) )->bot_ID, masterLowEdgeGraph, c->idOffset,
                            ( ( Consumer2Producer* )gridIConsumer2Producer.at( row - 1, col ).get() )->idOffset, masterHighEdgeGraph );
            }
            if ( row < gridHeight - 1 && !( ( Consumer2Producer* )&*gridIConsumer2Producer.at( row + 1, col ) )->nullTile ) {
                HandleEdge( c->bot_elevs, ( ( Consumer2Producer* )&*gridIConsumer2Producer.at( row + 1, col ) )->top_elevs, c->bot_ID,
                            ( ( Consumer2Producer* )&*gridIConsumer2Producer.at( row + 1, col ) )->top_ID, masterLowEdgeGraph, c->idOffset,
                            ( ( Consumer2Producer* )gridIConsumer2Producer.at( row + 1, col ).get() )->idOffset, masterHighEdgeGraph );
            }
            if ( col > 0 && !( ( Consumer2Producer* )&*gridIConsumer2Producer.at( row, col - 1 ) )->nullTile ) {
                HandleEdge( c->left_elevs, ( ( Consumer2Producer* )&*gridIConsumer2Producer.at( row, col - 1 ) )->right_elevs, c->left_ID,
                            ( ( Consumer2Producer* )&*gridIConsumer2Producer.at( row, col - 1 ) )->right_ID, masterLowEdgeGraph, c->idOffset,
                            ( ( Consumer2Producer* )gridIConsumer2Producer.at( row, col - 1 ).get() )->idOffset, masterHighEdgeGraph );
            }
            if ( col < gridWidth - 1 && !( ( Consumer2Producer* )&*gridIConsumer2Producer.at( row, col + 1 ) )->nullTile ) {
                HandleEdge( c->right_elevs, ( ( Consumer2Producer* )&*gridIConsumer2Producer.at( row, col + 1 ) )->left_elevs, c->right_ID,
                            ( ( Consumer2Producer* )&*gridIConsumer2Producer.at( row, col + 1 ) )->left_ID, masterLowEdgeGraph, c->idOffset,
                            ( ( Consumer2Producer* )gridIConsumer2Producer.at( row, col + 1 ).get() )->idOffset, masterHighEdgeGraph );
            }
        }
    }
    timer_mg_construct.stop();
    std::cerr << "!Mastergraph constructed in " << timer_mg_construct.elapsed() << "s. " << std::endl;
    if ( maxIDcells != 0 ) {
        dijkstra( masterLowEdgeGraph, masterHighEdgeGraph, 1 );
    }
    timer_calc.stop();
}

std::shared_ptr< IProducer2Consumer > Producer::toConsumer( const TileInfo& tileInfo, Grid< std::shared_ptr< IConsumer2Producer > > gridIConsumer2Producer ) {
    int gridRow = tileInfo.gridRow;
    int gridCol = tileInfo.gridCol;
    int gridWidth = gridIConsumer2Producer.getWidth();
    int gridHeight = gridIConsumer2Producer.getHeight();
    Producer2Consumer* p2c = new Producer2Consumer();
    auto job_low = std::vector< int >( graphLow.begin() + ( ( Consumer2Producer* )gridIConsumer2Producer.at( gridRow, gridCol ).get() )->idOffset,
                                       graphLow.begin() + ( ( Consumer2Producer* )gridIConsumer2Producer.at( gridRow, gridCol ).get() )->idOffset
                                           + ( ( Consumer2Producer* )gridIConsumer2Producer.at( gridRow, gridCol ).get() )->idIncrement );
    auto job_high = std::vector< int >( graphHigh.begin() + ( ( Consumer2Producer* )gridIConsumer2Producer.at( gridRow, gridCol ).get() )->idOffset,
                                        graphHigh.begin() + ( ( Consumer2Producer* )gridIConsumer2Producer.at( gridRow, gridCol ).get() )->idOffset
                                            + ( ( Consumer2Producer* )gridIConsumer2Producer.at( gridRow, gridCol ).get() )->idIncrement );
    OutBoundary left, right, bottom, top, left_top, right_top, left_bottom, right_bottom;
    if ( gridCol > 0 )  // left  1
    {
        Consumer2Producer* c2p = ( Consumer2Producer* )&*gridIConsumer2Producer.at( gridRow, gridCol - 1 );
        if ( !c2p->nullTile ) {
            left = OutBoundary( c2p->right_elevs, std::vector< int >( graphLow.begin() + c2p->idOffset, graphLow.begin() + c2p->idOffset + c2p->idIncrement ),
                                std::vector< int >( graphHigh.begin() + c2p->idOffset, graphHigh.begin() + c2p->idOffset + c2p->idIncrement ), c2p->right_ID );
        }
    }
    if ( gridCol > 0 && gridRow > 0 )  // top left
    {
        Consumer2Producer* c2p = ( Consumer2Producer* )&*gridIConsumer2Producer.at( gridRow - 1, gridCol - 1 );
        if ( !c2p->nullTile ) {
            left_top = OutBoundary( std::vector< float >( 1, c2p->bot_elevs.back() ), std::vector< int >( graphLow.begin() + c2p->idOffset, graphLow.begin() + c2p->idOffset + c2p->idIncrement ),
                                    std::vector< int >( graphHigh.begin() + c2p->idOffset, graphHigh.begin() + c2p->idOffset + c2p->idIncrement ), std::vector< int >( 1, c2p->bot_ID.back() ) );
        }
    }
    if ( gridRow > 0 )  // top
    {
        Consumer2Producer* c2p = ( Consumer2Producer* )&*gridIConsumer2Producer.at( gridRow - 1, gridCol );
        if ( !c2p->nullTile ) {
            top = OutBoundary( c2p->bot_elevs, std::vector< int >( graphLow.begin() + c2p->idOffset, graphLow.begin() + c2p->idOffset + c2p->idIncrement ),
                               std::vector< int >( graphHigh.begin() + c2p->idOffset, graphHigh.begin() + c2p->idOffset + c2p->idIncrement ), c2p->bot_ID );
        }
    }
    if ( gridCol < gridWidth - 1 && gridRow > 0 )  // top right
    {
        Consumer2Producer* c2p = ( Consumer2Producer* )&*gridIConsumer2Producer.at( gridRow - 1, gridCol + 1 );
        if ( !c2p->nullTile ) {
            right_top = OutBoundary( std::vector< float >( 1, c2p->bot_elevs.front() ), std::vector< int >( graphLow.begin() + c2p->idOffset, graphLow.begin() + c2p->idOffset + c2p->idIncrement ),
                                     std::vector< int >( graphHigh.begin() + c2p->idOffset, graphHigh.begin() + c2p->idOffset + c2p->idIncrement ), std::vector< int >( 1, c2p->bot_ID.front() ) );
        }
    }
    if ( gridCol < gridWidth - 1 )  // right
    {
        Consumer2Producer* c2p = ( Consumer2Producer* )&*gridIConsumer2Producer.at( gridRow, gridCol + 1 );
        if ( !c2p->nullTile ) {
            right = OutBoundary( c2p->left_elevs, std::vector< int >( graphLow.begin() + c2p->idOffset, graphLow.begin() + c2p->idOffset + c2p->idIncrement ),
                                 std::vector< int >( graphHigh.begin() + c2p->idOffset, graphHigh.begin() + c2p->idOffset + c2p->idIncrement ), c2p->left_ID );
        }
    }
    if ( gridRow < gridHeight - 1 && gridCol < gridWidth - 1 )  // right bottom
    {
        Consumer2Producer* c2p = ( Consumer2Producer* )&*gridIConsumer2Producer.at( gridRow + 1, gridCol + 1 );
        if ( !c2p->nullTile ) {
            right_bottom = OutBoundary( std::vector< float >( 1, c2p->top_elevs.front() ), std::vector< int >( graphLow.begin() + c2p->idOffset, graphLow.begin() + c2p->idOffset + c2p->idIncrement ),
                                        std::vector< int >( graphHigh.begin() + c2p->idOffset, graphHigh.begin() + c2p->idOffset + c2p->idIncrement ), std::vector< int >( 1, c2p->top_ID.front() ) );
        }
    }
    if ( gridRow < gridHeight - 1 )  // bottolm
    {
        Consumer2Producer* c2p = ( Consumer2Producer* )&*gridIConsumer2Producer.at( gridRow + 1, gridCol );
        if ( !c2p->nullTile ) {
            bottom = OutBoundary( c2p->top_elevs, std::vector< int >( graphLow.begin() + c2p->idOffset, graphLow.begin() + c2p->idOffset + c2p->idIncrement ),
                                  std::vector< int >( graphHigh.begin() + c2p->idOffset, graphHigh.begin() + c2p->idOffset + c2p->idIncrement ), c2p->top_ID );
        }
    }
    if ( gridRow < gridHeight - 1 && gridCol > 0 )  // left bottom
    {
        Consumer2Producer* c2p = ( Consumer2Producer* )&*gridIConsumer2Producer.at( gridRow + 1, gridCol - 1 );
        if ( !c2p->nullTile ) {
            left_bottom = OutBoundary( std::vector< float >( 1, c2p->top_elevs.back() ), std::vector< int >( graphLow.begin() + c2p->idOffset, graphLow.begin() + c2p->idOffset + c2p->idIncrement ),
                                       std::vector< int >( graphHigh.begin() + c2p->idOffset, graphHigh.begin() + c2p->idOffset + c2p->idIncrement ), std::vector< int >( 1, c2p->top_ID.back() ) );
        }
    }
    p2c->top = top;
    p2c->left = left;
    p2c->right = right;
    p2c->bottom = bottom;
    p2c->left_top = left_top;
    p2c->right_top = right_top;
    p2c->bottom_right = right_bottom;
    p2c->bottom_left = left_bottom;
    p2c->c_high = job_high;
    p2c->c_low = job_low;
    return std::shared_ptr< IProducer2Consumer >( p2c, ObjectDeleter() );
}

void Producer::free() {
    delete this;
}

void Producer::HandleEdge( std::vector< float >& elev_a, std::vector< float >& elev_b, std::vector< int >& ident_a, std::vector< int >& ident_b,
                           std::vector< std::map< int, int > >& masterGraph_lowEdge, const int ident_a_offset, const int ident_b_offset, std::vector< std::map< int, int > >& masterGraph_highEdge ) {
    assert( elev_a.size() == elev_b.size() );
    assert( ident_a.size() == ident_b.size() );
    assert( elev_a.size() == ident_b.size() );
    int len = elev_a.size();
    for ( int i = 0; i < len; i++ ) {
        bool flagLow = false, flagHigh = false;
        if ( ident_a[ i ] > 0 ) {
            auto c_1 = ident_a[ i ];

            if ( c_1 > 1 ) {
                c_1 += ident_a_offset;
            }
            for ( int j = i - 1; j <= i + 1; j++ ) {

                if ( j < 0 || j == len ) {
                    continue;
                }
                if ( elev_a[ i ] > elev_b[ j ] ) {
                    flagLow = true;
                }
                else if ( elev_a[ i ] < elev_b[ j ] ) {
                    flagHigh = true;
                }
                else {
                    if ( ident_b[ j ] > 0 ) {
                        auto n_1 = ident_b[ j ];
                        if ( n_1 > 1 ) {
                            n_1 += ident_b_offset;
                        }

                        masterGraph_lowEdge[ c_1 ][ n_1 ] = 2;
                        masterGraph_lowEdge[ n_1 ][ c_1 ] = 2;

                        masterGraph_highEdge[ c_1 ][ n_1 ] = 1;
                        masterGraph_highEdge[ n_1 ][ c_1 ] = 1;
                    }
                    else {
                        masterGraph_lowEdge[ c_1 ][ 1 ] = 2;
                        masterGraph_lowEdge[ 1 ][ c_1 ] = 2;
                    }
                }
            }
            if ( flagLow ) {
                masterGraph_lowEdge[ c_1 ][ 1 ] = 0;
                masterGraph_lowEdge[ 1 ][ c_1 ] = 0;
                for ( std::map< int, int >::iterator ite = masterGraph_highEdge.at( c_1 ).begin(); ite != masterGraph_highEdge.at( c_1 ).end(); ) {
                    int fi = ite->first;
                    masterGraph_highEdge.at( c_1 ).erase( ite++ );
                    masterGraph_highEdge.at( fi ).erase( c_1 );
                }
            }
            else if ( flagHigh ) {
                masterGraph_highEdge[ c_1 ][ 1 ] = 0;
                masterGraph_highEdge[ 1 ][ c_1 ] = 0;
            }
        }
    }
}

void Producer::HandleCorner( float& elev_a, float& elev_b, int ident_a, int ident_b, std::vector< std::map< int, int > >& masterGraph_lowEdge, const int ident_a_offset, const int ident_b_offset,
                             std::vector< std::map< int, int > >& masterGraph_highEdge ) {
    if ( ident_a > 0 ) {
        if ( ident_a > 1 ) {
            ident_a += ident_a_offset;
        }
        if ( ident_b > 1 ) {
            ident_b += ident_b_offset;
        }
        if ( elev_a == elev_b ) {
            if ( ident_b > 0 ) {
                masterGraph_lowEdge[ ident_a ][ ident_b ] = 2;
                masterGraph_lowEdge[ ident_b ][ ident_a ] = 2;
                masterGraph_highEdge[ ident_a ][ ident_b ] = 1;
                masterGraph_highEdge[ ident_b ][ ident_a ] = 1;
            }
            else {
                masterGraph_lowEdge[ ident_a ][ 1 ] = 2;
                masterGraph_lowEdge[ 1 ][ ident_a ] = 2;
            }
        }
        else if ( elev_a > elev_b ) {
            masterGraph_lowEdge[ ident_a ][ 1 ] = 0;
            masterGraph_lowEdge[ 1 ][ ident_a ] = 0;

            for ( std::map< int, int >::iterator ite = masterGraph_highEdge.at( ident_a ).begin(); ite != masterGraph_highEdge.at( ident_a ).end(); ) {
                int fi = ite->first;
                masterGraph_highEdge.at( ident_a ).erase( ite++ );
                masterGraph_highEdge.at( fi ).erase( ident_a );
            }
        }
        else {
            masterGraph_highEdge[ ident_a ][ 1 ] = 0;
            masterGraph_highEdge[ 1 ][ ident_a ] = 0;
        }
    }
}

void Producer::dijkstra( std::vector< std::map< int, int > >& masterGraph_lowEdge, std::vector< std::map< int, int > >& masterGraph_highEdge, int v0 ) {
    Timer flowDir_timer;
    flowDir_timer.start();
    int v;
    typedef std::pair< int, int > pii;
    std::priority_queue< pii, std::vector< pii >, std::greater< pii > > q;
    int maxlabel = masterGraph_lowEdge.size();
    graphLow.resize( maxlabel, 9999999 );
    graphHigh.resize( maxlabel, 9999999 );
    pii cur;
    graphLow[ v0 ] = 2;
    q.push( std::make_pair( 0, v0 ) );
    while ( !q.empty() ) {
        cur = q.top();
        q.pop();
        v = cur.second;
        if ( cur.first > graphLow[ v ] ) {
            continue;
        }
        for ( auto& n : masterGraph_lowEdge[ v ] ) {
            auto n_vertex_num = n.first;
            auto n_elev = n.second;
            if ( graphLow[ v ] + n_elev < graphLow[ n_vertex_num ] ) {
                graphLow[ n_vertex_num ] = graphLow[ v ] + n_elev;
                q.push( pii( graphLow[ n_vertex_num ], n_vertex_num ) );
            }
        }
    }
    graphHigh[ v0 ] = 1;
    std::priority_queue< pii, std::vector< pii >, std::greater< pii > > q_high;
    q_high.push( std::make_pair( 0, v0 ) );
    while ( !q_high.empty() ) {
        cur = q_high.top();
        q_high.pop();
        v = cur.second;
        if ( cur.first > graphHigh[ v ] ) {
            continue;
        }
        for ( auto& n : masterGraph_highEdge[ v ] ) {
            auto n_vertex_num = n.first;
            auto n_elev = n.second;
            if ( graphHigh[ v ] + n_elev < graphHigh[ n_vertex_num ] ) {

                graphHigh[ n_vertex_num ] = graphHigh[ v ] + n_elev;
                q_high.push( pii( graphHigh[ n_vertex_num ], n_vertex_num ) );
            }
        }
    }
    flowDir_timer.stop();
    std::cerr << "!Aggregated flow time: " << flowDir_timer.elapsed() << "s." << std::endl;
}
