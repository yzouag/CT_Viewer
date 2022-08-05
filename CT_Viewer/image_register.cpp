#include "image_register.h"
#include "json/json.hpp"
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QDebug>

using json = nlohmann::json;

ImageRegister::ImageRegister(QString fileName, QString filePath, CT_2d_Widget* widget)
{
    QDir dir;
    if (!(dir.mkpath(this->cachePath + "thumbnails/") && dir.mkpath(this->cachePath + "imageInfo/")))
    {
        qDebug() << "make directory failed! Cannot save cache.";
        throw 0;
        return;
    }
    this->fileName = fileName;
    this->filePath = filePath;
    this->createdTime = QDateTime::currentDateTime().toString("yyyyMMdd hh:mm:ss");
    createThumbnail(widget);
}

ImageRegister::ImageRegister(QString imageCachePath)
{
    ifstream f(imageCachePath.toStdString().c_str());
    json j;
    f >> j;
    
    this->fileName = QString::fromStdString(j["name"].get<std::string>());
    this->filePath = QString::fromStdString(j["path"].get<std::string>());
    this->createdTime = QString::fromStdString(j["created_time"].get<std::string>());
    this->thumbnailPath = QString::fromStdString(j["thumbnail"].get<std::string>());

    this->sliceCenter[0] = j["slice_center"].get<std::vector<double>>()[0];
    this->sliceCenter[1] = j["slice_center"].get<std::vector<double>>()[1];
    this->sliceCenter[2] = j["slice_center"].get<std::vector<double>>()[2];

    this->contrastThreshold[0] = j["contrast_threshold"]["lower"].get<int>();
    this->contrastThreshold[1] = j["contrast_threshold"]["upper"].get<int>();

    this->cameraPos[0] = j["camera"]["position"].get<std::vector<double>>()[0];
    this->cameraPos[1] = j["camera"]["position"].get<std::vector<double>>()[1];
    this->cameraPos[2] = j["camera"]["position"].get<std::vector<double>>()[2];

    this->focalPoint[0] = j["camera"]["focal_point"].get<std::vector<double>>()[0];
    this->focalPoint[1] = j["camera"]["focal_point"].get<std::vector<double>>()[1];
    this->focalPoint[2] = j["camera"]["focal_point"].get<std::vector<double>>()[2];
}

ImageRegister::~ImageRegister()
{
}

void ImageRegister::createThumbnail(CT_2d_Widget* widget)
{
    QString fileName = this->cachePath + "thumbnails/" + this->fileName + ".png";
    QPixmap pixmap(widget->size());
    widget->render(&pixmap);
    QFile file(fileName);
    file.open(QIODevice::WriteOnly);
    pixmap.save(&file, "PNG");
    this->thumbnailPath = fileName;
}

// save the class variables to the cache path
// current solution: a json file to store it? https://github.com/nlohmann/json
void ImageRegister::save()
{
    json j = {
        {"name", this->fileName.toStdString()},
        {"path", this->filePath.toStdString()},
        {"thumbnail", this->thumbnailPath.toStdString()},
        {"created_time", this->createdTime.toStdString()},
        {"slice_center", {this->sliceCenter[0], this->sliceCenter[1], this->sliceCenter[2]}},
        {"contrast_threshold", 
            {{"lower", this->contrastThreshold[0]}, 
            {"upper", this->contrastThreshold[1]}}
        },
        {"camera", 
            {{"position", {this->cameraPos[0], this->cameraPos[1], this->cameraPos[2]}},
            {"focal_point", {this->focalPoint[0], this->focalPoint[1], this->focalPoint[2]}}}
        }
    };
    QString json_path = this->cachePath + "imageInfo/" + this->fileName + ".json";
    ofstream f(json_path.toStdString().c_str());
    f << std::setw(4) << j << std::endl;
}

void ImageRegister::setSliceCenter(double* sliceCenter)
{
    this->sliceCenter[0] = sliceCenter[0];
    this->sliceCenter[1] = sliceCenter[1];
    this->sliceCenter[2] = sliceCenter[2];
}

void ImageRegister::setContrastThreshold(int lower, int upper)
{
    this->contrastThreshold[0] = lower;
    this->contrastThreshold[1] = upper;
}

QString ImageRegister::getThumbnail()
{
    return this->thumbnailPath;
}

QString ImageRegister::getFilePath()
{
    return this->filePath;
}

QString ImageRegister::getFileName()
{
    return this->fileName;
}

double * ImageRegister::getSliceCenter()
{
    return this->sliceCenter;
}

QString ImageRegister::getCreatedTime()
{
    return this->createdTime;
}

int * ImageRegister::getContrastThreshold()
{
    return this->contrastThreshold;
}

double * ImageRegister::getCameraPos()
{
    return this->cameraPos;
}

double * ImageRegister::getFocalPoint()
{
    return this->focalPoint;
}
