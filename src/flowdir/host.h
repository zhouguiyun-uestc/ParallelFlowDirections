#ifndef HOST_H
#define HOST_H
#include<paradem/grid_info.h>
#include <vector>
#include<paradem/tile_info.h>
#include<paradem/i_object_factory.h>
#include<mpi.h>
#include <cereal/types/vector.hpp>
#include <cereal/types/map.hpp>
#include <cereal/archives/binary.hpp>
#include<assert.h>
#include<paradem/raster.h>
#include<paradem/tool.h>
#include "consumer_2_producer.h"
#include <type_traits>

#define TagFirst 1
#define TagSecond 2
#define TagNull 3
#define ObjectFirst 4
#define ObjectSecond 5

typedef uint64_t comm_count_type;
static comm_count_type bytes_sent = 0; ///< Number of bytes sent
static comm_count_type bytes_recv = 0; ///< Number of bytes received
typedef std::vector<char> msg_type;

//接收最多两个对象并反序列化它们。
template<class T, class U>
void CommRecv(T* a, U* b, int from) {
	MPI_Status status;
	
	if (from == -1)
		from = MPI_ANY_SOURCE;

	//阻塞探测消息
	MPI_Probe(from, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

	
	//得到消息的大小
	int msg_size;
	MPI_Get_count(&status, MPI_CHAR, &msg_size);
	int receive_Id = status.MPI_SOURCE;
	std::stringstream ss(std::stringstream::in | std::stringstream::out | std::stringstream::binary);
	ss.unsetf(std::ios_base::skipws);

	// Allocate a buffer to hold the incoming data
	char* buf = (char*)malloc(msg_size);
	/*if (msg_size == 0)
	{
		//std::cout << "msg_size=0" << std::endl;
	}
	if (buf == NULL)
	{
		//std::cout << "three buf is null" << std::endl;
	}*/
	assert(buf != NULL);
	//std::cout << "tagfirst" << "---" << &buf << "--msg_size" << msg_size << std::endl;
	MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

	int error_code = MPI_Recv(buf, msg_size, MPI_CHAR, receive_Id, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	
	if (error_code != MPI_SUCCESS) 
	{ 
		char error_string[BUFSIZ];
		int length_of_error_string, error_class;
		MPI_Error_class(error_code, &error_class);
		MPI_Error_string(error_class, error_string, &length_of_error_string);
		fprintf(stderr, "%s\n", error_string);
		fprintf(stderr, "%s\n", error_class);
		fprintf(stderr, "%d\n", error_code);
		fprintf(stderr, "%d,%d,%d\n", status.MPI_ERROR, status.MPI_SOURCE, status.MPI_TAG);
		MPI_Abort(MPI_COMM_WORLD, error_code);
		
	}


	bytes_recv += msg_size;

	ss.write(buf, msg_size);
	free(buf);
	buf = NULL;//拴住野指针

	{
		cereal::BinaryInputArchive archive(ss);
		archive(*a);
		if (b != nullptr)
			archive(*b);
	}
}
template<class T, class U,class V>
void CommRecv(T* a, U* b,V*v, int from) {
	MPI_Status status;

	if (from == -1)
		from = MPI_ANY_SOURCE;

	MPI_Probe(from, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

	int msg_size;
	//MPI_Get_count(&status, MPI_BYTE, &msg_size);.
	MPI_Get_count(&status,MPI_CHAR, &msg_size);

	std::stringstream ss(std::stringstream::in | std::stringstream::out | std::stringstream::binary);
	ss.unsetf(std::ios_base::skipws);

	// Allocate a buffer to hold the incoming data
	char* buf = (char*)malloc(msg_size);
	if (msg_size == 0)
	{
		//std::cout << "msg_size=0" << std::endl;
	}
	if (buf == NULL)
	{
		//std::cout << "three buf is null" << std::endl;
	}
	assert(buf != NULL);

	MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
	//int error_code=MPI_Recv(buf, msg_size, MPI_BYTE, from, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	//int error_code = MPI_Recv(buf, msg_size, MPI_BYTE, from, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	int receive_Id = status.MPI_SOURCE;
	int error_code = MPI_Recv(buf, msg_size, MPI_CHAR, receive_Id,MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	if (error_code != MPI_SUCCESS)
	{
		//std::cout << "receive_id" << receive_Id << std::endl;
		//std::cout << "tagfirst" << "---" << &buf << "--msg_size" << msg_size << ",,,," << status.MPI_SOURCE << std::endl;
		//std::cout << "tagfirst" << "---" << &buf << "--msg_size" << msg_size << std::endl;
		char error_string[BUFSIZ];
		int length_of_error_string, error_class;
		MPI_Error_class(error_code, &error_class);
		MPI_Error_string(error_class, error_string, &length_of_error_string);
		fprintf(stderr, "%s\n", error_string);
		fprintf(stderr, "%s\n", error_class);
		MPI_Error_string(error_code, error_string, &length_of_error_string);
		fprintf(stderr, "%s\n", error_string);
		fprintf(stderr, "%d\n", error_code);
		fprintf(stderr, "%d,%d,%d\n", status.MPI_ERROR, status.MPI_SOURCE, status.MPI_TAG);
		MPI_Abort(MPI_COMM_WORLD, error_code);
	}
	
	bytes_recv += msg_size;

	ss.write(buf, msg_size);
	free(buf);
	buf = NULL;//拴住野指针
	{
		cereal::BinaryInputArchive archive(ss);
		archive(*a);
		archive(*b);
		archive(*v);
	}
}
///@brief Receive one object and deserialize it.//
//接收一个对象并反序列化它
template<class T>
void CommRecv(T* a, std::nullptr_t, int from) 
{
	CommRecv(a, (int*)nullptr, from);
}

template<class T, class U>
msg_type CommPrepare(const T* a, const U* b) {
	std::vector<char> omsg;
	std::stringstream ss(std::stringstream::in | std::stringstream::out | std::stringstream::binary);
	ss.unsetf(std::ios_base::skipws);
	cereal::BinaryOutputArchive archive(ss);
	archive(*a);
	if (b != nullptr)
		archive(*b);

	std::copy(std::istream_iterator<char>(ss), std::istream_iterator<char>(), std::back_inserter(omsg));

	return omsg;
}
template<class T, class U,class V>
msg_type CommPrepare(const T*a, const U*b, const V* v)
{
	std::vector<char> omsg;
	std::stringstream ss(std::stringstream::in | std::stringstream::out | std::stringstream::binary);
	ss.unsetf(std::ios_base::skipws);
	cereal::BinaryOutputArchive archive(ss);
	archive(*a);
	archive(*b);
	archive(*v);

	std::copy(std::istream_iterator<char>(ss), std::istream_iterator<char>(), std::back_inserter(omsg));

	return omsg;
}

template<class T, class U>
void CommSend(const T* a, const U* b, int dest, int tag) 
{
	auto omsg = CommPrepare(a, b);

	bytes_sent += omsg.size();

	int ret = MPI_Send(omsg.data(), omsg.size(), MPI_CHAR, dest, tag, MPI_COMM_WORLD);
	
	assert(ret == MPI_SUCCESS);
	//_unused(ret);
}



template<class T>
void CommSend(const T* a, std::nullptr_t, int dest, int tag)
{
	CommSend(a, (int*)nullptr, dest, tag);
}
template<class T>
void CommSend(const T* a, std::nullptr_t, int dest, int tag,int rank)
{
	CommSend(a, (int*)nullptr, dest, tag,rank);
}

template<class T, class U,class V>
void CommSend(const T* a, const U* b,const V*v, int dest, int tag)
{
	auto omsg = CommPrepare(a, b,v);

	bytes_sent += omsg.size();

	//int ret = MPI_Send(omsg.data(), omsg.size(), MPI_BYTE, dest, tag, MPI_COMM_WORLD);
	int ret = MPI_Send(omsg.data(), omsg.size(), MPI_CHAR, dest, tag, MPI_COMM_WORLD);
	assert(ret == MPI_SUCCESS);
	//_unused(ret);
}

void CommISend(msg_type &msg, int dest, int tag);
comm_count_type CommBytesSent();

///@brief Get the number of bytes received by this process
///@return Number of bytes received by this process
comm_count_type CommBytesRecv();

///@brief Reset message size statistics to zero.
void CommBytesReset();
int get_mem_info(const char *name, int index);
void hostProcess(GridInfo& gridInfo, std::vector<TileInfo>& tileInfos, IObjectFactory* pIObjFactory);
#endif