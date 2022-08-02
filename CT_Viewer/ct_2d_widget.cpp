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
    vtkNew<vtkGenericOpenGLRenderWindow> win;
    win->SetSize(230, 180);
    win->SetMultiSamples(0);
    this->renWin = win;
    this->SetRenderWindow(this->renWin);

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

void CT_2d_Widget::renderCTReslice(vtkImageReslice * reslice)
{
    this->reslice = reslice;

    // initialize the model center and the slice center
    // the slice center is at model center when image reslice
    // created, and it will change after cursor position update
    for (int i = 0; i < 3; i++) {
        this->sliceCenter[i] = reslice->GetResliceAxesOrigin()[i];
        this->modelCenter[i] = reslice->GetResliceAxesOrigin()[i];
    }

    // define look up table and create the image actor
    vtkNew<vtkLookupTable> lookupTable;
    lookupTable->SetRange(-1000, 1000);
    lookupTable->SetValueRange(0.0, 1.0);
    lookupTable->SetSaturationRange(0.0, 0.0);
    lookupTable->SetRampToLinear();
    lookupTable->Build();

    vtkNew<vtkImageMapToColors> mapToColors;
    this->mapToColor = mapToColors;
    mapToColors->SetLookupTable(lookupTable);
    mapToColors->SetInputConnection(this->reslice->GetOutputPort());
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
    callback->setImageReslice(this->reslice);
    callback->setMapToColors(mapToColors);
    callback->setInteractor(interactor);
    callback->setCursor(cursor);
    callback->setInteractor(interactor);
    callback->setRender(this->ren);
    callback->setQTWidget(this);

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
    this->reslice->SetInputData(0, ctImage);
    this->reslice->Modified();
    this->renWin->Render();
}

void CT_2d_Widget::sendPosSignal()
{
    emit cursorPosChange(this->cursor->GetFocalPoint()[0], this->cursor->GetFocalPoint()[1], this->mode);
}

void CT_2d_Widget::sendResliceSignal()
{
    double position[3];
    this->reslice->GetResliceAxesOrigin(position);

    // update the current view's scrollbar
    this->scrollBar->blockSignals(true);
    this->scrollBar->setValue(position[this->mode]);
    this->scrollBar->blockSignals(false);
    // send the signal to other two views to update the cursor
    emit reslicePosChange(position[this->mode], this->mode);
}

double * CT_2d_Widget::getModelCenter()
{
    return this->modelCenter;
}

// this function must go after renderCTReslices
void CT_2d_Widget::setScrollBar(QScrollBar * scrollBar)
{
    this->scrollBar = scrollBar;
    scrollBar->setMinimum(0);
    scrollBar->setMaximum(this->modelCenter[mode] * 2); // WARNING: not sure about the boundary, should we -1?
    
    // block the signal when we set value without user interactions
    // this will avoid the signal emitting and other two views being affected
    scrollBar->blockSignals(true);
    scrollBar->setValue(this->modelCenter[mode]);
    scrollBar->blockSignals(false);
    scrollBar->setTracking(true);
}

// must be done after set view mode
void CT_2d_Widget::setWindowTitle()
{
    setHeader(this->ren, this->mode);
}

void CT_2d_Widget::updateCursorPos()
{
    double x, y;

    // the transform of transform to 3D position is very tricky, be cautious!!!
    switch (this->mode) {
    case Sagittal:
        x = this->modelCenter[1] - this->sliceCenter[1];
        y = this->modelCenter[2] - this->sliceCenter[2];
        break;
    case Coronal:
        x = this->modelCenter[0] - this->sliceCenter[0];
        y = this->modelCenter[2] - this->sliceCenter[2];
        break;
    case Axial:
        x = this->modelCenter[0] - this->sliceCenter[0];
        y = this->sliceCenter[1] - this->modelCenter[1];
        break;
    }
    this->cursor->SetFocalPoint(x, y, 0);
    this->cursor->Modified();
    this->renWin->Render();
}

void CT_2d_Widget::updateReslicePos()
{
    switch (this->mode) {
    case Sagittal:
        // a lot of corner cases for these interactions, don't know why
        this->reslice->SetResliceAxesOrigin(this->modelCenter[0] * 2 - this->sliceCenter[0], this->modelCenter[1], this->modelCenter[2]);
        break;
    case Coronal:
        this->reslice->SetResliceAxesOrigin(this->modelCenter[0], this->sliceCenter[1], this->modelCenter[2]);
        break;
    case Axial:
        this->reslice->SetResliceAxesOrigin(this->modelCenter[0], this->modelCenter[1], this->sliceCenter[2]);
        break;
    }

    // also update the scroll bar
    this->scrollBar->blockSignals(true);
    if (this->mode == Sagittal) {
        this->scrollBar->setValue(this->modelCenter[0] * 2 - this->sliceCenter[this->mode]);
    } else {
        this->scrollBar->setValue(this->sliceCenter[this->mode]);
    }
    this->scrollBar->blockSignals(false);

    this->reslice->Modified();
    this->renWin->Render();
}

void CT_2d_Widget::updateWhenReslicePosChange(int z, ViewMode comingSignalViewMode)
{
    if (this->mode == comingSignalViewMode) {
        return; // if same window, no need to update
    }
    this->sliceCenter[comingSignalViewMode] = z;
    updateCursorPos();
}

void CT_2d_Widget::updateColorMap(int lower, int upper)
{
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

void CT_2d_Widget::updateWhenScrollbarChanged(int value)
{
    switch (this->mode) {
    case Sagittal:
        this->reslice->SetResliceAxesOrigin(value, this->modelCenter[1], this->modelCenter[2]);
        break;
    case Coronal:
        this->reslice->SetResliceAxesOrigin(this->modelCenter[0], value, this->modelCenter[2]);
        break;
    case Axial:
        this->reslice->SetResliceAxesOrigin(this->modelCenter[0], this->modelCenter[1], value);
        break;
    }
    this->reslice->Update();
    this->renWin->GetInteractor()->Render();
    if (this->mode == Sagittal) {
        emit reslicePosChange(this->modelCenter[0] * 2 - value, this->mode);
    } else {
        emit reslicePosChange(value, this->mode);
    }
}

void CT_2d_Widget::updateWhenCursorPosChange(int x, int y, ViewMode comingSignalViewMode)
{
    if (this->mode == comingSignalViewMode) {
        return; // if same window, no need to update
    }

    // the transformation of cursor position to global position is very tricky, be cautious!!!
    switch (comingSignalViewMode) {
    case Sagittal:
        this->sliceCenter[1] = modelCenter[1] - x;
        this->sliceCenter[2] = modelCenter[2] - y;
        break;
    case Coronal:
        this->sliceCenter[0] = modelCenter[0] - x;
        this->sliceCenter[2] = modelCenter[2] - y;
        break;
    case Axial:
        this->sliceCenter[0] = modelCenter[0] - x;
        this->sliceCenter[1] = y + modelCenter[1];
        break;
    }
    updateCursorPos();
    updateReslicePos();
}