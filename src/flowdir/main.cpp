
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

#include <cereal/archives/binary.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/vector.hpp>

#include <assert.h>
#include <exception>
#include <iostream>
#include <map>
#include <vector>

int main( int argc, char** argv ) {
    MPI_Init( &argc, &argv );
    int rank;
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    if ( rank == 0 ) {
        char* method = argv[ 1 ];
        if ( strcmp( method, "parallel" ) == 0 ) {
            std::string inputFile = argv[ 2 ];
            std::string outputPath = argv[ 3 ];  // output flow directions
            std::cerr << "inputFile" << inputFile << std::endl;
            std::cerr << "outputPath" << outputPath << std::endl;
            GridInfo gridInfo;
            Timer timer_master;
            timer_master.start();
            Timer timer_overall;
            timer_overall.start();
            std::vector< TileInfo > tileInfos;
            readTXTInfo( inputFile, tileInfos, gridInfo );
            gridInfo.outputFolder = outputPath;
            timer_overall.stop();
            std::cerr << "t Preparer time = " << timer_overall.elapsed() << "s" << std::endl;
            ObjectFactory objectFactory;
            hostProcess( gridInfo, tileInfos, &objectFactory );
            timer_master.stop();
            std::cerr << "t Total wall-time=" << timer_master.elapsed() << "s" << std::endl;
        }
        else if ( strcmp( method, "test" ) == 0 ) {
            //---------generate DEM with perling DEM--------
            std::cout << "1.generate DEM!" << std::endl;
            std::string outputDEMFile = argv[ 2 ];
            int height = std::stoi( argv[ 3 ] );
            int width = std::stoi( argv[ 4 ] );
            createPerlinNoiseDEM( outputDEMFile, height, width );
            //----------sequential Barnes flow direction--------
            std::cout << "2.sequential flow directions!" << std::endl;
            std::string outputsequentialFlow = argv[ 5 ];
            PerformAlgorithm( outputDEMFile, outputsequentialFlow );
            //-----------divide tiles--------------------
            std::cout << "3.divided tiles!" << std::endl;
            int tileHeight = std::stoi( argv[ 6 ] );
            int tileWidth = std::stoi( argv[ 7 ] );
            std::string outputtileDEMfolder = argv[ 8 ];
            generateTiles( outputDEMFile.c_str(), tileHeight, tileWidth, outputtileDEMfolder.c_str() );
            std::cout << "4.parallel computing!" << std::endl;
            std::string inputFile = outputtileDEMfolder + "/" + "tileInfo.txt";
            std::string outputPath = argv[ 9 ];  // output flow directions
            GridInfo gridInfo;
            Timer timer_master;
            timer_master.start();
            Timer timer_overall;
            timer_overall.start();
            std::vector< TileInfo > tileInfos;
            readTXTInfo( inputFile, tileInfos, gridInfo );
            gridInfo.inputFolder = outputtileDEMfolder;
            gridInfo.outputFolder = outputPath;
            timer_overall.stop();
            std::cerr << "t Preparer time = " << timer_overall.elapsed() << "s" << std::endl;
            ObjectFactory objectFactory;
            hostProcess( gridInfo, tileInfos, &objectFactory );
            std::cout << "5.merge flow tif!" << std::endl;
            gridInfo.grandHeight = height;
            gridInfo.grandWidth = width;
            gridInfo.tileHeight = tileHeight;
            gridInfo.tileWidth = tileWidth;
            std::string outputFile = outputPath + "/merge.tif";
            mergeTiles( gridInfo, outputFile.c_str() );
            //-----------compare  results---------------
            std::cout << "6.compare results!" << std::endl;
            comPareResults( outputsequentialFlow, outputFile );
            timer_master.stop();
            std::cerr << "t Total wall-time=" << timer_master.elapsed() << "s" << std::endl;
        }
        else {
            std::cout << " parameter error!" << std::endl;
            return -1;
        }
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