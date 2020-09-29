
#include "object_factory.h"
#include "consumer.h"
#include "consumer_2_producer.h"
#include "producer.h"
#include <paradem/object_deleter.h>

std::shared_ptr< IConsumer > ObjectFactory::createConsumer() {
    return std::shared_ptr< IConsumer >( new Consumer(), ObjectDeleter() );
}

std::shared_ptr< IProducer > ObjectFactory::createProducer() {
    return std::shared_ptr< IProducer >( new Producer(), ObjectDeleter() );
}

std::shared_ptr< IConsumer2Producer > ObjectFactory::createConsumer2Producer() {
    return std::shared_ptr< IConsumer2Producer >( new Consumer2Producer(), ObjectDeleter() );
}
