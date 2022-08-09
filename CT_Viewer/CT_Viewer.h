#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_CT_Viewer.h"
#include <vtkImageData.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkImageReslice.h>
#include <vtkBoxWidget.h>
#include <vtkActor.h>
#include <QMap>
#include <QString>
#include <QVector>
#include "ct_image.h"

class CT_Viewer : public QMainWindow
{
    Q_OBJECT

public:
    CT_Viewer(CT_Image* ctImage, QWidget *parent = Q_NULLPTR);
    ~CT_Viewer();
    void init2DViews();
    void loadSliceAndThreshold(double* sliceCenter, int* contrastThreshold);
    void loadCameraSettings(double* cameraPos, double* focalPoint);

private:
    bool CT_uploaded = false; // CT_uploaded allows other buttons to function
    Ui::CT_ViewerClass ui;
    CT_Image* ctImage;
    QVector<PlantingScrews*> screwList;
    void takeScreenshot(QWidget* widget);

private slots:
    void loadCT();
    void handleAdd();
    void handleClear();
    void handleDetail();

    // three scroll bar slots
    void handleAxialScrollBarChange(int val);
    void handleCoronalScrollBarChange(int val);
    void handleSagittalScrollBarChange(int val);

    // four zoom in and zoom out button
    void handle3DView();
    void handleAxialView();
    void handleCoronalView();
    void handleSagittalView();

    // screen shot for each window
    void handle3DScreenshot();
    void handleAxialScreenshot();
    void handleCoronalScreenshot();
    void handleSagittalScreenshot();

    // 3D image camera reset
    void handle3DReset();

    // set the contrast, color window
    void handleSetContrast();

    // six direction button and the orientation slider bar
    void onScrewButtonClick();
    void onScrewSliderChange(double value);
};
