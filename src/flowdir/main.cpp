
#include "consumer.h"
#include "consumer_2_producer.h"
#include "object_factory.h"
#include "producer_2_consumer.h"

#include <paradem/gdal.h>
#include <paradem/memory.h>
#include <paradem/raster.h>
#include <paradem/timeInfo.h>
#include <paradem/timer.h>
#include <paradem/tool.h>

#include <CLI/CLI.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/vector.hpp>

#include <assert.h>
#include <exception>
#include <iostream>
#include <map>
#include <vector>

int main( int argc, char** argv ) {
    if ( argc < 3 ) {
        std::cout << "Too few arguments." << std::endl;
		std::cout << "This program determines flow directions over flat surfaces using our proposed parallel algorithm" << std::endl;
	    std::cout << "mpirun -np <PROCESSES_NUMBER> ParallelFlowDir <INPUT> <OUTPUT>" << std::endl;
	    std::cout << "This command determines flow directions in the given DEM " << std::endl;
                    std::cout << "Example usage: mpirun -np 3 ParallelFlowDir ./test_data/ansai_part.txt ./test_data/ansai_flow" << std::endl;
        return -1;
    }

    MPI_Init( &argc, &argv );
    int rank;
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );

    int size;
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    if ( size < 2 ) {
		std::cout << "Specify at least 2 processes, e.g. 'mpirun -np 2 ParallelFlowDir ./test_data/ansai_part.txt ./test_data/ansai_flow'" << std::endl;
        return -1;
    }
    if ( rank == 0 ) {
		CLI::App app("Parallel-Flow-Directions Example Program");
		std::string inputFile;
		std::string outputPath;
		app.add_option("inputFile", inputFile, "A text file which includes the path of DEMs")->required();
		app.add_option("outputPath", outputPath, "Path of flow-directios output folder")->required();
		CLI11_PARSE(app, argc, argv);
		std::cout << "The program is running with paralel mode." << std::endl;
		std::cout << "Input File: " << inputFile << std::endl;
		std::cout << "Path of flow-directios output folder: " << outputPath << std::endl;
		GridInfo gridInfo;
		Timer timer_master;
		timer_master.start();
		Timer timer_overall;
		timer_overall.start();
		std::vector< TileInfo > tileInfos;
		readTXTInfo(inputFile, tileInfos, gridInfo);
		gridInfo.outputFolder = outputPath;
		timer_overall.stop();
		std::cerr << "Preparer time = " << timer_overall.elapsed() << "s" << std::endl;
		ObjectFactory objectFactory;
		hostProcess(gridInfo, tileInfos, &objectFactory);
		timer_master.stop();
		std::cerr << "Total wall-time = " << timer_master.elapsed() << "s" << std::endl;
    }
    else {
        ObjectFactory pIObjFactory;
        GridInfo gridInfo;
        while ( true ) {
            MPI_Status status;
            MPI_Probe( 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status );
            int the_job = status.MPI_TAG;
            if ( the_job == TagFirst ) {
                Timer timer_overall;
                timer_overall.start();
                TileInfo tileInfo;
                CommRecv( &tileInfo, &gridInfo, 0 );
                std::shared_ptr< IConsumer2Producer > pIConsumer2Producer = pIObjFactory.createConsumer2Producer();
                std::shared_ptr< IConsumer > pIConsumer = pIObjFactory.createConsumer();
                pIConsumer->processRound1( gridInfo, tileInfo, pIConsumer2Producer.get() );
                timer_overall.stop();
                long vmpeak, vmhwm;
                ProcessMemUsage( vmpeak, vmhwm );
                Consumer2Producer* p = ( Consumer2Producer* )pIConsumer2Producer.get();
                p->time_info = TimeInfo( ( ( Consumer* )pIConsumer.get() )->timer_calc.elapsed(), timer_overall.elapsed(), ( ( Consumer* )pIConsumer.get() )->timer_io.elapsed(), vmpeak, vmhwm );
                CommSend( p, nullptr, 0, ObjectFirst );
            }
            else if ( the_job == TagSecond ) {
                Timer timer_overall;
                timer_overall.start();
                TileInfo tileInfo;
                std::shared_ptr< IProducer2Consumer > pIProducer2Consumer;
                Producer2Consumer pP2C;
                CommRecv( &tileInfo, &pP2C, &gridInfo, 0 );
                std::shared_ptr< IConsumer > pIConsumer = pIObjFactory.createConsumer();
                pIConsumer->processRound2( gridInfo, tileInfo, &pP2C );

                timer_overall.stop();
                long vmpeak, vmhwm;
                ProcessMemUsage( vmpeak, vmhwm );
                TimeInfo temp( ( ( Consumer* )pIConsumer.get() )->timer_calc.elapsed(), timer_overall.elapsed(), ( ( Consumer* )pIConsumer.get() )->timer_io.elapsed(), vmpeak, vmhwm );
                CommSend( &temp, nullptr, 0, ObjectSecond );
            }
            else if ( the_job == TagNull ) {
                break;
            }
        }
    }
    MPI_Finalize();
    return 0;
}
