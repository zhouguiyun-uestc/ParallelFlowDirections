#ifndef PARADEM_TOOL_H
#define PARADEM_TOOL_H

#include <paradem/grid_info.h>
#include <vector>
#include <paradem/tile_info.h>
#include <paradem/i_object_factory.h>
#include"mpi.h"
#include<assert.h>
#include <cereal/types/vector.hpp>
#include <cereal/types/map.hpp>
#include <cereal/archives/binary.hpp>

bool readTXTInfo(std::string filePathTXT, std::vector<TileInfo>&tileInfos, GridInfo &gridInfo);
bool generateTiles(const char* filePath, int tileHeight, int tileWidth, const char* outputFolder);
bool mergeTiles(const char* folder, GridInfo& gridInfo, const char* outputFilePath);
bool createDiffFile(const char* filePath1, const char* filePath2, const char* diffFilePath);
bool readGridInfo(const char* filePath, GridInfo& gridInfo);
void createTileInfoArray(GridInfo& gridInfo, std::vector<TileInfo>& tileInfos);
void processTileGrid(GridInfo& gridInfo, std::vector<TileInfo>& tileInfos, IObjectFactory* pIObjFactory);


#endif