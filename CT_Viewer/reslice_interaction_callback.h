#pragma once
#include <vtkCommand.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>
#include <vtkCursor2D.h>
#include <vtkRenderer.h>
#include "ct_image.h"
#include "ct_2d_widget.h"

class resliceInteractionCallback : public vtkCommand
{
public:
    static resliceInteractionCallback* New()
    {
        return new resliceInteractionCallback;
    }
    resliceInteractionCallback();
    ~resliceInteractionCallback();
    void Execute(vtkObject* caller, unsigned long eventId, void* callData) override;
    void setCursor(vtkSmartPointer<vtkCursor2D> cursor);
    void setInteractor(vtkSmartPointer<vtkRenderWindowInteractor> interactor);
    void setRender(vtkSmartPointer<vtkRenderer> render);
    void setImageReslice(CT_2d_Widget* imageWindowWidget);
    void setCTImage(CT_Image* ctImage);

private:
    bool isSlicing;
    bool isToggled;
    CT_2d_Widget* imageWindowWidget;
    vtkSmartPointer<vtkCursor2D> cursor;
    vtkSmartPointer<vtkRenderWindowInteractor> interactor;
    vtkSmartPointer<vtkRenderer> ren;
    CT_Image* ctImage;
    void updateCursorPos();
    void updateReslicePos();
    bool isOutBound(double x, double y, double z);
};

