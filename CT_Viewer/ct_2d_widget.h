#pragma once
#include "QVTKOpenGLNativeWidget.h"
#include "vtkHelper.h"
#include <vtkImageReslice.h>
#include <vtkRenderer.h>
#include <vtkImageData.h>
#include <vtkCursor2D.h>
#include <vtkActor.h>
#include <vtkImageActor.h>
#include <vtkImageMapToColors.h>
#include <QScrollBar>
#include "ct_image.h"

namespace Ui {
    class CT_2d_Widget;
}

// this class is to show three 2D views, Sagittal, Coronal, and Axial View
// this class is derived from QVTKOpenGLNativeWidget, and can put a vtk window inside
// the class will receive signal from CT_Image, and set the slice, cursor and scroll
// bar accordingly
class CT_2d_Widget : public QVTKOpenGLNativeWidget
{
    Q_OBJECT;

public:
    CT_2d_Widget(QWidget *parent);
    ~CT_2d_Widget();
    void setViewMode(ViewMode mode);
    void renderCTReslice(CT_Image* ctImage);
    void updateCTReslice(vtkImageData* ctImage);
    void setScrollBar(QScrollBar* scrollBar);
    vtkImageReslice* getReslice();
    ViewMode getViewMode();

public slots:
    void updateWhenSliceCenterChange(double x, double y, double z);
    void updateColorMap(int lower, int upper);

private:
    vtkGenericOpenGLRenderWindow* renWin;
    vtkRenderer* ren;
    ViewMode mode = ThreeDimension;
    CT_Image* ctImage;
    vtkCursor2D* cursor;
    vtkImageMapToColors* mapToColor;
    QScrollBar* scrollBar;
    void setWindowTitle();
};