
#ifndef PARADEM_CONSUMER_I_H
#define PARADEM_CONSUMER_I_H

#include <paradem/tile_info.h>
#include <paradem/grid_info.h>
#include <paradem/i_consumer_2_producer.h>
#include <paradem/i_producer_2_consumer.h>
#include<paradem/raster.h>


class IConsumer:public IObject
{
public:
	virtual bool processRound1(const GridInfo& gridInfo, TileInfo& tileInfo, IConsumer2Producer* pIConsumer2Producer) = 0;
	virtual bool processRound2(const GridInfo& gridInfo, const TileInfo& tileInfo, IProducer2Consumer* pIProducer2Consumer) = 0;
};

#endif

