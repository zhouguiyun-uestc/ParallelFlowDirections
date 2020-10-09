# ParallelFlowDirections

**Manuscript Title**: Parallel assignment of flow directions over flat surfaces in massive digital elevation models  

**Authors**: Lihui Song, Guiyun Zhou, Yi Liu  

**Corresponding Author**: Guiyun Zhou(zhouguiyun@uestc.edu.cn)  


# Abstract
The assignment of flow directions in flat regions needs to be treated with special algorithms to obtain hydrologically correct flow patterns. Based on the sequential algorithm and the three-step parallel framework, we propose a parallel algorithm for assigning the hydrologically correct flow directions in flat regions. The proposed parallel algorithm assigns pre-divided tiles to multiple consumer processes, which construct local graphs that encode the geodesic distance information among tile border flat cells and higher terrain or lower terrain. Based on the local graphs in all tiles, the producer process constructs the global graph and computes the global geodesic distances of tile border flat cells from higher terrain and lower terrain. The consumer processes then compute the final geodesic distance in each tile and assign the hydrologically correct flow directions. Three experiments are conducted to evaluate the performance of our proposed algorithm. The memory requirement is much less than the total DEM file size. The speed-up ratio generally increases with more consumer processes. The scaling efficiency decreases with more consumer processes. The proposed parallel algorithm can process massive DEMs that cannot be successfully processed using the sequential algorithm. It fills the void in the processing pipeline of automatic drainage network extraction based on the three-step parallel framework and enables efficient parallel implementation of the entire processing pipeline possible, which should substantially improve the attractiveness of the three-step parallel framework.

# Prerequisite
GDAL and MPI

# Compilation
To compile the programs run:
```
cmake .
make
```
The result is a program called  `ParallelFlowDir`.

The program is run by typing:
```
mpirun -np <processes_number> flowdirPara <method> <INPUT> <OUTPUT>
mpirun -np 3 ParallelFlowDir parallel ./test_data/ansai_part.txt ./test_data/ansai_flow  
```
In the foregoing example `-np 3` indicates that the program should be run in parallel over 3 processes, which includes one producer process and 2 consumer processes. 'parallel' indicates that the program takes a text file as an input parameter, the text file includes the paths of the tile of DEMs.

# Input file 
The input file includes a two dimensional array of file paths of tiles.  
```
f1.tif, f2.tif, f3.tif, f4.tif,
      , f5.tif, f6.tif, f7.tif,
      , f8.tif, f9.tif,
```
If the file path is blank, it indicate that there is no tile there. The file format can be any type that GDAL support. 

# Testing
The folder (test_data) constains our test data, in which the text file includes the paths of the corresponding DEM.
To verify the correctness of the program, The program can be run by typing
```
mpirun -np <processes_number> ParallelFlowDir <method> <OUTPUT_DEM> <THE HEIGHT OF THE DEM > <THE WIDTH OF THE DEM> <OUTPUT SEQUENTIAL FLOW DIRECTION> <TILE HEIGHT> <TILE WIDTH> <DIVIDE PATH> <OUTPUT PARALLEL FLOW DIRECIOTN>
mpirun -np 4 ParallelFlowDir test ./test_data/dem.tif 2000 3000 ./test_data/seqFlow/seqFlow.tif 500 800 ./test_data/tileDEM ./test_data/paraFlow
```
In the foregoing example `-np 4` indicates that the program should be run in parallel over 4 processes, which includes one producer process and 3 consumer processes. 'test' indicates that the program automatically verify the correctness of the results. `./test_data/dem.tif ` is the output path of automtically generated DEM. `2000` is the height of the DEM, `3000` is the width of the DEM, `./test_data/seqFlow/seqFlow.tif` is the output path of flow directions of the DEM using the sequential Barnes algorithm. `500` is the height of the tile, `800` is the width of the tile. `./test_data/tileDEM` is the output path of the tiles of DEM. `./test_data/paraFlow` is the output path of the flow directions of tiles. If using the sequential Barnes algorithm and our parallel algorithm results in the same flow directions, the program will output `The two pictures are the same!`.



