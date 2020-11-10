#the minimum version
cmake_minimum_required(VERSION 3.9)

project (ParallelFlowDir LANGUAGES CXX)
message("Project Name:" ${PROJECT_NAME})

find_package(MPI REQUIRED)
find_package(GDAL REQUIRED)

#The output directory of the execution file
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

#header file 1
include_directories(${PROJECT_SOURCE_DIR}/include)

#header file 2
include_directories(${PROJECT_SOURCE_DIR}/src/flowdir})

#If find_package(GDAL REQUIRED) cannot find GDAL, you can set the GDAL_LIBRARY by yourself
#set(GDAL_INCLUDE_DIR /share/home/user/gdalBuild/include)
#set(GDAL_LIBRARY /share/home/user/gdalBuild/lib/libgdal.so)

message(STATUS " version:${GDAL_VERSION}")
message(STATUS " libraries:${GDAL_LIBRARY}")
message(STATUS " include path:${GDAL_INCLUDE_DIR}")

add_executable(${PROJECT_NAME}
  src/flowdir/producer_2_consumer.cpp
  src/flowdir/producer.cpp
  src/flowdir/outBoundary.cpp
  src/flowdir/object_factory.cpp
  src/flowdir/main.cpp
  src/flowdir/host.cpp
  src/flowdir/consumer_2_producer.cpp
  src/flowdir/consumer.cpp
  src/flowdir/GridCell.cpp
  src/common/tool.cpp
  src/common/timeInfo.cpp
  src/common/tile_info.cpp
  src/common/raster.cpp
  src/common/object_deleter.cpp
  src/common/memory.cpp
  src/common/grid.cpp
  src/common/gdal.cpp
  src/common/bit_raster.cpp
  src/common/Timer.cpp
)

target_link_libraries(${PROJECT_NAME} ${GDAL_LIBRARY} MPI::MPI_CXX)
target_include_directories(${PROJECT_NAME} PRIVATE ${GDAL_INCLUDE_DIR})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_11)
target_compile_options(${PROJECT_NAME} PRIVATE -fpermissive -Wall -pedantic)