#include <vtkAutoInit.h> 
VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingVolumeOpenGL2);
VTK_MODULE_INIT(vtkRenderingFreeType);
#include "ct_2d_widget.h"
#include <vtkNew.h>
#include <vtkLookupTable.h>
#include <vtkImageMapToColors.h>
#include <vtkImageActor.h>
#include <vtkImageMapper3D.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkInteractorStyleImage.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include "reslice_interaction_callback.h"
#include <QDebug>

CT_2d_Widget::CT_2d_Widget(QWidget *parent = Q_NULLPTR)
{
    // create the render window and set it to the CT_2D_widget
    vtkNew<vtkGenericOpenGLRenderWindow> win;
    win->SetSize(230, 180);
    win->SetMultiSamples(0);
    this->renWin = win;
    this->SetRenderWindow(this->renWin);

    // create the render and add it to the window
    vtkNew<vtkRenderer> render;
    this->renWin->AddRenderer(render);
    this->ren = render;
}

CT_2d_Widget::~CT_2d_Widget()
{
}

void CT_2d_Widget::setViewMode(ViewMode mode)
{
    this->mode = mode;
    setWindowTitle();
}

void CT_2d_Widget::renderCTReslice(CT_Image * ctImage)
{
    this->ctImage = ctImage;

    // define look up table and create the image actor
    vtkNew<vtkLookupTable> lookupTable;
    lookupTable->SetRange(this->ctImage->getContrastThreshold()[0], this->ctImage->getContrastThreshold()[1]);
    lookupTable->SetValueRange(0.0, 1.0);
    lookupTable->SetSaturationRange(0.0, 0.0);
    lookupTable->SetRampToLinear();
    lookupTable->Build();

    vtkNew<vtkImageMapToColors> mapToColors;
    this->mapToColor = mapToColors;
    mapToColors->SetLookupTable(lookupTable);
    mapToColors->SetInputConnection(this->ctImage->getCTImageReslice(this->mode)->GetOutputPort());
    mapToColors->Update();

    vtkNew<vtkImageActor> imageActor;
    imageActor->GetMapper()->SetInputConnection(mapToColors->GetOutputPort());

    // define the cursor 2D
    vtkNew<vtkCursor2D> cursor;
    double bounds[6];
    mapToColors->GetOutput()->GetBounds(bounds);
    cursor->SetModelBounds(bounds);
    cursor->AllOn();
    cursor->OutlineOff();
    cursor->SetFocalPoint(0, 0, 0);
    cursor->Update();
    this->cursor = cursor;

    vtkNew<vtkPolyDataMapper> cursorMapper;
    cursorMapper->SetInputConnection(cursor->GetOutputPort());
    vtkNew<vtkActor> cursorActor;
    cursorActor->GetProperty()->SetColor(200, 50, 0);
    cursorActor->SetMapper(cursorMapper);
    
    // put the cursor above the image
    double pos[3];
    imageActor->GetPosition(pos);
    cursorActor->SetPosition(pos[0], pos[1], pos[2] + 1);

    // add actors to render
    this->ren->AddActor(imageActor);
    this->ren->AddActor(cursorActor);
    this->ren->ResetCamera();

    vtkNew<vtkRenderWindowInteractor> interactor;
    interactor->SetRenderWindow(this->renWin);
    vtkNew<vtkInteractorStyleImage> imageStyle;
    interactor->SetInteractorStyle(imageStyle);

    // define logics for reslice the model and cursor interaction
    vtkNew<resliceInteractionCallback> callback;
    callback->setImageReslice(this);
    callback->setInteractor(interactor);
    callback->setCursor(cursor);
    callback->setInteractor(interactor);
    callback->setRender(this->ren);
    callback->setCTImage(this->ctImage);

    imageStyle->AddObserver(vtkCommand::RightButtonPressEvent, callback);
    imageStyle->AddObserver(vtkCommand::RightButtonReleaseEvent, callback);
    imageStyle->AddObserver(vtkCommand::MouseMoveEvent, callback);
    imageStyle->AddObserver(vtkCommand::LeftButtonPressEvent, callback);
    imageStyle->AddObserver(vtkCommand::LeftButtonReleaseEvent, callback);
    
    // generate the scene
    this->renWin->Render();
}

