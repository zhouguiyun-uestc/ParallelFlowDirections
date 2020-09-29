
#ifndef PARADEM_OBJECT_FACTORY_H
#define PARADEM_OBJECT_FACTORY_H

#include <paradem/i_object_factory.h>

class ObjectFactory:public IObjectFactory
{
public:
	virtual std::shared_ptr<IConsumer> createConsumer();
	virtual std::shared_ptr<IProducer> createProducer();
	virtual std::shared_ptr<IConsumer2Producer> createConsumer2Producer();
};

#endif

