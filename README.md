# ParallelFlowDirections

**Manuscript Title**: Parallel assignment of flow directions over flat surfaces in massive digital elevation models  

**Authors**:Lihui Song, Guiyun Zhou, Yi Liu  

**Corresponding Author**:Guiyun Zhou(zhouguiyun@uestc.edu.cn)  


# Compilation
To compile the programs run:
```
make
```
The result is a program called  `flowdirPara`.

The program is run by typing:
```
mpirun -np <processes_number> flowdirPara <INPUT> <OUTPUT>
mpirun -np 3 flowdirPara carlton.txt carlton_3m   
```
In the foregoing example `-np 3` indicates that the program should be run in parallel over 3 processes, which includes one producer process and 2 consumer processes.
# Layout Files
A layout file is a text file with the format:
```
f1.tif, f2.tif, f3.tif, f4.tif,
      , f5.tif, f6.tif, f7.tif,
      , f8.tif, f9.tif,
```
fx.tifs with same row have the same height. fx.tifs with same column have the same width. Blanks between commas indicate that there is no tile there.
Note that the files need not have TIF format: they can be of any type which GDAL can read. 



