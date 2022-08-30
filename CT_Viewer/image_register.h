#pragma once

#include <QString>
#include <QPixmap>
#include "ct_2d_widget.h"
#include "planting_screws.h"

class ImageRegister
{
public:
    ImageRegister(QString fileName, QString filePath, CT_2d_Widget* widget, QVector<PlantingScrews*>* screwList);
    ImageRegister(QString imageCachePath);
    ~ImageRegister();
    void createThumbnail(CT_2d_Widget* widget);
    void save();
    
    // setters
    void setSliceCenter(double* sliceCenter);
    void setContrastThreshold(int lower, int upper);
    
    // getters
    QString getImageThumbnail();
    QString getWorkspaceThumbnail();
    QString getFilePath();
    QString getFileName();
    double* getSliceCenter();
    QString getCreatedTime();
    int* getContrastThreshold();
    double* getCameraPos();
    double* getFocalPoint();
    QVector<QPair<QString, QVector<double>>>& getScrewList();

private:
    // location of the registry
    QString cachePath = "./cache/";
    // descriptor for the file
    QString fileName;
    QString filePath;
    QString imageThumbnailPath; // path + name
    QString workspaceThumbnailPath; // path + name
    QString createdTime;
    // current slice center
    double sliceCenter[3];
    // current contrast
    int contrastThreshold[2];
    // current camera settings
    double focalPoint[3];
    double cameraPos[3];
    // current screw list
    QVector<PlantingScrews*>* screwList;
    QVector<QPair<QString, QVector<double>>> screws;
};