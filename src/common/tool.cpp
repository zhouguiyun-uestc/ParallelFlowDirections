#include <paradem/tool.h>
#include <paradem/i_consumer.h>
#include <paradem/i_producer.h>
#include <paradem/i_object_factory.h>
#include <paradem/grid.h>
#include <memory>
#include <paradem/object_deleter.h>
#include<memory>
#include<cstring>
#include <iostream> 
#include<fstream>
#include<string>
#include<paradem/gdal.h>
#include<vector>
#include<map>
#include<paradem/tool.h>
#include"mpi.h"
#include<assert.h>
#include<cmath>
#include<algorithm>
//#include <unistd.h>
//#include<io.h>


bool readTXTInfo(std::string filePathTXT, std::vector<TileInfo>&tileInfos, GridInfo& gridInfo)
{
	std::vector< std::vector< std::string > > fgrid;
	//Open file
	std::ifstream fin_layout(filePathTXT);
	if (!fin_layout.is_open())
	{
		std::cerr << "cant open the file" << std::endl;
	}

	//Did the file open?
	if (!fin_layout.good())
		throw std::runtime_error("Problem opening layout file '" + filePathTXT + "'");

	//Read the entire file
	while (fin_layout) {
		//Get a new line from the file.
		std::string row;
		if (!getline(fin_layout, row))
			break;

		//Add another row to the grid to store the data from this line
		fgrid.emplace_back();
		//Put the data somewhere we can process it
		std::istringstream ss(row);

		//While there is unprocessed data
		while (ss) {
			std::string item;
			if (!getline(ss, item, ','))
				break;
			if (!item.empty())
			{
				item.erase(0, item.find_first_not_of(" "));
				item.erase(item.find_last_not_of(" ") + 1);
				//std::cout << "the value of item" << item << std::endl;
			}
			/*else
			{
				//std::cout << "the value of item is empty" << item << std::endl;
			}*/
			
			fgrid.back().push_back(item);
		}
	}
	if (!fin_layout.eof())
		throw std::runtime_error("Failed to read the entire layout file!");

	//Let's find the longest row
	auto max_col_size = fgrid.front().size();
	for (const auto &row : fgrid)
		max_col_size = std::max(max_col_size, row.size());

	for (auto &row : fgrid)
		if (row.size() == max_col_size - 1)   //If the line was one short of max, assume it ends blank
			row.emplace_back();
		else if (row.size() < max_col_size) //The line was more than one short of max: uh oh
			throw std::runtime_error("Not all of the rows in the layout file had the same number of columns!");

	//SONGLIHUI ADD INFORMATION
	auto max_row_size = fgrid.size();
	gridInfo.gridHeight = max_row_size;
	gridInfo.gridWidth = max_col_size;
	//assign filename into tileinfo
	tileInfos.resize(max_row_size*max_col_size);
	//std::cout << max_row_size << "..." << max_col_size << std::endl;
	for (int tileRow = 0; tileRow < max_row_size; tileRow++)
	{
		for (int tileCol = 0; tileCol < max_col_size; tileCol++)
		{
			TileInfo tileInfo;
			if (fgrid[tileRow][tileCol].size()==0)
			{
				tileInfo.nullTile = true;
			}
			else
			{
				tileInfo.filename = fgrid[tileRow][tileCol];
				tileInfo.nullTile = false;
			}
			//std::cout << tileRow << " ," << tileCol << "," << tileInfo.nullTile << "," << tileInfo.filename << "。" << std::endl;
			tileInfo.gridRow = tileRow;
			tileInfo.gridCol = tileCol;
			tileInfos[tileRow*max_col_size + tileCol] = tileInfo;
		}
	}

	for (int tileRow = 0; tileRow < max_row_size; tileRow++)
	{
		for (int tileCol = 0; tileCol < max_col_size; tileCol++)
		{
			if (tileInfos[tileRow*max_col_size + tileCol].nullTile) //如果它为nullTile，则它的上下左右，左上，右上，左下，右下
			{
				//判断不是最右边
				if (tileCol != max_col_size - 1)
				{
					tileInfos[tileRow*max_col_size + tileCol + 1].d8Tile |= TILE_LEFT;
					if (tileRow > 0)//不是第一行
					{
						tileInfos[(tileRow - 1)*max_col_size + tileCol + 1].d8Tile |= TILE_LEFT_BOTTOM;
					}
					if (tileRow < max_row_size - 1)//不是最后一行
					{
						tileInfos[(tileRow + 1)*max_col_size + tileCol + 1].d8Tile |= TILE_LEFT_TOP;
					}

				}
				//判断不是最上边
				if (tileRow != 0)
				{
					tileInfos[(tileRow - 1)*max_col_size + tileCol].d8Tile |= TILE_BOTTOM;
					if (tileCol > 0)
					{
						tileInfos[(tileRow - 1)*max_col_size + tileCol - 1].d8Tile |= TILE_RIGHT_BOTTOM;
					}
					//tile_left_bottom的已经有了
				}
				//判断不是最左边
				if (tileCol != 0)
				{
					tileInfos[tileRow *max_col_size + tileCol - 1].d8Tile |= TILE_RIGHT;
					if (tileRow < max_row_size - 1)
					{
						tileInfos[(tileRow + 1) *max_col_size + tileCol - 1].d8Tile |= TILE_RIGHT_TOP;
					}
				}
				//判断不是最下边
				if (tileRow != max_row_size - 1)
				{
					tileInfos[(tileRow + 1) *max_col_size + tileCol].d8Tile |= TILE_TOP;
				}
			}
		}
	}


}

