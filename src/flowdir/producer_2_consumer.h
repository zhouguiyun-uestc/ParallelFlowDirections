#ifndef PARADEM_PRODUCER2CONSUMER_H
#define PARADEM_PRODUCER2CONSUMER_H

#include <paradem/i_producer_2_consumer.h>
#include"outBoundary.h"
#include<cereal/archives/binary.hpp>
class Producer2Consumer: public IProducer2Consumer
{
private:
	friend class cereal::access;
	template<class Archive>
	void serialize(Archive &ar)
	{
		ar(c_low,
			c_high,
			left,
			right,
			top,
			bottom,
			left_top,
			right_top,
			bottom_left,
			bottom_right);
	}
public:
	std::vector<int> c_low, c_high;//自身tile的四周边界的low_graph,high_graph
	//该tile四周的高程以及low_graph,high_graph,以及边界的ID
	OutBoundary left, right, top, bottom, left_top, right_top, bottom_left, bottom_right;

public:
	Producer2Consumer();
	~Producer2Consumer();
public:
	virtual void free();
};

#endif
