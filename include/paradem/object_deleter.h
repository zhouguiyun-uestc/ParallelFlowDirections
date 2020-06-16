#ifndef PARADEM_OBJECT_DELETER_H
#define PARADEM_OBJECT_DELETER_H
#include <paradem/i_object.h>
class ObjectDeleter
{
public:
	void operator() (IObject * x);
};

#endif

