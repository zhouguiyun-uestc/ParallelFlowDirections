
#include "host.h"
#include "consumer_2_producer.h"
#include "producer.h"
#include "producer_2_consumer.h"

#include <paradem/memory.h>
#include <paradem/timeInfo.h>
#include <paradem/timer.h>

void CommISend( msg_type& msg, int dest, int tag ) {
    MPI_Request request;
    bytes_sent += msg.size();
    MPI_Isend( msg.data(), msg.size(), MPI_BYTE, dest, tag, MPI_COMM_WORLD, &request );
}
comm_count_type CommBytesSent() {
    return bytes_sent;
}

comm_count_type CommBytesRecv() {
    return bytes_recv;
}

void CommBytesReset() {
    bytes_recv = 0;
    bytes_sent = 0;
}
void hostProcess( GridInfo& gridInfo, std::vector< TileInfo >& tileInfos, IObjectFactory* pIObjFactory ) {
    Timer timer_overall;
    timer_overall.start();
    int good_to_go = 1;
    std::vector< msg_type > msgs;
    int size;
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    const int active_consumer_limit = size - 1;
    int jobs_out = 0;
    MPI_Bcast( &good_to_go, 1, MPI_INT, 0, MPI_COMM_WORLD );
    for ( size_t i = 0; i < tileInfos.size(); i++ ) {
        TileInfo& tileInfo = tileInfos[ i ];
        if ( tileInfo.nullTile ) {
            continue;
        }
        msgs.push_back( CommPrepare( &tileInfos[ i ], &gridInfo ) );
        CommISend( msgs.back(), ( jobs_out % active_consumer_limit ) + 1, TagFirst );
        jobs_out++;
    }
    Grid< std::shared_ptr< IConsumer2Producer > > gridIConsumer2Producer;
    gridIConsumer2Producer.init( gridInfo.gridHeight, gridInfo.gridWidth );
    while ( jobs_out-- ) {
        Consumer2Producer pC2P;
        CommRecv( &pC2P, nullptr, -1 );
        gridIConsumer2Producer.at( pC2P.gridRow, pC2P.gridCol ) = std::make_shared< Consumer2Producer >( pC2P );
    }
    std::cerr << "First stage Tx = " << CommBytesSent() << " B" << std::endl;
    std::cerr << "First stage Rx = " << CommBytesRecv() << " B" << std::endl;
    CommBytesReset();
    TimeInfo time_first_total;
    double maxTimeoverall = 0;
    double minTimeoverall = 1000000000000;
    double maxTimecal = 0;
    double minTimecal = 1000000000000;
    for ( int row = 0; row < gridInfo.gridHeight; row++ ) {
        for ( int col = 0; col < gridInfo.gridWidth; col++ ) {
            if ( tileInfos[ row * gridInfo.gridWidth + col ].nullTile ) {
                continue;
            }
            time_first_total += ( ( Consumer2Producer* )gridIConsumer2Producer.at( row, col ).get() )->time_info;
            maxTimeoverall = std::max( maxTimeoverall, ( ( Consumer2Producer* )gridIConsumer2Producer.at( row, col ).get() )->time_info.overall );
            minTimeoverall = std::min( minTimeoverall, ( ( Consumer2Producer* )gridIConsumer2Producer.at( row, col ).get() )->time_info.overall );
            maxTimecal = std::max( maxTimecal, ( ( Consumer2Producer* )gridIConsumer2Producer.at( row, col ).get() )->time_info.calc );
            minTimecal = std::min( minTimecal, ( ( Consumer2Producer* )gridIConsumer2Producer.at( row, col ).get() )->time_info.calc );
        }
    }
    std::cout << "MaxTimeOverall: " << maxTimeoverall << ",minTimeoverall: " << minTimeoverall << ",avgTimeoverall: " << time_first_total.overall / active_consumer_limit << std::endl;
    std::cout << "MaxTimecal: " << maxTimecal << ",minTimecal: " << minTimecal << ",avgTimecal: " << time_first_total.calc / active_consumer_limit << std::endl;
    std::shared_ptr< IProducer > pIProducer = pIObjFactory->createProducer();
    pIProducer->process( tileInfos, gridIConsumer2Producer );
    jobs_out = 0;

    for ( size_t i = 0; i < tileInfos.size(); i++ ) {
        TileInfo& tileInfo = tileInfos[ i ];
        if ( tileInfo.nullTile ) {
            continue;
        }
        msgs = std::vector< msg_type >();
        std::shared_ptr< IProducer2Consumer > pIProducer2Consumer = pIProducer->toConsumer( tileInfo, gridIConsumer2Producer );
        Producer2Consumer* pP2c = ( Producer2Consumer* )pIProducer2Consumer.get();
        CommSend( &tileInfos[ i ], pP2c, &gridInfo, ( jobs_out % active_consumer_limit ) + 1, TagSecond );
        jobs_out++;
    }

    maxTimeoverall = 0;
    minTimeoverall = 1000000000000;
    maxTimecal = 0;
    minTimecal = 1000000000000;
    TimeInfo time_second_total;
    while ( jobs_out-- ) {
        TimeInfo temp;
        CommRecv( &temp, nullptr, -1 );
        time_second_total += temp;

        maxTimeoverall = std::max( maxTimeoverall, temp.overall );
        minTimeoverall = std::min( minTimeoverall, temp.overall );
        maxTimecal = std::max( maxTimecal, temp.calc );
        minTimecal = std::min( minTimecal, temp.calc );
    }
    std::cout << "MaxTimeOverall: " << maxTimeoverall << ",minTimeoverall: " << minTimeoverall << ",avgTimeoverall: " << time_second_total.overall / active_consumer_limit << std::endl;
    std::cout << "MaxTimecal: " << maxTimecal << ",minTimecal: " << minTimecal << ",avgTimecal: " << time_second_total.calc / active_consumer_limit << std::endl;
    for ( int i = 1; i < size; i++ ) {
        int temp;
        CommSend( &temp, nullptr, i, TagNull );
    }
    timer_overall.stop();
    std::cerr << "First stage total overall time = " << time_first_total.overall << " s" << std::endl;
    std::cerr << "First stage total io time = " << time_first_total.io << " s" << std::endl;
    std::cerr << "First stage total calc time = " << time_first_total.calc << " s" << std::endl;
    std::cerr << "First stage peak child VmPeak = " << time_first_total.vmpeak << std::endl;
    std::cerr << "First stage peak child VmHWM = " << time_first_total.vmhwm << std::endl;
    std::cerr << "Second stage Tx = " << CommBytesSent() << " B" << std::endl;
    std::cerr << "Second stage Rx = " << CommBytesRecv() << " B" << std::endl;
    std::cerr << "Second stage total overall time = " << time_second_total.overall << " s" << std::endl;
    std::cerr << "Second stage total IO time = " << time_second_total.io << " s" << std::endl;
    std::cerr << "Second stage total calc time = " << time_second_total.calc << " s" << std::endl;
    std::cerr << "Second stage peak child VmPeak = " << time_second_total.vmpeak << std::endl;
    std::cerr << "Second stage peak child VmHWM = " << time_second_total.vmhwm << std::endl;
    std::cerr << "Producer overall time = " << timer_overall.elapsed() << " s" << std::endl;
    std::cerr << "Producer calc time = " << ( ( Producer* )pIProducer.get() )->timer_calc.elapsed() << " s" << std::endl;
    long vmpeak, vmhwm;
    ProcessMemUsage( vmpeak, vmhwm );
    std::cerr << "Producer's VmPeak = " << vmpeak << std::endl;
    std::cerr << "Producer's VmHWM = " << vmhwm << std::endl;
}
