#ifndef HOST_H
#define HOST_H

#include "consumer_2_producer.h"

#include <cereal/archives/binary.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/vector.hpp>

#include <paradem/grid_info.h>
#include <paradem/i_object_factory.h>
#include <paradem/raster.h>
#include <paradem/tile_info.h>
#include <paradem/tool.h>

#include <mpi.h>
#include <type_traits>
#include <vector>

#define TagFirst 1
#define TagSecond 2
#define TagNull 3
#define ObjectFirst 4
#define ObjectSecond 5

typedef uint64_t comm_count_type;
static comm_count_type bytes_sent = 0;  ///< Number of bytes sent
static comm_count_type bytes_recv = 0;  ///< Number of bytes received
typedef std::vector< char > msg_type;


template < class T, class U > void CommRecv( T* a, U* b, int from ) {
    MPI_Status status;
    if ( from == -1 )
        from = MPI_ANY_SOURCE;
    MPI_Probe( from, MPI_ANY_TAG, MPI_COMM_WORLD, &status );
    int msg_size;
    MPI_Get_count( &status, MPI_CHAR, &msg_size );
    int receive_Id = status.MPI_SOURCE;
    std::stringstream ss( std::stringstream::in | std::stringstream::out | std::stringstream::binary );
    ss.unsetf( std::ios_base::skipws );
    char* buf = ( char* )malloc( msg_size );
    MPI_Comm_set_errhandler( MPI_COMM_WORLD, MPI_ERRORS_RETURN );
    int error_code = MPI_Recv( buf, msg_size, MPI_CHAR, receive_Id, MPI_ANY_TAG, MPI_COMM_WORLD, &status );
    if ( error_code != MPI_SUCCESS ) {
        char error_string[ BUFSIZ ];
        int length_of_error_string, error_class;
        MPI_Error_class( error_code, &error_class );
        MPI_Error_string( error_class, error_string, &length_of_error_string );
        fprintf( stderr, "%s\n", error_string );
        MPI_Abort( MPI_COMM_WORLD, error_code );
    }
    bytes_recv += msg_size;
    ss.write( buf, msg_size );
    free( buf );
    buf = NULL;
    {
        cereal::BinaryInputArchive archive( ss );
        archive( *a );
        if ( b != nullptr )
            archive( *b );
    }
}

template < class T, class U, class V > void CommRecv( T* a, U* b, V* v, int from ) {
    MPI_Status status;
    if ( from == -1 )
        from = MPI_ANY_SOURCE;
    MPI_Probe( from, MPI_ANY_TAG, MPI_COMM_WORLD, &status );
    int msg_size;
    MPI_Get_count( &status, MPI_CHAR, &msg_size );
    std::stringstream ss( std::stringstream::in | std::stringstream::out | std::stringstream::binary );
    ss.unsetf( std::ios_base::skipws );
    char* buf = ( char* )malloc( msg_size );
    MPI_Comm_set_errhandler( MPI_COMM_WORLD, MPI_ERRORS_RETURN );
    int receive_Id = status.MPI_SOURCE;
    int error_code = MPI_Recv( buf, msg_size, MPI_CHAR, receive_Id, MPI_ANY_TAG, MPI_COMM_WORLD, &status );
    if ( error_code != MPI_SUCCESS ) {
        char error_string[ BUFSIZ ];
        int length_of_error_string, error_class;
        MPI_Error_class( error_code, &error_class );
        MPI_Error_string( error_class, error_string, &length_of_error_string );
        fprintf( stderr, "%s\n", error_string );
        MPI_Error_string( error_code, error_string, &length_of_error_string );
        fprintf( stderr, "%s\n", error_string );
        fprintf( stderr, "%d\n", error_code );
        fprintf( stderr, "%d,%d,%d\n", status.MPI_ERROR, status.MPI_SOURCE, status.MPI_TAG );
        MPI_Abort( MPI_COMM_WORLD, error_code );
    }
    bytes_recv += msg_size;
    ss.write( buf, msg_size );
    free( buf );
    buf = NULL;
    {
        cereal::BinaryInputArchive archive( ss );
        archive( *a );
        archive( *b );
        archive( *v );
    }
}

template < class T > void CommRecv( T* a, std::nullptr_t, int from ) {
    CommRecv( a, ( int* )nullptr, from );
}

template < class T, class U > msg_type CommPrepare( const T* a, const U* b ) {
    std::vector< char > omsg;
    std::stringstream ss( std::stringstream::in | std::stringstream::out | std::stringstream::binary );
    ss.unsetf( std::ios_base::skipws );
    cereal::BinaryOutputArchive archive( ss );
    archive( *a );
    if ( b != nullptr )
        archive( *b );
    std::copy( std::istream_iterator< char >( ss ), std::istream_iterator< char >(), std::back_inserter( omsg ) );
    return omsg;
}

template < class T, class U, class V > msg_type CommPrepare( const T* a, const U* b, const V* v ) {
    std::vector< char > omsg;
    std::stringstream ss( std::stringstream::in | std::stringstream::out | std::stringstream::binary );
    ss.unsetf( std::ios_base::skipws );
    cereal::BinaryOutputArchive archive( ss );
    archive( *a );
    archive( *b );
    archive( *v );
    std::copy( std::istream_iterator< char >( ss ), std::istream_iterator< char >(), std::back_inserter( omsg ) );
    return omsg;
}

template < class T, class U > void CommSend( const T* a, const U* b, int dest, int tag ) {
    auto omsg = CommPrepare( a, b );
    bytes_sent += omsg.size();
    MPI_Send( omsg.data(), omsg.size(), MPI_CHAR, dest, tag, MPI_COMM_WORLD );
}

template < class T > void CommSend( const T* a, std::nullptr_t, int dest, int tag ) {
    CommSend( a, ( int* )nullptr, dest, tag );
}

template < class T > void CommSend( const T* a, std::nullptr_t, int dest, int tag, int rank ) {
    CommSend( a, ( int* )nullptr, dest, tag, rank );
}

template < class T, class U, class V > void CommSend( const T* a, const U* b, const V* v, int dest, int tag ) {
    auto omsg = CommPrepare( a, b, v );
    bytes_sent += omsg.size();
    MPI_Send( omsg.data(), omsg.size(), MPI_CHAR, dest, tag, MPI_COMM_WORLD );
}

void CommISend( msg_type& msg, int dest, int tag );
comm_count_type CommBytesSent();
comm_count_type CommBytesRecv();
void CommBytesReset();
void hostProcess( GridInfo& gridInfo, std::vector< TileInfo >& tileInfos, IObjectFactory* pIObjFactory );

#endif