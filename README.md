# ParallelFlowDirections

**Manuscript Title**: Parallel assignment of flow directions over flat surfaces in massive digital elevation models  

**Authors**: Lihui Song, Guiyun Zhou, Yi Liu  

**Corresponding Author**: Guiyun Zhou (zhouguiyun@uestc.edu.cn)  


# Abstract
The assignment of flow directions in flat regions needs to be treated with special algorithms to obtain hydrologically correct flow patterns. Based on the sequential algorithm and the three-step parallel framework, we propose a parallel algorithm for assigning the hydrologically correct flow directions in flat regions. The proposed parallel algorithm assigns pre-divided tiles to multiple consumer processes, which construct local graphs that encode the geodesic distance information among tile border flat cells and higher terrain or lower terrain. Based on the local graphs in all tiles, the producer process constructs the global graph and computes the global geodesic distances of tile border flat cells from higher terrain and lower terrain. The consumer processes then compute the final geodesic distance in each tile and assign the hydrologically correct flow directions. Three experiments are conducted to evaluate the performance of our proposed algorithm. The memory requirement is much less than the total DEM file size. The speed-up ratio generally increases with more consumer processes. The scaling efficiency decreases with more consumer processes. The proposed parallel algorithm can process massive DEMs that cannot be successfully processed using the sequential algorithm. It fills the void in the processing pipeline of automatic drainage network extraction based on the three-step parallel framework and enables efficient parallel implementation of the entire processing pipeline possible, which should substantially improve the attractiveness of the three-step parallel framework.

# Prerequisite

GDAL , MPI, and cereal

The packages can be installed using standard install procedures. For example, to install cereal, the following command can be used on Ubuntu:   
```
sudo apt install libcereal-dev
```




# Compilation

To compile the programs run:
```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
make
```
The result is two programs called  `ParallelFlowDir` and `test`.

# Program arguments

 `ParallelFlowDir` program derives flow directions. 
 `test` program runs test cases on randomly generated DEMs.

 `ParallelFlowDir` program has the following arguments: 
```
mpirun -np <PROCESSES_NUMBER> ParallelFlowDir  <INPUT> <OUTPUT>
```
The `<INPUT>` argument is a text file and contains the paths of the tiles of the DEM.  
The `<OUTPUT>` argument specifes the output folder.   

An example command is: 
```
mpirun -np 3 ParallelFlowDir ./test_data/ansai_part.txt ./test_data/ansai_flow
```
`-np 3` indicates that the program is run in parallel over 3 processes, which includes 1 producer process and 2 consumer processes.  

`test` program has the following arguments: 
```
mpirun -np <PROCESSES_NUMBER> ParallelFlowDir <OUTPUT_PATH_OF_DEM> <HEIGHT_OF_THE_DEM > <WIDTH_OF_THE_DEM> <OUTPUT_PATH_OF_SEQUENTIAL_FLOW_DIRECTIONS> <TILE_HEIGHT> <TILE_WIDTH> <DIVIDE_FOLDER> <OUTPUT_FOLDER_OF_PARALLEL_FLOW_DIRECIOTNS>   
```
The `<OUTPUT_PATH_OF_DEM>` argument specifes the output path of the randomly generated DEM.  
The `<HEIGHT_OF_THE_DEM>` argument specifes the height of the DEM.   
The `<WIDTH_OF_THE DEM>` argument specifes the width of the DEM.   
The `<OUTPUT_PATH_OF_SEQUENTIAL_FLOW_DIRECTIONS>` argument specifes the output path of the flow directions using the sequential Barnes algorithm.  
The `<TILE HEIGHT>` argument specifes the height of the tile.   
The `<TILE_WIDTH>` argument specifes the width of the tile.   
The `<DIVIDE_PATH>` argument specifes the output folder of the tiles.  
The `<OUTPUT_FOLDER_OF_PARALLEL_FLOW_DIRECTIONS>` argument specifes the output folder of flow directions using our proposed parallel algorithm.  

An example command is: 
```
mpirun -np 4 ParallelFlowDir ./test_data/dem.tif 2000 3000 ./test_data/seqFlow/seqFlow.tif 500 800 ./test_data/tileDEM ./test_data/paraFlow 
```
`-np 4` indicates that the program is run in parallel over 4 processes, which includes 1 producer process and 3 consumer processes. 

# Format of input file 

The input file includes a two dimensional array of file paths of tiles.  
```
f1.tif, f2.tif, f3.tif, f4.tif,
      , f5.tif, f6.tif, f7.tif,
      , f8.tif, f9.tif,
```
If the file path is blank, it indicate that there is no tile there. The file format can be any type that GDAL support. 

# Testing

The program has a `test` mode, which verifies the correctness of our proposed parallel algorithm by comparing with the output of sequential algorithm using randomly generated DEMs.   
If using the sequential Barnes algorithm and our parallel algorithm results in the same flow directions, the program will output `The two rasters are the same!`.

# Test data

The test_data folder contains two test datasets that can be used with the program.