void CT_2d_Widget::updateCTReslice(vtkImageData* ctImage)
{
    this->ctImage->getCTImageReslice(this->mode)->SetInputData(0, ctImage);
    this->ctImage->getCTImageReslice(this->mode)->Modified();
    this->renWin->Render();
}

// this function must go after renderCTReslices
void CT_2d_Widget::setScrollBar(QScrollBar * scrollBar)
{
    this->scrollBar = scrollBar;
    scrollBar->setMinimum(0);
    scrollBar->setMaximum(this->ctImage->getModelCenter()[mode] * 2); // WARNING: not sure about the boundary, should we -1?
    
    // block the signal when we set value without user interactions
    // this will avoid the signal emitting and other two views being affected
    scrollBar->blockSignals(true);
    scrollBar->setValue(this->ctImage->getModelCenter()[mode]);
    scrollBar->blockSignals(false);
    scrollBar->setTracking(true);
}

vtkImageReslice * CT_2d_Widget::getReslice()
{
    return this->ctImage->getCTImageReslice(this->mode);
}

// to check which view mode, sagittal, coronal or axial
ViewMode CT_2d_Widget::getViewMode()
{
    return this->mode;
}

// must be done after set view mode
void CT_2d_Widget::setWindowTitle()
{
    setHeader(this->ren, this->mode);
}

// this function will update reslice, cursor and scroll bar position
// x, y, z represents sagittal, coronal and axial axis
void CT_2d_Widget::updateWhenSliceCenterChange(double x, double y, double z)
{
    double cursor_pos_x, cursor_pos_y;

    // block the scroll bar signal so it will not call updateScrollBar slot
    this->scrollBar->blockSignals(true);
    switch (this->mode) {
    case Sagittal:
        // a lot of corner cases for these interactions, don't know why
        this->ctImage->getCTImageReslice(this->mode)->SetResliceAxesOrigin(this->ctImage->getModelCenter()[0] * 2 - x, this->ctImage->getModelCenter()[1], this->ctImage->getModelCenter()[2]);
        cursor_pos_x = this->ctImage->getModelCenter()[1] - y;
        cursor_pos_y = this->ctImage->getModelCenter()[2] - z;
        this->scrollBar->setValue(x);
        break;
    case Coronal:
        this->ctImage->getCTImageReslice(this->mode)->SetResliceAxesOrigin(this->ctImage->getModelCenter()[0], y, this->ctImage->getModelCenter()[2]);
        cursor_pos_x = this->ctImage->getModelCenter()[0] - x;
        cursor_pos_y = this->ctImage->getModelCenter()[2] - z;
        this->scrollBar->setValue(y);
        break;
    case Axial:
        this->ctImage->getCTImageReslice(this->mode)->SetResliceAxesOrigin(this->ctImage->getModelCenter()[0], this->ctImage->getModelCenter()[1], z);
        cursor_pos_x = this->ctImage->getModelCenter()[0] - x;
        cursor_pos_y = y - this->ctImage->getModelCenter()[1];
        this->scrollBar->setValue(z);
        break;
    }
    // unblock the scroll bar for incoming scroll movements
    this->scrollBar->blockSignals(false);

    // update the cursor and reslice and notify the pipeline
    this->cursor->SetFocalPoint(cursor_pos_x, cursor_pos_y, 0);
    this->cursor->Modified();
    this->ctImage->getCTImageReslice(this->mode)->Modified();

    // render the scene again to show the result
    this->renWin->Render();
}

// adjust the lookup table
// re-render the scene to update the changes
void CT_2d_Widget::updateColorMap(int lower, int upper)
{
    // create a new lookup table for updated contrast value
    vtkNew<vtkLookupTable> lookupTable;
    lookupTable->SetRange(lower, upper);
    lookupTable->SetValueRange(0.0, 1.0);
    lookupTable->SetSaturationRange(0.0, 0.0);
    lookupTable->SetRampToLinear();
    lookupTable->Build();

    this->mapToColor->SetLookupTable(lookupTable);
    this->mapToColor->Modified();
    this->renWin->Render();
}
