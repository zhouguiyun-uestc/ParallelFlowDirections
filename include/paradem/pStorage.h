#ifndef PARADEM_PSTORAGE_H
#define PARADEM_PSTORAGE_H
#include"raster.h"
#include<map>
typedef  struct storageTile
{
	Raster<int> flowdirs;
	Raster<int> labels;
	Raster<int> IdBounderyCell;
	Raster<int> lowEdgeMask;
	Raster<int> highEdgeMask;
	std::vector<int> flatHeight;
	bool NoFlat;
}; 

using StorageType = std::map<std::pair<int, int>, storageTile>;
#endif
