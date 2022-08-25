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
    this->imageWindowWidget = nullptr;
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

void resliceInteractionCallback::setImageReslice(CT_2d_Widget * imageWindowWidget)
{
    this->imageWindowWidget = imageWindowWidget;
}

void resliceInteractionCallback::setCTImage(CT_Image * ctImage)
{
    this->ctImage = ctImage;
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

    // if the position is out of bound, set it to the bound
    double bounds[6];
    this->cursor->GetModelBounds(bounds);
    worldPos[0] = std::max(worldPos[0], bounds[0]);
    worldPos[0] = std::min(worldPos[0], bounds[1]);
    worldPos[1] = std::max(worldPos[1], bounds[2]);
    worldPos[1] = std::min(worldPos[1], bounds[3]);

    // update the sliceCenter in CT_Image
    int depth = this->ctImage->getSliceCenter()[this->imageWindowWidget->getViewMode()];
    double* modelBounds = this->ctImage->getModelBounds();
    switch (this->imageWindowWidget->getViewMode()) {
    case Sagittal:
        // sagittal view is different, need special care. The leftmost cursor maps to right most slice bound
        this->ctImage->updateSliceCenter(depth, modelBounds[2] + bounds[1] - worldPos[0], modelBounds[5] - bounds[3] + worldPos[1]);
        break;
    case Coronal:
        this->ctImage->updateSliceCenter(modelBounds[1] - bounds[1] + worldPos[0], depth, modelBounds[5] - bounds[3] + worldPos[1]);
        break;
    case Axial:
        this->ctImage->updateSliceCenter(modelBounds[1] - bounds[1] + worldPos[0], modelBounds[3] - bounds[3] + worldPos[1], depth);
        break;
    }
}

void resliceInteractionCallback::updateReslicePos()
{
    // get the distance of slices user want to skip
    int lastPos[2], curPos[2];
    this->interactor->GetLastEventPosition(lastPos);
    this->interactor->GetEventPosition(curPos);
    int deltaY = lastPos[1] - curPos[1];
    
    // set the new reslice origin
    this->imageWindowWidget->getReslice()->Update();
    double spacing = this->imageWindowWidget->getReslice()->GetOutput()->GetSpacing()[2];
    vtkMatrix4x4* matrix = this->imageWindowWidget->getReslice()->GetResliceAxes();
    double point[4], center[4];
    point[0] = 0.0;
    point[1] = 0.0;
    point[2] = spacing * deltaY;
    point[3] = 1.0;
    matrix->MultiplyPoint(point, center);

    // if the reslice is out of the image, then cancel that origin move
    if (isOutBound(center[0], center[1], center[2])) {
        return;
    }

    // update the slice center in CT_Image
    this->ctImage->updateSliceCenter(center[0], center[1], center[2]);
}

bool resliceInteractionCallback::isOutBound(double x, double y, double z)
{
    double* bounds = this->ctImage->getModelBounds();
    if (x < bounds[0] || x > bounds[1] || y < bounds[2] || y > bounds[3] || z < bounds[4] || z > bounds[5])
        return true;
    else
        return false;
}
