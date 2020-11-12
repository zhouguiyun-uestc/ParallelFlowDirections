
#ifndef PARADEM_PRODUCER2CONSUMER_H
#define PARADEM_PRODUCER2CONSUMER_H

#include "outBoundary.h"

#include <cereal/archives/binary.hpp>
#include <paradem/i_producer_2_consumer.h>

class Producer2Consumer : public IProducer2Consumer {
private:
    friend class cereal::access;
    template < class Archive > void serialize( Archive& ar ) {
        ar( c_low, c_high, left, right, top, bottom, left_top, right_top, bottom_left, bottom_right );
    }

public:
    std::vector< int > c_low, c_high;
    OutBoundary left, right, top, bottom, left_top, right_top, bottom_left, bottom_right;

public:
    Producer2Consumer() = default;
    virtual ~Producer2Consumer() = default;

public:
    virtual void free();
};

#endif
