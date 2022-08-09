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
#include <vtkLegendScaleActor.h>
#include <vtkCamera.h>
#include <QDebug>
#include <vtkActor.h>
#include <vtkProp3D.h>
#include <vtkPolyDataMapper.h>
#include <vtkTransformPolyDataFilter.h>
#include "box_widget_callback.h"

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
    this->ren = render;

    // set the title for main view widget
    setHeader(render, ThreeDimension);

    // init last picked screw vtk property (nasty design)
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
    this->ren->AddActor(ctVolume);
    this->ren->ResetCamera();

    // add scale actor
    vtkNew<vtkLegendScaleActor> legendScaleActor;
    legendScaleActor->AllAnnotationsOff();
    legendScaleActor->SetRightAxisVisibility(true);
    this->ren->AddActor(legendScaleActor);

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

void CT_3d_Widget::reset()
{
    this->ren->ResetCamera();
}

void CT_3d_Widget::getCameraSettings(double * position, double * focalPoint)
{
    position[0] = this->ren->GetActiveCamera()->GetPosition()[0];
    position[1] = this->ren->GetActiveCamera()->GetPosition()[1];
    position[2] = this->ren->GetActiveCamera()->GetPosition()[2];

    focalPoint[0] = this->ren->GetActiveCamera()->GetFocalPoint()[0];
    focalPoint[1] = this->ren->GetActiveCamera()->GetFocalPoint()[1];
    focalPoint[2] = this->ren->GetActiveCamera()->GetFocalPoint()[2];
}

void CT_3d_Widget::addScrew(PlantingScrews* screw)
{
    // set the box widget interactor and start the box widget
    vtkNew<BoxWidgetCallback> callback;
    callback->setScrew(screw);
    screw->getScrewWidget()->AddObserver(vtkCommand::InteractionEvent, callback);
    screw->getScrewWidget()->SetInteractor(this->interactor);
    screw->getScrewWidget()->On();
    this->ren->AddActor(screw->getScrewActor());
    this->renWin->Render();
}

void CT_3d_Widget::removeAll(QVector<PlantingScrews*> screwList) // this method is suspicious of memory leak!!!
{
    for (auto screw : screwList) {
        screw->getScrewWidget()->SetEnabled(0);
        this->ren->RemoveActor(screw->getScrewActor());
        delete screw;
    }
    screwList.clear();
    this->renWin->Render();
}
