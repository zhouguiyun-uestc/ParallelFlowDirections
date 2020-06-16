#include <paradem/tool.h>
#include "object_factory.h"
#include <paradem/raster.h>
#include <paradem/gdal.h>
#include<vector>
#include<iostream>
#include"mpi.h"
#include<assert.h>
#include"consumer_2_producer.h"
#include <cereal/types/vector.hpp>
#include <cereal/types/map.hpp>
#include <cereal/archives/binary.hpp>
#include"host.h"
#include<map>
#include"producer_2_consumer.h"
#include<paradem/timer.h>
#include<paradem/memory.h>
#include<paradem/timeInfo.h>
#include"consumer.h"
#include"exception"

// @author: Lihui Song
// @citation: Parallel assignment of flow directions over flat surfaces in massive digital elevation models



int main(int argc,char **argv)
{
	MPI_Init(&argc, &argv);
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	if (rank == 0)
	{
		std::string inputFile;
		std::string outputFile;
		inputFile = argv[1];
		outputFile = argv[2];	
		std::cerr << "inputFile" << inputFile << std::endl;
		std::cerr << "outputFile" << outputFile << std::endl;
		GridInfo gridInfo;
		Timer timer_master;
		timer_master.start();
		Timer timer_overall;
		timer_overall.start();
		std::vector<TileInfo> tileInfos;
		readTXTInfo(inputFile,tileInfos, gridInfo);
		gridInfo.outputFolder = outputFile;
		timer_overall.stop();
		std::cerr << "t Preparer time = " << timer_overall.elapsed() << "s" << std::endl;
		ObjectFactory objectFactory;
		hostProcess(gridInfo, tileInfos, &objectFactory);

		timer_master.stop();
		std::cerr << "t Total wall-time=" << timer_master.elapsed() << "s" << std::endl;
	}
	else
	{
		storageTile storage;
		ObjectFactory  pIObjFactory;
		GridInfo gridInfo;
		while (true)
		{			
			MPI_Status status;
			MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);			
			int the_job = status.MPI_TAG;
			if (the_job == TagFirst)
			{
				Timer timer_overall;
				timer_overall.start();
				TileInfo tileInfo;
				CommRecv(&tileInfo, &gridInfo, 0);	
				std::shared_ptr<IConsumer2Producer> pIConsumer2Producer = pIObjFactory.createConsumer2Producer();
				std::shared_ptr<IConsumer> pIConsumer = pIObjFactory.createConsumer();
				pIConsumer->processRound1(gridInfo, tileInfo, pIConsumer2Producer.get());	
				timer_overall.stop();
				long vmpeak, vmhwm;
				ProcessMemUsage(vmpeak, vmhwm);
				Consumer2Producer *p = (Consumer2Producer*)pIConsumer2Producer.get();
				p->time_info = TimeInfo(((Consumer*)pIConsumer.get())->timer_calc.elapsed(),timer_overall.elapsed(), ((Consumer*)pIConsumer.get())->timer_io.elapsed(),vmpeak,vmhwm);
  				CommSend(p, nullptr, 0, ObjectFirst);					
			}
			else if (the_job == TagSecond)
			{
				Timer timer_overall;
				timer_overall.start();
				TileInfo tileInfo;
				std::shared_ptr<IProducer2Consumer> pIProducer2Consumer;
				Producer2Consumer pP2C;
				CommRecv(&tileInfo, &pP2C,&gridInfo, 0);
				std::shared_ptr<IConsumer> pIConsumer = pIObjFactory.createConsumer();
				pIConsumer->processRound2(gridInfo, tileInfo, &pP2C);
				timer_overall.stop();
				long vmpeak, vmhwm;
				ProcessMemUsage(vmpeak, vmhwm);
				TimeInfo temp(((Consumer*)pIConsumer.get())->timer_calc.elapsed(), timer_overall.elapsed(), ((Consumer*)pIConsumer.get())->timer_io.elapsed(), vmpeak, vmhwm);
				CommSend(&temp, nullptr, 0, ObjectSecond);
			}
			else if (the_job==TagNull)
			{
				break;
			}
		}	
	}
	MPI_Finalize();


	return 0;
}