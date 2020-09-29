
#include <paradem/object_deleter.h>

void ObjectDeleter::operator()(IObject* x)
{
	if (x != nullptr)
		x->free();
}


