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
#include "ct_image.h"

class CT_Viewer : public QMainWindow
{
    Q_OBJECT

public:
    CT_Viewer(CT_Image* ctImage, QWidget *parent = Q_NULLPTR);
    ~CT_Viewer();
    void init2DViews();

private:
    bool CT_uploaded = false;     // CT_uploaded allows other buttons to function
    Ui::CT_ViewerClass ui;
    CT_Image* ctImage;
    void takeScreenshot(QWidget* widget);

private slots:
    void loadCT();
    void handleAdd();
    void handleConfirm();
    void handleClear();
    void handleDetail();
    void handle3DView();
    void handleAxialView();
    void handleCoronalView();
    void handleSagittalView();
    void handle3DScreenshot();
    void handleAxialScreenshot();
    void handleCoronalScreenshot();
    void handleSagittalScreenshot();
    void handle3DReset();
    void handleSetContrast();
    void onScrewButtonClick();
    void onScrewSliderChange(double value);
};
