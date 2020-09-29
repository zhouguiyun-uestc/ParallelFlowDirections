
#ifndef PARADEM_GDAL_H
#define PARADEM_GDAL_H

#include <gdal_priv.h>
#include <paradem/raster.h>

bool WriteGeoTIFF( const char* path, int height, int width, void* pData, GDALDataType type, double* geoTransformArray6Eles, double* min, double* max, double* mean, double* stdDev,
                   double nodatavalue );
bool readGeoTIFF( const char* path, GDALDataType type, Raster< float >& dem );
bool readflowTIFF( const char* path, GDALDataType type, Raster< int >& flow );

#endif
