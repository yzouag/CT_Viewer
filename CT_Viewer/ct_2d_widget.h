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
    void sendPosSignal();
    void sendResliceSignal();
    double* getModelCenter();
    void setScrollBar(QScrollBar* scrollBar);
    double* getSliceCenter();
    int* getContrastThreshold();

public slots:
    void updateWhenCursorPosChange(int x, int y, ViewMode comingSignalViewMode);
    void updateWhenReslicePosChange(int z, ViewMode comingSignalViewMode);
    void updateColorMap(int lower, int upper);
    void updateWhenScrollbarChanged(int value);

signals:
    void cursorPosChange(int x, int y, ViewMode mode);
    void reslicePosChange(int z, ViewMode mode);

private:
    vtkGenericOpenGLRenderWindow* renWin;
    vtkRenderer* ren;
    ViewMode mode = ThreeDimension;
    vtkImageReslice* reslice;
    vtkCursor2D* cursor;
    vtkImageMapToColors* mapToColor;
    QScrollBar* scrollBar;
    double modelCenter[3];
    double sliceCenter[3];
    int contrastThreshold[2] = { -1000, 1000};
    void setWindowTitle();
    void updateCursorPos();
    void updateReslicePos();
};

