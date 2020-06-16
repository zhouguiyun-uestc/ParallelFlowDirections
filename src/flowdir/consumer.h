#ifndef PARADEM_CONSUMER_H
#define PARADEM_CONSUMER_H

#include <paradem/i_consumer.h>
#include<stdint.h>
#include<deque>
#include"GridCell.h"
#include "producer_2_consumer.h"
#include<map>
#include"paradem/raster.h"
#include<list>
#include"host.h"
#include"paradem/timer.h"
#include"paradem/pStorage.h"
struct Point
{
	int row, col;
	int F, G, H;
	Point * parent;//
	Point(int _row, int _col) :row(_row), col(_col), F(0), G(0), H(0), parent(NULL)
	{
	}
};
class Consumer:public IConsumer
{
public:
	//StorageType storages;
	Raster<float> dem;
	Raster<int> flowdirs;
	Raster<int> labels;
	Raster<int> highEdgeMask;
	Raster<int> lowEdgeMask;
	std::vector<int>IdBounderyCells;
	//Raster<int> IdBounderyCells;
	std::vector<std::map<int, int>> lowEdgeGraph;
	std::vector<std::map<int, int>> highEdgeGraph;
	std::vector<int> flatHeight;
	Raster<int> flatMask;
	bool FlatTile = false;
	bool NoFlat = false;
	int gridRow;
	int gridCol;
	int gridHeight;
	int gridWidth;
	std::list<Point *> openList;
	std::list<Point*> closeList;
	Timer timer_io;
	Timer timer_calc;
	
	
public:
	virtual bool processRound1(const GridInfo& gridInfo, TileInfo& tileInfo, IConsumer2Producer* pIConsumer2Producer);
	virtual bool processRound2(const GridInfo& gridInfo, const TileInfo& tileInfo, IProducer2Consumer* pIProducer2Consumer);
public:
	virtual void free();
	void FlowAssign(Raster<int> &flowdirs, uint8_t d8Tile);
	int D8FlowDir(const int & row, const int & col, const int &height, const int &width, uint8_t d8Tile);
	void LabelFlatArea(bool firstTime, Raster<int>& flowdirs, Raster<int>& labels, std::vector<int>&flatHeight,
		Raster<int> &highEdgeMask, Raster<int> &lowEdgeMask, std::vector<int> &IdBounderyCells);
	void FindFlatEdge(std::deque<GridCell>& low_edges, std::deque<GridCell>& high_edges, Raster<int>&flowdirs, std::deque<GridCell>& uncertainEdges);
	void LabelCell(const int & row, const int & col, const int label, Raster<int> &labels);
	void BuildAwaySpillPnt(std::deque<GridCell> low_edges, Raster<int> &flowdirs, Raster<int> &labels, Raster<int> & lowEdgeMask);
	void BuildAwayGradient(std::deque<GridCell> high_edges, Raster<int> &flowdirs, Raster<int>&labels, Raster<int>&highEdgeMask, std::vector<int>&flatHeight);
	void BuildPartGraph(const int groupNumber, bool firstTime, std::vector<int>&IdBounderyCells, Raster<int> &labels, Raster<int>&lowEdgeMask, Raster<int>&highEdgeMask, Raster<int> &flowdirs, std::deque<GridCell>& uncertainEdges);
	void block2block(std::vector<GridCell>& boundarySet, std::vector<int>& bulk, std::vector<int> &IdBounderyCells, Raster<int>&lowEdgeMask, Raster<int>&highEdgeMask, Raster<int>&labels);
	void bc2block(GridCell source, std::vector<int32_t>& minimum_distance, Raster<int> &labels);//j代表该块的第j个栅格作为源栅格
	void ExternalGraph2(std::vector<int>& bulk, int & source_bulk, int & object_bulk, const std::vector<GridCell>& boundarySet, std::vector<std::vector<int32_t>>& minimum_distances, const int cell_1, const int cell_n, std::vector<int>&IdBounderyCells, int width, int height);
	bool InArray(int current, int qi, int zhong); 
	void RenewMask(const TileInfo & tile, IProducer2Consumer * Ip2c, Raster<int> &flowdirs, std::vector<int> &IdBounderyCells,
		Raster<int> &highEdgeMask, Raster<int> &lowEdgeMask, Raster<int> &labels, std::vector<int>&flatHeight);
	void D8FlowFlats(Raster<int> &flowdirs, Raster<int> &labels);
	int8_t D8MaskedFlowDir(const int row, const int col, Raster<int> &labels);
	void ModifyBoundaryFlowDir(const TileInfo & tileInfo, IProducer2Consumer * Ip2c, Raster<int> &flowdirs,
		Raster<int>&labels, std::vector<int> &IdBounderyCells, Raster<int>&lowEdgeMask, std::vector<int> &flatHeight);
	void SaveFlowDir(const GridInfo& gridInfo, const TileInfo & tileInfo, Raster<int>&flowdirs);
//	void findPotentialPnt(Raster<int32_t>& minimum_distance, std::vector<GridCell>& boundarySet, int qi_z, int zhong_z, int m_iden, Raster<int>&IdBounderyCells);
	int xy2positionNum(const int x, const int y, const int width,const int height);
	//void positionNum2xy(const int num, int &x, int &y, const int width, const int height);
public:
	struct Mask
	{
		int mask;
		GridCell cell;

	};
	struct cmp
	{
		bool operator()(Mask &a, Mask &b) const
		{
			return a.mask > b.mask;	
		}
	};
	




};

#endif
