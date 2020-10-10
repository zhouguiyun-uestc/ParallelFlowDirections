
#ifndef PARADEM_CONSUMER2PRODUCER_H
#define PARADEM_CONSUMER2PRODUCER_H

#include <paradem/i_consumer_2_producer.h>
#include <paradem/timeInfo.h>

#include <cereal/archives/binary.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/vector.hpp>

#include <iterator>
#include <map>
#include <stdint.h>
#include <vector>

class Consumer2Producer : public IConsumer2Producer {
private:
    friend class cereal::access;
    template < class Archive > void serialize( Archive& ar ) {
        ar( top_elevs, bot_elevs, left_elevs, right_elevs, top_ID, bot_ID, left_ID, right_ID, lowEdgeGraph, highEdgeGraph, idOffset, gridRow, gridCol, nullTile, time_info );
    }

public:
    std::vector< float > top_elevs, bot_elevs, left_elevs, right_elevs;  // DEM
    std::vector< int > top_ID, bot_ID, left_ID, right_ID;
    std::vector< std::map< int, int > > lowEdgeGraph;
    std::vector< std::map< int, int > > highEdgeGraph;
    int idOffset;
    int idIncrement;
    int gridRow;
    int gridCol;
    bool nullTile;
    TimeInfo time_info;

public:
    Consumer2Producer() = default;
    ~Consumer2Producer() = default;

public:
    virtual void free();
};

#endif
