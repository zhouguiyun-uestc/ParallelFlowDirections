#ifndef PARADEM_OBJECT_FACTORY_I_H
#define PARADEM_OBJECT_FACTORY_I_H

#include <paradem/i_consumer.h>
#include <paradem/i_consumer_2_producer.h>
#include <paradem/i_producer.h>

#include <memory>

class IObjectFactory {
public:
    virtual std::shared_ptr< IConsumer > createConsumer() = 0;
    virtual std::shared_ptr< IProducer > createProducer() = 0;
    virtual std::shared_ptr< IConsumer2Producer > createConsumer2Producer() = 0;
};
#endif
