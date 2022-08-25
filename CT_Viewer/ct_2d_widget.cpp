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
#include <vtkCutter.h>
#include <vtkStripper.h>
#include <vtkContourTriangulator.h>
#include <vtkTransform.h>

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
    // also we need to set the normal of the plane
    vtkNew<vtkPlane> plane;
    switch (mode) {
    case Sagittal:
        plane->SetNormal(1, 0, 0);
        break;
    case Coronal:
        plane->SetNormal(0, 1, 0);
        break;
    case Axial:
        plane->SetNormal(0, 0, 1);
        break;
    }
    this->plane = plane;
}

vtkSmartPointer<vtkProp> CT_2d_Widget::renderCTReslice(CT_Image * ctImage)
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
    cursor->GetModelBounds(this->cursorBounds);

    vtkNew<vtkPolyDataMapper> cursorMapper;
    cursorMapper->SetInputConnection(cursor->GetOutputPort());
    vtkNew<vtkActor> cursorActor;
    cursorActor->GetProperty()->SetColor(200, 50, 0);
    cursorActor->GetProperty()->SetLineWidth(2.0);
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
    callback->setRender(this->ren);
    callback->setCTImage(this->ctImage);

    imageStyle->AddObserver(vtkCommand::RightButtonPressEvent, callback);
    imageStyle->AddObserver(vtkCommand::RightButtonReleaseEvent, callback);
    imageStyle->AddObserver(vtkCommand::MouseMoveEvent, callback);
    imageStyle->AddObserver(vtkCommand::LeftButtonPressEvent, callback);
    imageStyle->AddObserver(vtkCommand::LeftButtonReleaseEvent, callback);
    
    // generate the scene
    this->renWin->Render();
    return imageActor;
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
    double* bounds = this->ctImage->getModelBounds();
    scrollBar->setMinimum(bounds[mode * 2]);
    scrollBar->setMaximum(bounds[mode * 2 + 1]);
    
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

vtkProp* CT_2d_Widget::addScrew(PlantingScrews* screw)
{
    double* sliceCenter = this->ctImage->getSliceCenter();
    // set the plane's position as same as reslice axes origin
    this->plane->SetOrigin(sliceCenter);
    
    // init the cutter and set the cut function to the plane
    vtkNew<vtkCutter> cutter;
    cutter->SetCutFunction(this->plane);
    cutter->SetInputData(screw->getScrewSource());
    vtkNew<vtkStripper> stripper;
    stripper->SetInputConnection(cutter->GetOutputPort());

    // this contour triangulate filter will fill the contour
    vtkNew<vtkContourTriangulator> solidCut;
    solidCut->SetInputConnection(stripper->GetOutputPort());
    vtkNew<vtkPolyDataMapper> coneContourMapper;
    coneContourMapper->SetInputConnection(solidCut->GetOutputPort());
    
    // create the actor
    vtkNew<vtkActor> screwContour;
    screwContour->GetProperty()->SetColor(255, 0, 0);
    screwContour->SetMapper(coneContourMapper);
    // to put the cone contour to the same position as the reslice image
    vtkNew<vtkTransform> t;
    switch (this->mode) {
    case Sagittal:
        screwContour->SetPosition(-sliceCenter[0] - 1, -sliceCenter[1], -sliceCenter[2]);
        t->RotateY(90);
        t->RotateX(-90);
        screwContour->SetUserTransform(t);
        break;
    case Coronal:
        screwContour->SetPosition(-sliceCenter[0], -sliceCenter[1] + 1, -sliceCenter[2]);
        t->RotateX(90);
        t->RotateY(180);
        screwContour->SetUserTransform(t);
        break;
    case Axial:
        screwContour->SetPosition(-sliceCenter[0], -sliceCenter[1], -sliceCenter[2] + 1);
        screwContour->SetScale(-1, 1, 1);
        break;
    }
    screwContour->GetProperty()->SetColor(255, 0, 0);
    
    // add the contour to the 2D view render
    this->ren->AddActor(screwContour);
    this->screwContourList.append(screwContour);
    this->ren->ResetCamera();
    this->renWin->Render();

    return screwContour;
}

void CT_2d_Widget::removeAll()
{
    for (auto actor : this->screwContourList) {
        this->ren->RemoveActor(actor);
    }
    this->renWin->Render();
    screwContourList.clear();
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
        this->scrollBar->setValue(x);
        x = this->ctImage->getModelBounds()[1] - x + this->ctImage->getModelBounds()[0];
        this->ctImage->getCTImageReslice(this->mode)->SetResliceAxesOrigin(x, this->ctImage->getModelCenter()[1], this->ctImage->getModelCenter()[2]);
        // sagittal view is different, need special care, leftmost cursor maps to rightmost model
        cursor_pos_x = this->cursorBounds[1] + this->ctImage->getModelBounds()[2] - y;
        cursor_pos_y = this->cursorBounds[3] - this->ctImage->getModelBounds()[5] + z;
        if (this->screwContourList.size() > 0) {
            this->plane->SetOrigin(x, this->ctImage->getModelCenter()[1], this->ctImage->getModelCenter()[2]);
            this->plane->Modified();
            for (auto actor : this->screwContourList) {
                    actor->SetPosition(-x - 1, -this->ctImage->getModelCenter()[1], -this->ctImage->getModelCenter()[2]);
            }
        }
        break;
    case Coronal:
        this->ctImage->getCTImageReslice(this->mode)->SetResliceAxesOrigin(this->ctImage->getModelCenter()[0], y, this->ctImage->getModelCenter()[2]);
        cursor_pos_x = this->cursorBounds[1] - this->ctImage->getModelBounds()[1] + x;
        cursor_pos_y = this->cursorBounds[3] - this->ctImage->getModelBounds()[5] + z;
        this->scrollBar->setValue(y);
        if (this->screwContourList.size() > 0) {
            this->plane->SetOrigin(this->ctImage->getModelCenter()[0], y, this->ctImage->getModelCenter()[2]);
            this->plane->Modified();
            for (auto actor : this->screwContourList) {
                actor->SetPosition(-this->ctImage->getModelCenter()[0], -y+1, -this->ctImage->getModelCenter()[2]);
            }
        }
        break;
    case Axial:
        this->ctImage->getCTImageReslice(this->mode)->SetResliceAxesOrigin(this->ctImage->getModelCenter()[0], this->ctImage->getModelCenter()[1], z);
        cursor_pos_x = this->cursorBounds[1] - this->ctImage->getModelBounds()[1] + x;
        cursor_pos_y = this->cursorBounds[3] - this->ctImage->getModelBounds()[3] + y;
        this->scrollBar->setValue(z);
        if (this->screwContourList.size() > 0) {
            this->plane->SetOrigin(this->ctImage->getModelCenter()[0], this->ctImage->getModelCenter()[1], z);
            this->plane->Modified();
            for (auto actor : this->screwContourList) {
                actor->SetPosition(-this->ctImage->getModelCenter()[0], -this->ctImage->getModelCenter()[1], -z+1);
            }
        }
        break;
    }
    // unblock the scroll bar for incoming scroll movements
    this->scrollBar->blockSignals(false);

    // update the cursor and reslice and notify the pipeline
    this->cursor->SetFocalPoint(cursor_pos_x, cursor_pos_y, 0);
    this->cursor->Modified();
    this->ctImage->getCTImageReslice(this->mode)->Modified();

    // render the scene again to show the result
    this->ren->ResetCamera();
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
