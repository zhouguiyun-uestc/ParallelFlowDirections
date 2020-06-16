#include <paradem/gdal.h>
#include <paradem/raster.h>

#include <iostream>
#include <memory>

//create a new GeoTIFF file
bool  WriteGeoTIFF(const char* path, int height, int width, void* pData, GDALDataType type, double* geoTransformArray6Eles,
	double* min, double* max, double* mean, double* stdDev, double nodatavalue)
{
	GDALDataset *poDataset;
	GDALAllRegister();
	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

	GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
	char **papszOptions = NULL;
	poDataset = poDriver->Create(path, width, height, 1, type,	papszOptions);


	if (geoTransformArray6Eles != NULL)
		poDataset->SetGeoTransform(geoTransformArray6Eles);


	GDALRasterBand* poBand;
	poBand = poDataset->GetRasterBand(1);

	poBand->SetNoDataValue(nodatavalue);

	if (min != NULL && max != NULL && mean != NULL && stdDev != NULL)
	{
		poBand->SetStatistics(*min, *max, *mean, *stdDev);
	}
	poBand->RasterIO(GF_Write, 0, 0, width, height,
		pData, width, height, type, 0, 0);

	GDALClose((GDALDatasetH)poDataset);
	return true;
}
//read GeoTIFF
bool readGeoTIFF(const char* path, GDALDataType type, Raster<float>& dem)
{
	if (path == nullptr)
	{
		//std::cout << "path is nullptr" << std::endl;
	}
	GDALDataset *poDataset;   
	GDALAllRegister();
	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");
	//std::cout << "start to open tif" << path << std::endl;
	poDataset = (GDALDataset*)GDALOpen(path, GA_ReadOnly);
	if (poDataset == NULL)
	{
		//std::cout << "read tif has some problems"<< std::endl;
		//std::cout << "fail to read file" << std::endl;
		return false;
	}

	GDALRasterBand* poBand;
	poBand = poDataset->GetRasterBand(1);
	GDALDataType dataType = poBand->GetRasterDataType();
	/*if (dataType != type)
	{
		//std::cout << "fail to read file" << std::endl;
		GDALClose((GDALDatasetH)poDataset);
		return false;
	}*/
//	if (geoTransformArray6Eles != nullptr)
	{
//		memset(geoTransformArray6Eles, 0, 6);
		dem.geoTransforms = std::make_shared<std::vector<double>>(std::vector<double>(6));
		poDataset->GetGeoTransform(&dem.geoTransforms->at(0));
	}	

	if (!dem.init(poBand->GetYSize(), poBand->GetXSize())) {
		GDALClose((GDALDatasetH)poDataset);
		return false;
	}

	poBand->RasterIO(GF_Read, 0, 0, dem.getWidth(), dem.getHeight(),
		(void *)&dem, dem.getWidth(), dem.getHeight(), type, 0, 0);
	//增加了一个dem的nodata值
	
	dem.NoDataValue =poBand->GetNoDataValue();

	GDALClose((GDALDatasetH)poDataset);

	return true;
}
