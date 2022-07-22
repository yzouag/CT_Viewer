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
    CT_Viewer(QWidget *parent = Q_NULLPTR);

private:
    bool CT_uploaded = false;     // CT_uploaded allows other buttons to function
    Ui::CT_ViewerClass ui;
    CT_Image ctImage;

private slots:
    void loadCT();
    void handleAdd();
    void handleConfirm();
    void handleClear();
    void handleDetail();
    void onScrewButtonClick();
    void onScrewSliderChange(double value);
};
