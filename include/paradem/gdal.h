#ifndef PARADEM_GDAL_H
#define PARADEM_GDAL_H

#include <paradem/raster.h>
#include <gdal_priv.h>
bool  WriteGeoTIFF(const char* path, int height, int width, void* pData, GDALDataType type, double* geoTransformArray6Eles,
	double* min, double* max, double* mean, double* stdDev, double nodatavalue);

bool readGeoTIFF(const char* path, GDALDataType type, Raster<float>& dem);

#endif 
