#pragma once

#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkImageReslice.h>
#include <QProgressDialog>
#include <itkGDCMImageIO.h>
#include <itkSmartPointer.h>
#include "planting_screws.h"
#include <QVector>
#include <QMap>

class CT_Image
{
public:
    CT_Image();
    ~CT_Image();
    void loadDicomFromDirectory(const char* path, QProgressDialog* dialog);
    vtkSmartPointer<vtkImageData> getCTImageData();
    vtkSmartPointer<vtkImageReslice> getCTImageReslice(int axis);
    QMap<QString, QString> getMetaInfo();
    bool checkLoadSuccess();
    void updateImage(QVector<PlantingScrews*> screwList);
    void resetImage();

private:
    double const VIEWDIRECTIONMATRIX[3][16] = {
        {0, 0,-1, 0,
        -1, 0, 0, 0,
        0,-1, 0, 0,
        0, 0, 0, 1},

        {1, 0, 0, 0,
        0, 0, 1, 0,
        0, -1,0, 0,
        0, 0, 0, 1},

        {1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1}
    };
    void loadMetaInfo(QProgressDialog* dialog);
    void createReslices(int axis);
    void createMetaInfo(itk::SmartPointer<itk::GDCMImageIO> dicomIO);
    vtkSmartPointer<vtkImageData> ctImage;
    vtkSmartPointer<vtkImageData> originImage;
    vtkSmartPointer<vtkImageReslice> ctReslices[3];
    QMap<QString, QString> metaInfo;
    bool load_succeed;
    const char* path;
};