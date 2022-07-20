#pragma once
#include "QVTKOpenGLNativeWidget.h"
#include "vtkHelper.h"
#include <vtkImageReslice.h>
#include <vtkRenderer.h>
#include <vtkImageData.h>

namespace Ui {
    class CT_2d_Widget;
}

class CT_2d_Widget : public QVTKOpenGLNativeWidget
{
    Q_OBJECT;

public:
    CT_2d_Widget(QWidget *parent);
    ~CT_2d_Widget();
    void setViewMode(ViewMode mode);
    void renderCTReslice(vtkImageReslice* reslice);
    void updateCTReslice(vtkImageData* ctImage);

private:
    vtkGenericOpenGLRenderWindow* renWin;
    vtkRenderer* render;
    ViewMode mode = ThreeDimension;
    vtkImageReslice* reslice;
    void setWindowTitle();
};