bool generateTiles(const char* filePath, int tileHeight, int tileWidth, const char* outputFolder)
{
	std::string inputFilePath = filePath;
	std::string output = outputFolder;	
	GDALAllRegister();
	GDALDataset *fin = (GDALDataset*)GDALOpen(inputFilePath.c_str(), GA_ReadOnly);
	if (fin == NULL)
		throw std::runtime_error("Could not open file '" + inputFilePath + "' to get dimensions.");
	GDALRasterBand *band = fin->GetRasterBand(1);
	GDALDataType type = band->GetRasterDataType();
	//file type is assumed to be float
	int grandHeight = band->GetYSize();
	int grandWidth = band->GetXSize();
	std::vector<double> geotransform(6);
	fin->GetGeoTransform(&geotransform[0]);
	int height, width;
	int gridHeight = std::ceil((double)grandHeight / tileHeight);
	int gridWidth = std::ceil((double)grandWidth / tileWidth);
	for (int tileRow = 0; tileRow < gridHeight; tileRow++)
	{
		for (int tileCol = 0; tileCol < gridWidth; tileCol++)
		{
			std::string outputFileName = std::to_string(tileRow) + "_" + std::to_string(tileCol) + ".tif";
			std::string path = output + "\\" + outputFileName;
			height = (grandHeight - tileHeight*tileRow >= tileHeight) ? tileHeight : (grandHeight - tileHeight*tileRow);
			width = (grandWidth - tileWidth*tileCol >= tileWidth) ? tileWidth : (grandWidth - tileWidth*tileCol);
			Raster<float> tile;
			if (!tile.init(height, width))
			{
				GDALClose((GDALDatasetH)fin);
				return false;
			}
			band->RasterIO(GF_Read, tileWidth*tileCol, tileHeight*tileRow, tile.getWidth(), tile.getHeight(),
				(void *)&tile, tile.getWidth(), tile.getHeight(), type, 0, 0);
			std::vector<double> tileGeotransform(geotransform);
			tileGeotransform[0] = geotransform[0] + tileWidth*tileCol*geotransform[1] + tileHeight*tileRow*geotransform[2];
			tileGeotransform[3] = geotransform[3] + tileHeight*tileRow*geotransform[5] + tileWidth*tileCol*geotransform[4];
			WriteGeoTIFF(path.data(), tile.getHeight(), tile.getWidth(), &tile, type, &tileGeotransform[0], nullptr, nullptr, nullptr, nullptr, tile.NoDataValue);
		}
	}
	GDALClose((GDALDatasetH)fin);
	std::string txtPath = output + "\\" + "gridInfo.txt";
	std::ofstream fout;
	fout.open(txtPath, std::ofstream::app);
	if (fout.fail()) { 
		std::cerr << "Open " << txtPath << " error!" << std::endl; return false; 
	}
	fout << tileHeight << std::endl;
	fout << tileWidth << std::endl;
	fout << gridHeight << std::endl;
	fout << gridWidth << std::endl;
	fout << grandHeight << std::endl;
	fout << grandWidth << std::endl;
	fout.close();	
	return true;
}

