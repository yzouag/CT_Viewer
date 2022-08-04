#pragma once

#include <QString>
#include <QPixmap>
#include "ct_2d_widget.h"

class ImageRegister
{
public:
    ImageRegister(QString fileName, QString filePath, CT_2d_Widget* widget);
    ImageRegister(QString imageCachePath);
    ~ImageRegister();
    void createThumbnail(CT_2d_Widget* widget);
    void save();
    
    // setters
    void setSliceCenter(double* sliceCenter);
    void setContrastThreshold(int lower, int upper);
    
    // getters
    QString getThumbnail();
    QString getFilePath();
    QString getFileName();
    double* getSliceCenter();
    QString getCreatedTime();
    int* getContrastThreshold();

private:
    // location of the registry
    QString cachePath = "../cache/";
    // descriptor for the file
    QString fileName;
    QString filePath;
    QString thumbnailPath; // path + name
    QString createdTime;
    // current slice center
    double sliceCenter[3];
    // current contrast
    int contrastThreshold[2];
};