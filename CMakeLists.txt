#the minimum version
cmake_minimum_required(VERSION 3.9)

project (ParallelFlowDir LANGUAGES CXX)
message("Project Name:" ${PROJECT_NAME})

find_package(MPI REQUIRED)
find_package(GDAL REQUIRED)

#The output directory of the execution file
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

set(CL11_INCLUDE_DIR CLI11/include)

#header file 1
include_directories(${PROJECT_SOURCE_DIR}/include)

#header file 2
include_directories(${PROJECT_SOURCE_DIR}/src/flowdir)

message(STATUS " version:${GDAL_VERSION}")
message(STATUS " libraries:${GDAL_LIBRARY}")	
message(STATUS " include path:${GDAL_INCLUDE_DIR}")	

set(SOURCE_FILES 
src/common/bit_raster.cpp 
src/common/gdal.cpp 
src/common/grid.cpp 
src/common/memory.cpp 
src/common/object_deleter.cpp 
src/common/raster.cpp 
src/common/tile_info.cpp 
src/common/timeInfo.cpp 
src/common/Timer.cpp 
src/common/tool.cpp 
src/flowdir/producer_2_consumer.cpp 
src/flowdir/producer.cpp 
src/flowdir/outBoundary.cpp 
src/flowdir/object_factory.cpp 
src/flowdir/main.cpp 
src/flowdir/host.cpp 
src/flowdir/GridCell.cpp 
src/flowdir/consumer_2_producer.cpp 
src/flowdir/consumer.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${GDAL_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${CL11_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${GDAL_LIBRARY} MPI::MPI_CXX)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_11)
target_compile_options(${PROJECT_NAME} PRIVATE -fpermissive -Wall -pedantic)
