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
}


CT_3d_Widget::~CT_3d_Widget()
{
}

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

void CT_3d_Widget::addScrew(const char * screwName)
{
    PlantingScrews* screw = new PlantingScrews(screwName);
    screw->getScrewWidget()->SetInteractor(this->interactor);
    screw->getScrewWidget()->On();
    this->render->AddActor(screw->getScrewActor());
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
}
