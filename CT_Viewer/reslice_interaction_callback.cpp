#include "reslice_interaction_callback.h"
#include <vtkCoordinate.h>
#include <vtkMatrix4x4.h>
#include <vtkInteractorStyle.h>
#include <vtkImageData.h>

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
    int lastPos[2], curPos[2];
    this->interactor->GetLastEventPosition(lastPos);
    this->interactor->GetEventPosition(curPos);
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
            int deltaY = lastPos[1] - curPos[1];
            this->imageReslice->Update();
            double spacing = this->imageReslice->GetOutput()->GetSpacing()[2];
            vtkMatrix4x4* matrix = this->imageReslice->GetResliceAxes();
            double point[4], center[4];
            point[0] = 0.0;
            point[1] = 0.0;
            point[2] = spacing * deltaY;
            point[3] = 1.0;

            matrix->MultiplyPoint(point, center);
            matrix->SetElement(0, 3, center[0]);
            matrix->SetElement(1, 3, center[1]);
            matrix->SetElement(2, 3, center[2]);

            this->mapToColors->Update();
            this->interactor->Render();
        }
        else {
            vtkInteractorStyle* style = vtkInteractorStyle::SafeDownCast(this->interactor->GetInteractorStyle());
            if (style)
                style->OnMouseMove();
        }
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

void resliceInteractionCallback::setInteractor(vtkSmartPointer<vtkRenderWindowInteractor> interactor)
{
    this->interactor = interactor;
}

void resliceInteractionCallback::updateCursorPos()
{
    int pos[2];
    this->interactor->GetEventPosition(pos);
    vtkNew<vtkCoordinate> coordinateSystem;
    coordinateSystem->SetCoordinateSystemToDisplay();
    coordinateSystem->SetValue(1.0 * pos[0], 1.0 * pos[1]);
    double* worldPos = coordinateSystem->GetComputedWorldValue(this->ren);
    this->cursor->SetFocalPoint(worldPos);
    this->cursor->Modified();
    this->interactor->Render();
}
