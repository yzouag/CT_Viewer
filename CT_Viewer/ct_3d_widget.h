#pragma once
#include "QVTKOpenGLNativeWidget.h"
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkImageData.h>
#include <vtkRenderWindowInteractor.h>
#include <QProgressDialog>
#include "planting_screws.h"
#include <QVector>

namespace Ui {
    class CT_3d_Widget;
}

class CT_3d_Widget : public QVTKOpenGLNativeWidget
{
    Q_OBJECT

public:
    CT_3d_Widget(QWidget *parent);
    ~CT_3d_Widget();
    void setRenderWindowSize(int height, int width);
    vtkGenericOpenGLRenderWindow* getRenderWindow();
    void setCTImage(vtkImageData* ctImage);
    QVector<PlantingScrews*> getScrewList();

public slots:
    void loadCT();
    void addScrew(const char* screwName);
    void removeAll();

private:
    vtkGenericOpenGLRenderWindow* renWin;
    vtkRenderer* render;
    vtkImageData* ctImage;
    vtkRenderWindowInteractor* interactor;
    QVector<PlantingScrews*> screwList;
};