bool mergeTiles(const char* folder,GridInfo& gridInfo, const char* outputFilePath)
{
	//if (!readGridInfo((std::string(folder)+"\\gridInfo.txt").c_str(), gridInfo)) return false;

	//int grandHeight = gridInfo.grandHeight;
	//int grandWidth = gridInfo.grandWidth;
	//int gridHeight = gridInfo.gridHeight;
	//int gridWidth = gridInfo.gridWidth;
	//int tileHeight = gridInfo.tileHeight;
	//int tileWidth = gridInfo.tileWidth;
	//Raster<float> tiles;
	//if (!tiles.init(grandHeight, grandWidth)) return false;
	//std::string inputFolder = folder;
	//std::vector<double> geotransform(6);
	//for (int tileRow = 0; tileRow < gridHeight; tileRow++)
	//{
	//	for (int tileCol = 0; tileCol < gridWidth; tileCol++)
	//	{
	//		std::string fileName = inputFolder + "\\" + std::to_string(tileRow) + "_" + std::to_string(tileCol) + ".tif";
	//		Raster<float> tile;
	//		if (!readGeoTIFF(fileName.data(), GDALDataType::GDT_Float32, tile));//GDALDataType::GDT_Float32
	//		if (tileRow == 0 && tileCol == 0)
	//		{
	//			for (int i = 0; i < 6; i++)
	//			{
	//				geotransform[i] = tile.geoTransforms->at(i);
	//			}
	//		}
	//
	//		int height = tile.getHeight();
	//		int width = tile.getWidth();
	//		int startRow = tileHeight*tileRow;
	//		int startCol = tileWidth*tileCol;
	//		for (int row = 0; row < height; row++)
	//		{
	//			for (int col = 0; col < width; col++)
	//			{
	//				tiles.at(startRow + row, startCol + col) = tile.at(row, col);
	//			}
	//		}
	//	}
	//}

	//WriteGeoTIFF(outputFilePath, tiles.getHeight(), tiles.getWidth(), &tiles, GDALDataType::GDT_Float32, &geotransform[0], nullptr, nullptr, nullptr, nullptr, tiles.NoDataValue);
	////delete tiles?
	return true;

}

//this file is yet to be tested
bool createDiffFile(const char* filePath1, const char* filePath2, const char* diffFilePath)
{
	Raster<float> dem1, dem2;
	if (!readGeoTIFF(filePath1, GDALDataType::GDT_Float32, dem1)) return false;
	if (!readGeoTIFF(filePath2, GDALDataType::GDT_Float32, dem2)) return false;
	Raster<float> diff=dem1 - dem2;
	WriteGeoTIFF("d:\\temp\\filled.tif", diff.getHeight(), diff.getWidth(), &diff, GDALDataType::GDT_Float32, diff.getGeoTransformsPtr(), nullptr, nullptr, nullptr, nullptr, diff.NoDataValue);
	return true;
}

void processTileGrid(GridInfo& gridInfo, std::vector<TileInfo>& tileInfos, IObjectFactory* pIObjFactory) 
{
	Grid<std::shared_ptr<IConsumer2Producer>> gridIConsumer2Producer;
	gridIConsumer2Producer.init(gridInfo.gridHeight, gridInfo.gridWidth);
	//std::shared_ptr<IConsumer> pIConsumer = pIObjFactory->createConsumer();

	for (int i =0; i < tileInfos.size(); i++)//tileInfos.size()
	{
		TileInfo& tileInfo = tileInfos[i];
		std::shared_ptr<IConsumer2Producer> pIConsumer2Producer = pIObjFactory->createConsumer2Producer();
		std::shared_ptr<IConsumer> pIConsumer = pIObjFactory->createConsumer();
		std::cout << "th " << i << "is processing！" << std::endl;
		if (tileInfos[i].nullTile)
		{
			continue;
		}
		pIConsumer->processRound1(gridInfo, tileInfo, pIConsumer2Producer.get());
		gridIConsumer2Producer.at(tileInfo.gridRow, tileInfo.gridCol) = pIConsumer2Producer;
	}

	//std::cout << "start to process global data" << std::endl;
	std::shared_ptr<IProducer> pIProducer = pIObjFactory->createProducer();
	pIProducer->process(tileInfos,gridIConsumer2Producer);


	for (int i = 0; i < tileInfos.size(); i++)
	{
		TileInfo& tileInfo = tileInfos[i];
		if (tileInfo.nullTile)
		{
			continue;
		}
		std::shared_ptr<IProducer2Consumer> pIProducer2Consumer = pIProducer->toConsumer(tileInfo, gridIConsumer2Producer);
		std::shared_ptr<IConsumer> pIConsumer = pIObjFactory->createConsumer();

		pIConsumer->processRound2(gridInfo, tileInfo, pIProducer2Consumer.get());
	}
}

