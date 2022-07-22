#pragma once
#include "QVTKOpenGLNativeWidget.h"
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkImageData.h>
#include <vtkRenderWindowInteractor.h>
#include <QProgressDialog>
#include "planting_screws.h"
#include <vtkProperty.h>
#include <QVector>

enum ScrewAction
{
    UP, DOWN, LEFT, RIGHT, FRONT, BACK, ROTATE_IS, ROTATE_LR
};

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
    void confirmActors();
    void moveScrew(ScrewAction action, double value=0);
    void setActiveScrew(vtkBoxWidget* activeScrew);
    vtkBoxWidget* getActiveScrew();
    vtkProperty* getLastPickedProperty();
    void setLastPickedProperty(vtkProperty* lastPickedProperty);

public slots:
    void loadCT();
    void addScrew(const char* screwName);
    void removeAll();

private:
    vtkBoxWidget* activeScrew = nullptr;
    vtkGenericOpenGLRenderWindow* renWin;
    vtkRenderer* render;
    vtkImageData* ctImage;
    vtkRenderWindowInteractor* interactor;
    QVector<PlantingScrews*> screwList;
    vtkProperty* lastPickedProperty;
};

