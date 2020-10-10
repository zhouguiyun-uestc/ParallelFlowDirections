#the minimum version
cmake_minimum_required(VERSION 2.6)

project (ParallelFlowDir LANGUAGES CXX)
message("Project Name:" ${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-std=c++11 -fpermissive -O3)

#The output directory of the execution file
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

#header file 1
include_directories(${PROJECT_SOURCE_DIR}/include)

#header file 2
include_directories(${PROJECT_SOURCE_DIR}/src/flowdir})

#Add the cereal libraty path
include_directories(/share/home/cereal/include)

aux_source_directory(${PROJECT_SOURCE_DIR}/src/common c_files)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/flowdir c_files)

find_package(MPI REQUIRED)
set(CMAKE_MPI MPI::MPI_CXX)


#If find_package(GDAL REQUIRED) cannot find GDAL, you can set the GDAL_LIBRARY by yourself
#set(GDAL_INCLUDE_DIR /share/home/user/gdalBuild/include)
#set(GDAL_LIBRARY /share/home/user/gdalBuild/lib/libgdal.so)

find_package(GDAL REQUIRED)
message(STATUS " version:${GDAL_VERSION}")
message(STATUS " libraries:${GDAL_LIBRARY}")
message(STATUS " include path:${GDAL_INCLUDE_DIR}")

add_executable(${PROJECT_NAME} ${c_files})

target_link_libraries(${PROJECT_NAME} ${GDAL_LIBRARY} ${CMAKE_MPI})
