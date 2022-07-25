#include <vtkAutoInit.h> 
VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingVolumeOpenGL2);
VTK_MODULE_INIT(vtkRenderingFreeType);
#include "ct_3d_widget.h"
#include <vtkNew.h>
#include <vtkDICOMImageReader.h>
#include <vtkImageCast.h>
#include "vtkHelper.h"
#include <vtkPiecewiseFunction.h>
#include <vtkVolume.h>
#include <vtkColorTransferFunction.h>
#include <vtkVolumeProperty.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkTransform.h>
#include <vtkNamedColors.h>
#include <QDebug>

namespace {
    // this is a private class for progress update
    class ReadCTProgressUpdate : public vtkCommand
    {
    public:
        static ReadCTProgressUpdate* New()
        {
            return new ReadCTProgressUpdate;
        }
        void setDialog(QProgressDialog* dialog)
        {
            this->dialog = dialog;
        }
        virtual void Execute(vtkObject*caller, unsigned long eventId, void* callData)
        {
            double* progress = static_cast<double*>(callData);
            this->dialog->setValue((int)(*progress * 100));
        }
    private:
        QProgressDialog* dialog;
    };

    // callback for vtk box widget, the actor will transform with the
    //  outline widget together
    class WidgetCallback : public vtkCommand
    {
    public:
        static WidgetCallback* New()
        {
            return new WidgetCallback;
        }
        virtual void Execute(vtkObject* caller, unsigned long, void*)
        {
            vtkNew<vtkTransform> t;
            vtkBoxWidget* widget = reinterpret_cast<vtkBoxWidget*>(caller);
            widget->GetTransform(t);
            widget->GetProp3D()->SetUserTransform(t);
            
            // if the current active widget is the same, return
            if (this->qtWidget->getActiveScrew() == widget)
                return;
            
            // first reset the property of last picked actor if this is not the first screw created
            if (this->qtWidget->getActiveScrew() != nullptr) {
                vtkActor* lastPickedActor = reinterpret_cast<vtkActor*>(this->qtWidget->getActiveScrew()->GetProp3D());
                lastPickedActor->GetProperty()->DeepCopy(this->qtWidget->getLastPickedProperty());
            }
            this->qtWidget->setActiveScrew(widget);
            // update the last picked actor to current actor
            vtkActor* currentPickedActor = reinterpret_cast<vtkActor*>(widget->GetProp3D());
            this->qtWidget->getLastPickedProperty()->DeepCopy(currentPickedActor->GetProperty());
            
            // highlight the current chosen actor
            vtkNew<vtkNamedColors> colors;
            currentPickedActor->GetProperty()->SetColor(colors->GetColor3d("cadmium_lemon").GetData());
            currentPickedActor->GetProperty()->SetDiffuse(1.0);
            currentPickedActor->GetProperty()->SetSpecular(0.0);
        }
        void setQTWidget(CT_3d_Widget* qtWidget)
        {
            this->qtWidget = qtWidget;
        }

    private:
        CT_3d_Widget* qtWidget;
    };
}

CT_3d_Widget::CT_3d_Widget(QWidget *parent = Q_NULLPTR)
{
    // create the vtk window
    vtkNew<vtkGenericOpenGLRenderWindow> win;
    win->SetSize(700, 370);
    win->SetMultiSamples(0);
    this->renWin = win;
    this->SetRenderWindow(this->renWin);

    // create the render
    vtkNew<vtkRenderer> render;
    this->renWin->AddRenderer(render);
    this->render = render;

    // set the title for main view widget
    setHeader(render, ThreeDimension);

    // init last picked property (nasty design)
    this->lastPickedProperty = vtkProperty::New();
}


CT_3d_Widget::~CT_3d_Widget()
{
    this->lastPickedProperty->Delete();
}

