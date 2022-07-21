#include "reslice_interaction_callback.h"
#include <vtkCoordinate.h>
#include <vtkMatrix4x4.h>
#include <vtkInteractorStyle.h>
#include <vtkImageData.h>
#include <QDebug>

resliceInteractionCallback::resliceInteractionCallback()
{
    this->isSlicing = false;
    this->isToggled = false;
    this->imageReslice = nullptr;
    this->interactor = nullptr;
    this->cursor = nullptr;
    this->ren = nullptr;
}

resliceInteractionCallback::~resliceInteractionCallback()
{
}

void resliceInteractionCallback::Execute(vtkObject * caller, unsigned long eventId, void * callData)
{
    // right button: slice on current view, adjust the cursor of other views
    // left button: adjust the cursor, slice on other views, the cursor of other views will also change
    switch (eventId) {
    case vtkCommand::RightButtonPressEvent:
        this->isSlicing = true;
        break;
    case vtkCommand::RightButtonReleaseEvent:
        this->isSlicing = false;
        break;
    case vtkCommand::LeftButtonPressEvent:
        isToggled = true;
        break;
    case vtkCommand::LeftButtonReleaseEvent:
        isToggled = false;
        break;
    case vtkCommand::MouseMoveEvent:
        if (isToggled) {
            updateCursorPos();
        }
        else if (isSlicing) {
            updateReslicePos();
        }
        else {
            vtkInteractorStyle* style = vtkInteractorStyle::SafeDownCast(this->interactor->GetInteractorStyle());
            if (style)
                style->OnMouseMove();
        }
        break;
    }
}

void resliceInteractionCallback::setCursor(vtkSmartPointer<vtkCursor2D> cursor)
{
    this->cursor = cursor;
}

void resliceInteractionCallback::setRender(vtkSmartPointer<vtkRenderer> render)
{
    this->ren = render;
}

void resliceInteractionCallback::setImageReslice(vtkSmartPointer<vtkImageReslice> reslice)
{
    this->imageReslice = reslice;
}

void resliceInteractionCallback::setMapToColors(vtkSmartPointer<vtkImageMapToColors> colors)
{
    this->mapToColors = colors;
}

void resliceInteractionCallback::setQTWidget(CT_2d_Widget * qtWidget)
{
    this->qtWidget = qtWidget;
}

void resliceInteractionCallback::setInteractor(vtkSmartPointer<vtkRenderWindowInteractor> interactor)
{
    this->interactor = interactor;
}

void resliceInteractionCallback::updateCursorPos()
{
    int pos[2];
    this->interactor->GetEventPosition(pos);

    // from view coordinate to world coordinate
    // the cursor is defined in world coordinate system
    vtkNew<vtkCoordinate> coordinateSystem;
    coordinateSystem->SetCoordinateSystemToDisplay();
    coordinateSystem->SetValue(1.0 * pos[0], 1.0 * pos[1]);
    double* worldPos = coordinateSystem->GetComputedWorldValue(this->ren);
    this->cursor->SetFocalPoint(worldPos);
    
    // update the cursor and render
    this->cursor->Modified();
    this->interactor->Render();

    // send signal to sync other two views
    this->qtWidget->sendPosSignal();
}

void resliceInteractionCallback::updateReslicePos()
{
    // get the distance of slices user want to skip
    int lastPos[2], curPos[2];
    this->interactor->GetLastEventPosition(lastPos);
    this->interactor->GetEventPosition(curPos);
    int deltaY = lastPos[1] - curPos[1];
    
    // set the new reslice origin
    this->imageReslice->Update();
    double spacing = this->imageReslice->GetOutput()->GetSpacing()[2];
    vtkMatrix4x4* matrix = this->imageReslice->GetResliceAxes();
    double point[4], center[4];
    point[0] = 0.0;
    point[1] = 0.0;
    point[2] = spacing * deltaY;
    point[3] = 1.0;

    // if the reslice is out of the image, then cancel that origin move
    if (isOutBound(center[0], center[1], center[2])) {
        return;
    }

    matrix->MultiplyPoint(point, center);
    matrix->SetElement(0, 3, center[0]);
    matrix->SetElement(1, 3, center[1]);
    matrix->SetElement(2, 3, center[2]);

    // update the render
    this->mapToColors->Update();
    this->interactor->Render();

    // send the signal to sync other two views
    this->qtWidget->sendResliceSignal();
}

bool resliceInteractionCallback::isOutBound(double x, double y, double z)
{
    double x_bound = this->qtWidget->getModelCenter()[0];
    double y_bound = this->qtWidget->getModelCenter()[1];
    double z_bound = this->qtWidget->getModelCenter()[2];
    if (x > x_bound || y > y_bound || z > z_bound)
        return true;
    else
        return false;
}
