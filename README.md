# ParallelFlowDirections

**Manuscript Title**: Parallel assignment of flow directions over flat surfaces in massive digital elevation models
**Authors**:Lihui Song, Guiyun Zhou, Yi Liu
**Corresponding Author**:Guiyun Zhou(zhouguiyun@uestc.edu.cn)
#Compilation
To compile the programs run:
'''
make
'''


This repository contains the source codes of the MPI-based implementation of the parallel algorithm presented in the manuscript above. These codes were used in performing the tests described in the manuscript.

The codes support floating-point GeoTIFF file format through the GDAL library. Please include GDAL library into your compilation.

Example usages:

mpirun -np 2 flowdirPara carlton.txt carlton_3m    

// use two processors to caculate flow directions; Input is a text file, which includes the paths of the DEM; Output is carlton_3m
//for the command arguments, please refer to the source code

