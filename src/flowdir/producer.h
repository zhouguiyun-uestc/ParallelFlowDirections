
#ifndef PARADEM_PRODUCER_H
#define PARADEM_PRODUCER_H

#include <map>
#include <memory>
#include <paradem/i_producer.h>
#include <paradem/timer.h>
#include <vector>

class Producer : public IProducer {
public:
    std::vector< int > graphLow;
    std::vector< int > graphHigh;
    Timer timer_io, timer_calc;

public:
    virtual void process( std::vector< TileInfo >& tileInfos, Grid< std::shared_ptr< IConsumer2Producer > >& gridIConsumer2Producer );
    virtual std::shared_ptr< IProducer2Consumer > toConsumer( const TileInfo& tileInfo, Grid< std::shared_ptr< IConsumer2Producer > > gridIConsumer2Producer );

public:
    virtual void free();

public:
    void HandleEdge( std::vector< float >& elev_a, std::vector< float >& elev_b, std::vector< int >& ident_a, std::vector< int >& ident_b, std::vector< std::map< int, int > >& masterGraph_lowEdge,
                     const int ident_a_offset, const int ident_b_offset, std::vector< std::map< int, int > >& masterGraph_highEdge );
    void HandleCorner( float& elev_a, float& elev_b, int ident_a, int ident_b, std::vector< std::map< int, int > >& masterGraph_lowEdge, const int ident_a_offset, const int ident_b_offset,
                       std::vector< std::map< int, int > >& masterGraph_highEdge );
    void dijkstra( std::vector< std::map< int, int > >& masterGraph_lowEdge, std::vector< std::map< int, int > >& masterGraph_highEdge, int v0 );
};

#endif
