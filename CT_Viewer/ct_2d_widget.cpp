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
#include <vtkCursor2D.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkInteractorStyleImage.h>
#include <vtkMatrix4x4.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkCoordinate.h>

CT_2d_Widget::CT_2d_Widget(QWidget *parent = Q_NULLPTR)
{
    vtkNew<vtkGenericOpenGLRenderWindow> win;
    win->SetSize(230, 180);
    win->SetMultiSamples(0);
    this->renWin = win;
    this->SetRenderWindow(this->renWin);

    vtkNew<vtkRenderer> render;
    this->renWin->AddRenderer(render);
    this->render = render;
}


CT_2d_Widget::~CT_2d_Widget()
{
}

void CT_2d_Widget::setViewMode(ViewMode mode)
{
    this->mode = mode;
    setWindowTitle();
}

class ctResliceCallback : public vtkCommand
{
public:
    static ctResliceCallback* New()
    {
        return new ctResliceCallback;
    }
    ctResliceCallback()
    {
        this->Slicing = 0;
        this->ImageReslice = nullptr;
        this->Interactor = nullptr;
    }

    void SetImageReslice(vtkImageReslice* reslice)
    {
        this->ImageReslice = reslice;
    }

    void SetMapToColors(vtkImageMapToColors* colors)
    {
        this->MapToColors = colors;
    }

    void SetInteractor(vtkRenderWindowInteractor* interactor)
    {
        this->Interactor = interactor;
    }

    void SetRenderWindow(vtkRenderWindow* window)
    {
        this->RenderWindow = window;
    }

    void Execute(vtkObject* caller, unsigned long eventId, void* callData) override
    {
        int lastPos[2], curPos[2];
        this->Interactor->GetLastEventPosition(lastPos);
        this->Interactor->GetEventPosition(curPos);
        if (eventId == vtkCommand::RightButtonPressEvent) {
            this->Slicing = 1;
        } else if (eventId == vtkCommand::RightButtonReleaseEvent) {
            this->Slicing = 0;
        } else if (eventId == vtkCommand::MouseMoveEvent) {
            if (this->Slicing) {
                int deltaY = lastPos[1] - curPos[1];
                this->ImageReslice->Update();
                double spacing = this->ImageReslice->GetOutput()->GetSpacing()[2];
                vtkMatrix4x4* matrix = this->ImageReslice->GetResliceAxes();
                double point[4], center[4];
                point[0] = 0.0;
                point[1] = 0.0;
                point[2] = spacing * deltaY;
                point[3] = 1.0;

                matrix->MultiplyPoint(point, center);
                matrix->SetElement(0, 3, center[0]);
                matrix->SetElement(1, 3, center[1]);
                matrix->SetElement(2, 3, center[2]);

                this->MapToColors->Update();
                this->OutputImageData = this->MapToColors->GetOutput();
                this->Interactor->Render();

            } else {
                vtkInteractorStyle* style = vtkInteractorStyle::SafeDownCast(this->Interactor->GetInteractorStyle());
                if (style)
                    style->OnMouseMove();
            }
        }
    }

private:
    int Slicing;
    vtkImageReslice* ImageReslice;
    vtkImageMapToColors* MapToColors;
    vtkRenderWindowInteractor* Interactor;
    vtkRenderWindow* RenderWindow;
    vtkImageData* OutputImageData;
};

class vtkImageInteractionCallback : public vtkCommand
{
public:
    static vtkImageInteractionCallback* New()
    {
        return new vtkImageInteractionCallback;
    }

    void setCursor(vtkSmartPointer<vtkCursor2D> cursor)
    {
        this->cursor = cursor;
    }

    void setInteractor(vtkSmartPointer<vtkRenderWindowInteractor> interactor)
    {
        this->interactor = interactor;
    }

    void setRender(vtkSmartPointer<vtkRenderer> render)
    {
        this->ren = render;
    }

    void Execute(vtkObject* caller, unsigned long eventId, void* callData) override
    {
        if (eventId == vtkCommand::LeftButtonPressEvent) {
            isToggled = true;
        } else if (eventId == vtkCommand::LeftButtonReleaseEvent) {
            isToggled = false;
            updateCursorPos();
        } else {
            if (!isToggled) {
                return;
            }
            updateCursorPos();
        }
    }

private:
    vtkSmartPointer<vtkCursor2D> cursor;
    vtkSmartPointer<vtkRenderWindowInteractor> interactor;
    vtkSmartPointer<vtkRenderer> ren;
    bool isToggled = false;
    void updateCursorPos()
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
};

void CT_2d_Widget::renderCTReslice(vtkImageReslice * reslice)
{
    this->reslice = reslice;

    // define look up table and create the image actor
    vtkNew<vtkLookupTable> lookupTable;
    lookupTable->SetRange(0, 255);
    lookupTable->SetValueRange(0.0, 1.0);
    lookupTable->SetSaturationRange(0.0, 0.0);
    lookupTable->SetRampToLinear();
    lookupTable->Build();

    vtkNew<vtkImageMapToColors> mapToColors;
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
    this->render->AddActor(imageActor);
    this->render->AddActor(cursorActor);
    this->render->ResetCamera();

    vtkNew<vtkRenderWindowInteractor> interactor;
    interactor->SetRenderWindow(this->renWin);
    vtkNew<vtkInteractorStyleImage> imageStyle;
    interactor->SetInteractorStyle(imageStyle);

    // define logics for reslice the model
    vtkNew<ctResliceCallback> callback;
    callback->SetImageReslice(this->reslice);
    callback->SetMapToColors(mapToColors);
    callback->SetInteractor(interactor);
    callback->SetRenderWindow(this->renWin);

    imageStyle->AddObserver(vtkCommand::RightButtonPressEvent, callback);
    imageStyle->AddObserver(vtkCommand::RightButtonReleaseEvent, callback);
    imageStyle->AddObserver(vtkCommand::MouseMoveEvent, callback);

    // add interactions for cursors
    vtkNew<vtkImageInteractionCallback> cursorCallback;
    cursorCallback->setCursor(cursor);
    cursorCallback->setInteractor(interactor);
    cursorCallback->setRender(this->render);

    imageStyle->AddObserver(vtkCommand::LeftButtonPressEvent, cursorCallback);
    imageStyle->AddObserver(vtkCommand::LeftButtonReleaseEvent, cursorCallback);
    imageStyle->AddObserver(vtkCommand::MouseMoveEvent, cursorCallback);
    
    // generate the scene
    this->renWin->Render();
}

void CT_2d_Widget::updateCTReslice(vtkImageData* ctImage)
{
    this->reslice->SetInputData(0, ctImage);
    this->reslice->Modified();
    this->renWin->Render();
}

// must be done after set view mode
void CT_2d_Widget::setWindowTitle()
{
    setHeader(this->render, this->mode);
}