void CT_3d_Widget::loadCT()
{
    // create mapper, filters to render Dicom data
    vtkNew<vtkPiecewiseFunction> opacityTransfunc;
    opacityTransfunc->AddPoint(70, 0.0);
    opacityTransfunc->AddPoint(90, 0.4);
    opacityTransfunc->AddPoint(180, 0.6);

    vtkNew<vtkColorTransferFunction> colorTransferFunction;
    colorTransferFunction->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
    colorTransferFunction->AddRGBPoint(64.0, 0.25, 0.25, 0.25);
    colorTransferFunction->AddRGBPoint(190.0, 0.5, 0.5, 0.5);
    colorTransferFunction->AddRGBPoint(220.0, 1, 1, 1);

    vtkNew<vtkVolumeProperty> ctVolumeProperty;
    ctVolumeProperty->SetColor(colorTransferFunction);
    ctVolumeProperty->SetScalarOpacity(opacityTransfunc);
    ctVolumeProperty->ShadeOn();
    ctVolumeProperty->SetInterpolationTypeToLinear();
    ctVolumeProperty->SetAmbient(0.2);
    ctVolumeProperty->SetDiffuse(0.9);
    ctVolumeProperty->SetSpecular(0.5);
    ctVolumeProperty->SetSpecularPower(50);

    // connect the pipeline and create volume actor
    vtkNew<vtkGPUVolumeRayCastMapper> ctMapper;
    ctMapper->SetInputData(this->ctImage);
    vtkNew<vtkVolume> ctVolume;
    ctVolume->SetMapper(ctMapper);
    ctVolume->SetProperty(ctVolumeProperty);
    ctVolume->PickableOff();

    // add actor to render
    this->render->AddActor(ctVolume);
    this->render->ResetCamera();

    // add interactor and interaction style
    vtkNew<vtkRenderWindowInteractor> interactor;
    interactor->SetRenderWindow(this->renWin);
    vtkNew<vtkInteractorStyleTrackballCamera> style;
    interactor->SetInteractorStyle(style);
    interactor->Initialize();
    this->interactor = interactor;
    this->renWin->Render();
}

void CT_3d_Widget::setRenderWindowSize(int height, int width)
{
    this->renWin->SetSize(height, width);
}

vtkGenericOpenGLRenderWindow * CT_3d_Widget::getRenderWindow()
{
    return this->renWin;
}

void CT_3d_Widget::setCTImage(vtkImageData * ctImage)
{
    this->ctImage = ctImage;
}

QVector<PlantingScrews*> CT_3d_Widget::getScrewList()
{
    return this->screwList;
}

void CT_3d_Widget::confirmActors()
{
    for (auto screw : this->screwList) {
        screw->getScrewWidget()->EnabledOff();
    }
}

void CT_3d_Widget::moveScrew(ScrewAction action, double value)
{
    if (this->activeScrew == nullptr) {
        return;
    }
    vtkNew<vtkTransform> t;
    this->activeScrew->GetTransform(t);
    switch (action) {
    case UP:
        t->Translate(1, 0, 0);
        break;
    case DOWN:
        t->Translate(-1, 0, 0);
        break;
    case LEFT:
        t->Translate(0, 1, 0);
        break;
    case RIGHT:
        t->Translate(0, -1, 0);
        break;
    case FRONT:
        t->Translate(0, 0, 1);
        break;
    case BACK:
        t->Translate(0, 0, -1);
        break;
    case ROTATE_IS:
        t->RotateX(value - t->GetOrientation()[0]);
        break;
    case ROTATE_LR:
        t->RotateZ(value - t->GetOrientation()[2]);
        break;
    }
    this->activeScrew->SetTransform(t);
    this->activeScrew->InvokeEvent(vtkCommand::InteractionEvent);
    this->renWin->Render();
}

void CT_3d_Widget::setActiveScrew(vtkBoxWidget * activeScrew)
{
    this->activeScrew = activeScrew;
}

vtkBoxWidget * CT_3d_Widget::getActiveScrew()
{
    return this->activeScrew;
}

vtkProperty * CT_3d_Widget::getLastPickedProperty()
{
    return this->lastPickedProperty;
}

void CT_3d_Widget::setLastPickedProperty(vtkProperty * lastPickedProperty)
{
    this->lastPickedProperty = lastPickedProperty;
}

void CT_3d_Widget::addScrew(const char * screwName)
{
    // create a screw according to its type name
    PlantingScrews* screw = new PlantingScrews(screwName);
    
    // set the box widget interactor and start the box widget
    vtkNew<WidgetCallback> callback;
    callback->setQTWidget(this);
    screw->getScrewWidget()->AddObserver(vtkCommand::InteractionEvent, callback);
    screw->getScrewWidget()->SetInteractor(this->interactor);
    screw->getScrewWidget()->On();
    this->render->AddActor(screw->getScrewActor());
    this->renWin->Render();

    // keep the pointer to the list, the newly added screw is active
    this->screwList.push_back(screw);
}

void CT_3d_Widget::removeAll() // this method is suspicious of memory leak!!!
{
    for (auto screw : this->screwList) {
        screw->getScrewWidget()->SetEnabled(0);
        this->render->RemoveActor(screw->getScrewActor());
        delete screw;
    }
    screwList.clear();
    this->renWin->Render();
}
