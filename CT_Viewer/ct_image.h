#pragma once

#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkImageReslice.h>
#include <QProgressDialog>
#include <itkGDCMImageIO.h>
#include <itkSmartPointer.h>
#include <vtkImageAccumulate.h>
#include "planting_screws.h"
#include <QVector>
#include <QMap>

class CT_Image : public QObject
{ 
    Q_OBJECT;
public:
    CT_Image();
    ~CT_Image();
    bool loadDicomFromDirectory(QString path, QProgressDialog* dialog);
    vtkSmartPointer<vtkImageData> getCTImageData();
    vtkSmartPointer<vtkImageReslice> getCTImageReslice(int axis);
    vtkSmartPointer<vtkImageAccumulate> getCTImageAccumulate();
    QMap<QString, QString> getMetaInfo();
    bool checkLoadSuccess();
    void updateImage(QVector<PlantingScrews*> screwList, QProgressDialog* dialog);
    QString getFilePath();
    double* getModelCenter();
    double* getSliceCenter();
    double* getModelBounds();
    int* getContrastThreshold();
    void saveImageData(QString diretoryPath, QProgressDialog* dialog);

public slots:
    void updateSliceCenter(double x, double y, double z);
    void updateContrastThreshold(int lower, int upper);

signals:
    void sliceCenterChange(double x, double y, double z);
    void contrastThresholdChange(int lower, int upper);

private:
    double const VIEWDIRECTIONMATRIX[3][16] = {
        {0, 0,-1, 0,
        -1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 0, 1},

        {-1, 0, 0, 0,
        0, 0, 1, 0,
        0, 1, 0, 0,
        0, 0, 0, 1},

        {-1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1}
    };

    void createReslices(int axis);
    void createMetaInfo(itk::SmartPointer<itk::GDCMImageIO> dicomIO);

    vtkSmartPointer<vtkImageAccumulate> ctImageAccumulate;
    vtkSmartPointer<vtkImageData> ctImage;
    vtkSmartPointer<vtkImageReslice> ctReslices[3];
    QMap<QString, QString> metaInfo;
    bool load_succeed;
    QString path;

    double sliceCenter[3];
    double modelCenter[3];
    double modelBounds[6];
    signed short min_pixel_val;
    signed short max_pixel_val;
    int contrastThreshold[2] = {-1000, 1000};
};