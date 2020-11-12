#the minimum version
cmake_minimum_required(VERSION 3.9)

project (ParallelFlowDir LANGUAGES CXX)
message("Project Name:" ${PROJECT_NAME})

find_package(MPI REQUIRED)
find_package(GDAL REQUIRED)

#The output directory of the execution file
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

#header file 1
include_directories(${PROJECT_SOURCE_DIR}/ParaDEM/include)

#header file 2
include_directories(${PROJECT_SOURCE_DIR}/ParaDEM/src/flowdir})

#Add the cereal libraty path
include_directories(${PROJECT_SOURCE_DIR}/ParaDEM)

message(STATUS " version:${GDAL_VERSION}")
message(STATUS " libraries:${GDAL_LIBRARY}")	
message(STATUS " include path:${GDAL_INCLUDE_DIR}")	

set(SOURCE_FILES 
${PROJECT_SOURCE_DIR}/ParaDEM/src/common/bit_raster.cpp 
${PROJECT_SOURCE_DIR}/ParaDEM/src/common/gdal.cpp 
${PROJECT_SOURCE_DIR}/ParaDEM/src/common/grid.cpp 
${PROJECT_SOURCE_DIR}/ParaDEM/src/common/memory.cpp 
${PROJECT_SOURCE_DIR}/ParaDEM/src/common/object_deleter.cpp 
${PROJECT_SOURCE_DIR}/ParaDEM/src/common/raster.cpp 
${PROJECT_SOURCE_DIR}/ParaDEM/src/common/tile_info.cpp 
${PROJECT_SOURCE_DIR}/ParaDEM/src/common/timeInfo.cpp 
${PROJECT_SOURCE_DIR}/ParaDEM/src/common/Timer.cpp 
${PROJECT_SOURCE_DIR}/ParaDEM/src/common/tool.cpp 
${PROJECT_SOURCE_DIR}/ParaDEM/src/flowdir/producer_2_consumer.cpp 
${PROJECT_SOURCE_DIR}/ParaDEM/src/flowdir/producer.cpp 
${PROJECT_SOURCE_DIR}/ParaDEM/src/flowdir/outBoundary.cpp 
${PROJECT_SOURCE_DIR}/ParaDEM/src/flowdir/object_factory.cpp 
${PROJECT_SOURCE_DIR}/ParaDEM/src/flowdir/main.cpp 
${PROJECT_SOURCE_DIR}/ParaDEM/src/flowdir/host.cpp 
${PROJECT_SOURCE_DIR}/ParaDEM/src/flowdir/GridCell.cpp 
${PROJECT_SOURCE_DIR}/ParaDEM/src/flowdir/consumer_2_producer.cpp 
${PROJECT_SOURCE_DIR}/ParaDEM/src/flowdir/consumer.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${GDAL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${GDAL_LIBRARY} MPI::MPI_CXX)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_11)
target_compile_options(${PROJECT_NAME} PRIVATE -fpermissive -Wall -pedantic)