bool readGridInfo(const char* filePath, GridInfo& gridInfo)
{
	std::ifstream infile;
	infile.open(filePath);
	if (!infile.is_open())
	{
		std::cerr << "Open gridInfo error!" << std::endl;
		return false;
	}
	std::string s;
	std::vector<std::string> input(9);
	int k = 0;
	while (getline(infile, s))
		input[k++] = s;

	size_t pos;
	gridInfo.tileHeight = stod(input[0], &pos);
	gridInfo.tileWidth = stod(input[1], &pos);
	gridInfo.gridHeight = stod(input[2], &pos);
	gridInfo.gridWidth = stod(input[3], &pos);
	gridInfo.grandHeight = stod(input[4], &pos);
	gridInfo.grandWidth = stod(input[5], &pos);

	infile.close();
	return true;
}

void createTileInfoArray(GridInfo& gridInfo, std::vector<TileInfo>& tileInfos) 
{
	int gridHeight = gridInfo.gridHeight;
	int gridWidth = gridInfo.gridWidth;
	tileInfos.resize(gridHeight*gridWidth);

	int grandHeight = gridInfo.grandHeight;
	int grandWidth = gridInfo.grandWidth;
	int tileHeight = gridInfo.tileHeight;
	int tileWidth = gridInfo.tileWidth;
	int height, width;
	for (int tileRow = 0; tileRow < gridHeight; tileRow++)
	{
		for (int tileCol = 0; tileCol < gridWidth; tileCol++)
		{
			height = (grandHeight - tileHeight*tileRow >= tileHeight) ? tileHeight : (grandHeight - tileHeight*tileRow);
			width = (grandWidth - tileWidth*tileCol >= tileWidth) ? tileWidth : (grandWidth - tileWidth*tileCol);
			TileInfo tileInfo;
			tileInfo.gridRow = tileRow;
			tileInfo.gridCol = tileCol;
			tileInfo.height = height;
			tileInfo.width = width;
			std::string path = gridInfo.inputFolder + "\\" + std::to_string(tileRow) + "_" + std::to_string(tileCol) + ".tif";
			const char* cPath = path.c_str();

			//if (access(cPath, F_OK) != -1) //表示文件存在
			//{
				tileInfo.nullTile = false;
				tileInfo.filename = path;
			//}
			//else
			//{
			//	tileInfo.nullTile = true;//该块为nullTile
			//}
			tileInfos[tileRow*gridWidth + tileCol] = tileInfo;
		}
	}

	for (int tileRow = 0; tileRow < gridHeight; tileRow++)
	{
		for (int tileCol = 0; tileCol < gridWidth; tileCol++)
		{
			if (tileInfos[tileRow*gridWidth + tileCol].nullTile) //如果它为nullTile，则它的上下左右，左上，右上，左下，右下
			{
				//判断不是最右边
				if (tileCol != gridWidth-1) 
				{
					tileInfos[tileRow*gridWidth + tileCol + 1].d8Tile |= TILE_LEFT;
					if (tileRow>0)//不是第一行
					{
						tileInfos[(tileRow - 1)*gridWidth + tileCol + 1].d8Tile |= TILE_LEFT_BOTTOM;
					}
					if (tileRow < gridHeight-1)//不是最后一行
					{
						tileInfos[(tileRow + 1)*gridWidth + tileCol + 1].d8Tile |= TILE_LEFT_TOP;
					}

				}
				//判断不是最上边
				if (tileRow != 0)
				{
					tileInfos[(tileRow - 1)*gridWidth + tileCol].d8Tile |= TILE_BOTTOM;
					if (tileCol > 0)
					{
						tileInfos[(tileRow - 1)*gridWidth + tileCol - 1].d8Tile |= TILE_RIGHT_BOTTOM;
					}
					//tile_left_bottom的已经有了
				}
				//判断不是最左边
				if (tileCol != 0)
				{
					tileInfos[tileRow *gridWidth + tileCol-1].d8Tile |= TILE_RIGHT;
					if (tileRow < gridHeight - 1)
					{
						tileInfos[(tileRow + 1) *gridWidth + tileCol - 1].d8Tile |= TILE_RIGHT_TOP;
					}
				}
				//判断不是最下边
				if (tileRow != gridHeight-1)
				{
					tileInfos[(tileRow+1) *gridWidth + tileCol].d8Tile |= TILE_TOP;
				}
			}
		}
	}


}
