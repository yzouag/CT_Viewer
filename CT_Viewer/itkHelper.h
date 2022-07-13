#pragma once
#ifndef ITKHELPER_H
#define ITKHELPER_H

#include <QMap>
#include <QString>
#include <string.h>
#include <itkSmartPointer.h>
#include <itkGDCMImageIO.h>
#include <vtkImageData.h>
#include <vtkSmartPointer.h>

struct CT_Data
{
    vtkSmartPointer<vtkImageData> CTImage;
    QMap<QString, QString> metaInfo;
    bool loadSucceed;
};

CT_Data loadCTSeries(const char* path);


#endif