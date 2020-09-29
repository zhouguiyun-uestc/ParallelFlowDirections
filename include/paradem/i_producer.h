#ifndef PARADEM_PRODUCER_I_H
#define PARADEM_PRODUCER_I_H

#include <memory>
#include <paradem/grid.h>
#include <paradem/grid_info.h>
#include <paradem/i_consumer_2_producer.h>
#include <paradem/i_producer_2_consumer.h>
#include <paradem/tile_info.h>

class IProducer : public IObject {
public:
    virtual void                                  process( std::vector< TileInfo >& tileInfos, Grid< std::shared_ptr< IConsumer2Producer > >& gridIConsumer2Producer ) = 0;
    virtual std::shared_ptr< IProducer2Consumer > toConsumer( const TileInfo& tileInfo, Grid< std::shared_ptr< IConsumer2Producer > > gridIConsumer2Producer )         = 0;
};

#endif
