#include "image_register.h"
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QDebug>
#include <vtkTransform.h>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>


ImageRegister::ImageRegister(QString fileName, QString filePath, CT_2d_Widget* widget, QVector<PlantingScrews*>* screwList)
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
    this->screwList = screwList;
}

ImageRegister::ImageRegister(QString imageCachePath)
{
    QFile loadFile(imageCachePath);
    if (!loadFile.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open save file.");
        return;
    }
    QByteArray saveData = loadFile.readAll();
    QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
    QJsonObject imageObject = loadDoc.object();

    this->fileName = imageObject["name"].toString();
    this->filePath = imageObject["path"].toString();
    this->createdTime = imageObject["created_time"].toString();
    this->imageThumbnailPath = imageObject["image_thumbnail"].toString();
    this->workspaceThumbnailPath = imageObject["workspace_thumbnail"].toString();

    this->sliceCenter[0] = imageObject["slice_center"].toArray()[0].toDouble();
    this->sliceCenter[1] = imageObject["slice_center"].toArray()[1].toDouble();
    this->sliceCenter[2] = imageObject["slice_center"].toArray()[2].toDouble();

    this->contrastThreshold[0] = imageObject["contrast_threshold"].toObject()["lower"].toInt();
    this->contrastThreshold[1] = imageObject["contrast_threshold"].toObject()["upper"].toInt();

    this->cameraPos[0] = imageObject["camera"].toObject()["position"].toArray()[0].toDouble();
    this->cameraPos[1] = imageObject["camera"].toObject()["position"].toArray()[1].toDouble();
    this->cameraPos[2] = imageObject["camera"].toObject()["position"].toArray()[2].toDouble();

    this->focalPoint[0] = imageObject["camera"].toObject()["focal_point"].toArray()[0].toDouble();
    this->focalPoint[1] = imageObject["camera"].toObject()["focal_point"].toArray()[1].toDouble();
    this->focalPoint[2] = imageObject["camera"].toObject()["focal_point"].toArray()[2].toDouble();

    QJsonArray arr = imageObject["screw_list"].toArray();
    for (auto ptr = arr.begin(); ptr < arr.end(); ptr++) {
        QPair<QString, QVector<double>> screw_pair;
        screw_pair.first = ptr->toObject()["screw_name"].toString();
        QVector<double> temp;
        for (int i = 0; i < 16; i++) {
            temp.append(ptr->toObject()["screw_transformation"].toArray()[i].toDouble());
        }
        screw_pair.second = temp;
        this->screws.append(screw_pair);
    }
}

ImageRegister::~ImageRegister()
{
    qDebug() << "image register deleted!";
}

void ImageRegister::createThumbnail(CT_2d_Widget* widget)
{
    // save workspace thumbnail
    QString fileName = this->cachePath + "thumbnails/" + "workspace_" + this->fileName + ".png";
    QPixmap pixmap(widget->size());
    widget->render(&pixmap);
    QFile file(fileName);
    file.open(QIODevice::WriteOnly);
    pixmap.save(&file, "PNG");
    this->workspaceThumbnailPath = fileName;

    // hide all the other actors, only keep the reslice image
    widget->showOnlyCTReslice();
    QString fileName1 = this->cachePath + "thumbnails/" + "image_" + this->fileName + ".png";
    QPixmap pixmap1(widget->size());
    widget->render(&pixmap1);
    QFile file1(fileName1);
    file1.open(QIODevice::WriteOnly);
    pixmap1.save(&file1, "PNG");
    this->imageThumbnailPath = fileName1;
}

// save the class variables to the cache path
void ImageRegister::save()
{
    QJsonObject imageObject;
    // basic info
    imageObject["name"] = this->fileName;
    imageObject["path"] = this->filePath;
    imageObject["image_thumbnail"] = this->imageThumbnailPath;
    imageObject["workspace_thumbnail"] = this->workspaceThumbnailPath;
    imageObject["created_time"] = this->createdTime;
    // slice center
    QJsonArray sliceCenter = {this->sliceCenter[0], this->sliceCenter[1], this->sliceCenter[2]};
    imageObject["slice_center"] = sliceCenter;
    // contrast settings
    QJsonObject contrast;
    contrast["lower"] = this->contrastThreshold[0];
    contrast["upper"] = this->contrastThreshold[1];
    imageObject["contrast_threshold"] = contrast;
    // camera settings
    QJsonObject camera;
    camera["position"] = QJsonArray({this->cameraPos[0], this->cameraPos[1], this->cameraPos[2]});
    camera["focal_point"] = QJsonArray({this->focalPoint[0], this->focalPoint[1], this->focalPoint[2]});
    imageObject["camera"] = camera;
    // screw list
    QJsonArray screwArray;
    for (auto screw : *screwList) {
        QJsonObject screwObject;
        screwObject["screw_name"] = screw->getScrewName();
        vtkNew<vtkTransform> t;
        screw->getScrewWidget()->GetTransform(t);
        double* temp = t->GetMatrix()->GetData();
        QJsonArray transformation;
        for (int i = 0; i < 16; i++) {
            transformation.append(temp[i]);
        }
        screwObject["screw_transformation"] = transformation;
        screwArray.append(screwObject);
    }
    imageObject["screw_list"] = screwArray;

    // store the json to the path
    QString json_path = this->cachePath + "imageInfo/" + this->fileName + ".json";
    QFile saveFile(json_path);
    if (!saveFile.open(QIODevice::WriteOnly)) {
        qWarning("Couldn't open save file.");
        return;
    }
    saveFile.write(QJsonDocument(imageObject).toJson());
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

QString ImageRegister::getImageThumbnail()
{
    return this->imageThumbnailPath;
}

QString ImageRegister::getWorkspaceThumbnail()
{
    return this->workspaceThumbnailPath;
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

QVector<QPair<QString, QVector<double>>>& ImageRegister::getScrewList()
{
    return screws;
}
