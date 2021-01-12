
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
    if ( argc < 4 ) {
        std::cout << "Too few arguments." << std::endl;
		std::cout << "This program determines flow directions over flat surfaces using our proposed parallel algorithm. It can also run test cases on randomly generated DEMs." << std::endl;
		std::cout << "To run test cases on randomly generated DEMs, use the following command: "<<std::endl;
		std::cout << "mpirun -np <PROCESSES_NUMBER> ParallelFlowDir test <OUTPUT_PATH_OF_DEM> <HEIGHT_OF_THE_DEM > <WIDTH_OF_THE_DEM> <OUTPUT_PATH_OF_SEQUENTIAL_FLOW_DIRECTIONS> <TILE_HEIGHT> <TILE_WIDTH> <DIVIDE_FOLDER> <OUTPUT_FOLDER_OF_PARALLEL_FLOW_DIRECIOTNS>" << std::endl;
		std::cout << "This command first generates a Perlin noise DEM with given height and width. Then, Calculating the flow directions of the DEM using the sequential Barnes algorithm and our proposed parallel algorithm. Finally, Comparing the flow directions of the two algorithms. " << std::endl;
		std::cout << "Example usage: mpirun -np 4 ParallelFlowDir test ./test_data/dem.tif 2000 3000 ./test_data/seqFlow/seqFlow.tif 500 800 ./test_data/tileDEM ./test_data/paraFlow " << std::endl;
		std::cout << "To determine flow directions using our proposed parallel algorithm, use the following command: " << std::endl;
		std::cout << "mpirun -np <PROCESSES_NUMBER> ParallelFlowDir parallel <INPUT> <OUTPUT>" << std::endl;
		std::cout << "This command determines flow directions in the given DEM." << std::endl;
		std::cout << "Example usage: mpirun -np 3 ParallelFlowDir parallel ./test_data/ansai_part.txt ./test_data/ansai_flow" << std::endl;
        return -1;
    }

    MPI_Init( &argc, &argv );
    int rank;
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );

    int size;
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    if ( size < 2 ) {
        std::cout << "Specify at least 2 processes, e.g. 'mpirun -np 2 ./ParallelFlowDir'" << std::endl;
        return -1;
    }
    if ( rank == 0 ) {
        char* method = argv[ 1 ];
        std::cout << "Running with = " << size << " processes." << std::endl;
        if ( strcmp( method, "parallel" ) == 0 ) {
            CLI::App app( "Parallel-Flow-Directions Example Program" );
            std::string inputFile;
            std::string outputPath;
            app.add_option( "method", method, "parallel model" )->required();
            app.add_option( "inputFile", inputFile, "A text file which includes the path of DEMs" )->required();
            app.add_option( "outputPath", outputPath, "Path of flow-directios output folder" )->required();
            CLI11_PARSE( app, argc, argv );
            std::cout << "The program is running with paralel mode." << std::endl;
            std::cout << "Input File: " << inputFile << std::endl;
            std::cout << "Path of flow-directios output folder: " << outputPath << std::endl;
            GridInfo gridInfo;
            Timer timer_master;
            timer_master.start();
            Timer timer_overall;
            timer_overall.start();
            std::vector< TileInfo > tileInfos;
            readTXTInfo( inputFile, tileInfos, gridInfo );
            gridInfo.outputFolder = outputPath;
            timer_overall.stop();
            std::cerr << "Preparer time = " << timer_overall.elapsed() << "s" << std::endl;
            ObjectFactory objectFactory;
            hostProcess( gridInfo, tileInfos, &objectFactory );
            timer_master.stop();
            std::cerr << "Total wall-time = " << timer_master.elapsed() << "s" << std::endl;
        }
        else if ( strcmp( method, "test" ) == 0 ) {
            //---------generate DEM with perling DEM--------
            CLI::App app( "Parallel-Flow-Directions Test Program" );
            std::string outputDEMFile;
            int height, width;
            std::string outputsequentialFlow;
            int tileHeight, tileWidth;
            std::string outputtileDEMfolder, outputPath;
            app.add_option( "method", method, "parallel model" )->required();
            app.add_option( "outputDEMFile", outputDEMFile, "Path of GeoTif output file" )->required();
            app.add_option( "height", height, "The height of the generated DEM is a numeric constant" )->required();
            app.add_option( "width", width, "The width of the generated DEM is a numeric constant" )->required();
            app.add_option( "GeoTif of flow-directions", outputsequentialFlow, "Path of GeoTif output file" )->required();
            app.add_option( "tileHeight", tileHeight, "The height of the divied tile is a numeric constant" )->required();
            app.add_option( "tileWidth", tileWidth, "The width of the divied tile is a numeric constant" )->required();
            app.add_option( "outputDEMFolder", outputtileDEMfolder, "Output folder of DEMs" )->required();
            app.add_option( "outputPath", outputPath, "Path of flow-directions output folder" )->required();
            CLI11_PARSE( app, argc, argv );
            std::cout << "The program is running  with test mode." << std::endl;
            std::cout << "Path of DEM output file: " << outputDEMFile << std::endl;
            std::cout << "Height of DEM: " << height << std::endl;
            std::cout << "Width of DEM: " << width << std::endl;
            std::cout << "Output file of flow-directions: " << outputsequentialFlow << std::endl;
            std::cout << "Height of tile: " << tileHeight << std::endl;
            std::cout << "Width of tile: " << tileWidth << std::endl;
            std::cout << "Output folder of DEMs: " << outputtileDEMfolder << std::endl;
            std::cout << "Path of flow-directions output folder: " << outputPath << std::endl;

            std::cout << "1.------Generate DEM!------" << std::endl;
            createPerlinNoiseDEM( outputDEMFile, height, width );
            //----------sequential Barnes flow direction--------
            std::cout << "2.------Sequential flow directions!------" << std::endl;
            PerformAlgorithm( outputDEMFile, outputsequentialFlow );
            //-----------divide tiles--------------------
            std::cout << "3.------Divided tiles!------" << std::endl;
            generateTiles( outputDEMFile.c_str(), tileHeight, tileWidth, outputtileDEMfolder.c_str() );
            std::cout << "4.------Parallel computing!------" << std::endl;
            std::string inputFile = outputtileDEMfolder + "/" + "tileInfo.txt";
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
            std::cerr << "Preparer time = " << timer_overall.elapsed() << "s" << std::endl;
            ObjectFactory objectFactory;
            hostProcess( gridInfo, tileInfos, &objectFactory );
            std::cout << "5.------Merge images!-----" << std::endl;
            gridInfo.grandHeight = height;
            gridInfo.grandWidth = width;
            gridInfo.tileHeight = tileHeight;
            gridInfo.tileWidth = tileWidth;
            std::string outputFile = outputPath + "/merge.tif";
            mergeTiles( gridInfo, outputFile.c_str() );
            //-----------compare  results---------------
            std::cout << "6.------Compare results!------" << std::endl;
            comPareResults( outputsequentialFlow, outputFile );
            timer_master.stop();
            std::cout << "Total wall-time = " << timer_master.elapsed() << "s" << std::endl;
        }
        else {
            std::cout << "Parameters error! You should specify the 'parallel' mode or the 'test' mode." << std::endl;
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
