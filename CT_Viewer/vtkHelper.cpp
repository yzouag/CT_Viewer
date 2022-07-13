#include <vtkAutoInit.h> 
VTK_MODULE_INIT(vtkRenderingOpenGL2); // VTK was built with vtkRenderingOpenGL2
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingVolumeOpenGL2);
#include "vtkHelper.h"
#include <vtkNew.h>
#include <vtkDICOMImageReader.h>
#include <vtkImageCast.h>
#include <vtkNamedColors.h>
#include <vtkPiecewiseFunction.h>
#include <vtkColorTransferFunction.h>
#include <vtkVolumeProperty.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkVolume.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkCommand.h>
#include <vtkImageMapToColors.h>
#include <vtkMatrix4x4.h>
#include <vtkLookupTable.h>
#include <vtkImageActor.h>
#include <vtkImageMapper3D.h>
#include <vtkInteractorStyleImage.h>
#include <vtkConeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkTransform.h>
#include <vtkPointData.h>
#include <vtkMath.h>
#include <vtkImageBlend.h>
#include <vtkImageData.h>
#include <vtkImageStencil.h>
#include <vtkPolyDataToImageStencil.h>

static double const VIEWDIRECTIONMATRIX[3][16] = {
        {0, 0,-1, 0,
        1, 0, 0, 0,
        0,-1, 0, 0,
        0, 0, 0, 1},
        
        {1, 0, 0, 0,
        0, 0, 1, 0,
        0, -1,0, 0,
        0, 0, 0, 1},
        
        {1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1}
};

vtkSmartPointer<vtkGenericOpenGLRenderWindow> createWindow(int x, int y)
{
    vtkNew<vtkGenericOpenGLRenderWindow> win;
    win->SetSize(x, y);
    win->SetMultiSamples(0);
    return win;
}

vtkSmartPointer<vtkRenderer> createRender3D(vtkSmartPointer<vtkImageData> ctImage)
{
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

    vtkNew<vtkGPUVolumeRayCastMapper> ctMapper;
    ctMapper->SetInputData(ctImage);
    vtkNew<vtkVolume> ctVolume;
    ctVolume->SetMapper(ctMapper);
    ctVolume->SetProperty(ctVolumeProperty);
    ctVolume->PickableOff();

    vtkNew<vtkRenderer> ren;
    ren->AddActor(ctVolume);
    ren->ResetCamera();
    return ren;
}

vtkSmartPointer<vtkRenderWindowInteractor> createAndBindInteractor(vtkRenderWindow * renWin)
{
    vtkNew<vtkRenderWindowInteractor> interactor;
    interactor->SetRenderWindow(renWin);
    vtkNew<vtkInteractorStyleTrackballCamera> style;
    interactor->SetInteractorStyle(style);
    interactor->Initialize();
    return interactor;
}

vtkSmartPointer<vtkImageReslice> createReslice(vtkImageData* ctImage, int axis)
{
    int extent[6];
    double spacing[3];
    double origin[3];
    double center[3];

    ctImage->GetExtent(extent);
    ctImage->GetSpacing(spacing);
    ctImage->GetOrigin(origin);

    center[0] = origin[0] + spacing[0] * 0.5 * (extent[0] + extent[1]);
    center[1] = origin[1] + spacing[1] * 0.5 * (extent[2] + extent[3]);
    center[2] = origin[2] + spacing[2] * 0.5 * (extent[4] + extent[5]);

    vtkNew<vtkMatrix4x4> resliceAxes;
    resliceAxes->DeepCopy(VIEWDIRECTIONMATRIX[axis]);
    resliceAxes->SetElement(0, 3, center[0]);
    resliceAxes->SetElement(1, 3, center[1]);
    resliceAxes->SetElement(2, 3, center[2]);

    vtkNew<vtkImageReslice> imageReslice;
    imageReslice->SetInputData(ctImage);
    imageReslice->SetOutputDimensionality(2);
    imageReslice->SetResliceAxes(resliceAxes);
    imageReslice->SetInterpolationModeToLinear();
    return imageReslice;
}

class widgetCallback : public vtkCommand
{
public:
    static widgetCallback* New()
    {
        return new widgetCallback;
    }
    virtual void Execute(vtkObject* caller, unsigned long, void*)
    {
        vtkNew<vtkTransform> t;
        vtkBoxWidget* widget = reinterpret_cast<vtkBoxWidget*>(caller);
        widget->GetTransform(t);
        widget->GetProp3D()->SetUserTransform(t);
    }
};

void addCone(QVector<vtkSmartPointer<vtkBoxWidget>>& coneList, QVector<vtkSmartPointer<vtkActor>>& coneActorList,vtkRenderer* ren, vtkRenderWindowInteractor* interactor)
{
    vtkNew<vtkNamedColors> colors;
    
    // create the cone source
    vtkNew<vtkConeSource> cone;
    cone->SetHeight(20.0);
    cone->SetRadius(5.0);
    cone->SetResolution(100);

    // add source to mapper and then create actor
    vtkNew<vtkPolyDataMapper> coneMapper;
    coneMapper->SetInputConnection(cone->GetOutputPort());
    vtkNew<vtkActor> coneActor;
    coneActor->SetMapper(coneMapper);
    coneActor->GetProperty()->SetColor(colors->GetColor3d("Blue").GetData());
    coneActor->GetProperty()->SetDiffuse(0.7);
    coneActor->GetProperty()->SetSpecular(0.4);
    coneActor->GetProperty()->SetSpecularPower(20);
    ren->AddActor(coneActor);

    // create box widget and attach to coneActor
    vtkNew<vtkBoxWidget> boxWidget;
    boxWidget->SetInteractor(interactor);
    boxWidget->SetProp3D(coneActor);
    boxWidget->SetPlaceFactor(1.25);
    boxWidget->PlaceWidget();
    vtkNew<widgetCallback> callback;
    boxWidget->AddObserver(vtkCommand::InteractionEvent, callback);
    boxWidget->On();
    
    // track the widget and actor
    coneActorList.push_back(coneActor);
    coneList.push_back(boxWidget);
}

vtkSmartPointer<vtkImageData> updateCTImage(vtkSmartPointer<vtkImageData> ctImage, QVector<vtkSmartPointer<vtkBoxWidget>>& coneList)
{
    vtkSmartPointer<vtkImageData> currentImage = ctImage;
    for (vtkBoxWidget* coneWidget : coneList) {
        vtkNew<vtkPolyData> pd;
        coneWidget->GetPolyData(pd);
        double pos[9][3];
        for (int i = 0; i < 8; i++) {
            pd->GetPoints()->GetPoint(i, pos[i]);
        }
        pd->GetPoints()->GetPoint(8, pos[8]);
        double height = std::sqrt(vtkMath::Distance2BetweenPoints(pos[0], pos[1]));
        double radius = 0.5 * std::sqrt(vtkMath::Distance2BetweenPoints(pos[1], pos[2]));
        vtkNew<vtkConeSource> coneSource;
        coneSource->SetRadius(radius);
        coneSource->SetHeight(height);
        coneSource->SetDirection(pos[1][0] - pos[0][0], pos[1][1] - pos[0][1], pos[1][2] - pos[0][2]);
        coneSource->SetResolution(500);
        coneSource->Update();

        vtkNew<vtkImageData> whiteImage;
        double spacing[3];
        ctImage->GetSpacing(spacing);
        whiteImage->SetSpacing(spacing);
        int extent[6];
        ctImage->GetExtent(extent);

        whiteImage->SetExtent(extent);
        whiteImage->SetOrigin(-pos[8][0], -pos[8][1], -pos[8][2]);
        whiteImage->AllocateScalars(VTK_SHORT, 1);

        // fill the image with foreground voxels:
        short inval = 20000;
        short outval = 0;
        vtkIdType count = whiteImage->GetNumberOfPoints();
        for (vtkIdType i = 0; i < count; ++i)
            whiteImage->GetPointData()->GetScalars()->SetTuple1(i, inval);

        // polygonal data --> image stencil:
        vtkNew<vtkPolyDataToImageStencil> pol2stenc;
        pol2stenc->SetInputConnection(coneSource->GetOutputPort());
        pol2stenc->SetOutputOrigin(-pos[8][0], -pos[8][1], -pos[8][2]);
        pol2stenc->SetOutputSpacing(spacing);
        pol2stenc->SetOutputWholeExtent(extent);
        pol2stenc->Update();

        // cut the corresponding white image and set the background:
        vtkNew<vtkImageStencil> imgstenc;
        imgstenc->SetInputData(whiteImage);
        imgstenc->SetStencilConnection(pol2stenc->GetOutputPort());
        imgstenc->ReverseStencilOff();
        imgstenc->SetBackgroundValue(outval);
        imgstenc->Update();

        vtkNew<vtkImageBlend> ctBlender;
        ctBlender->SetBlendMode(VTK_IMAGE_BLEND_MODE_NORMAL);
        ctBlender->AddInputData(currentImage);
        ctBlender->AddInputData(imgstenc->GetOutput());
        ctBlender->SetOpacity(0, 0.5);
        ctBlender->SetOpacity(1, 0.5);
        ctBlender->Update();
        currentImage = ctBlender->GetOutput();
    }
    return currentImage;
}

void updateRender2D(vtkImageReslice * ctReslice, vtkRenderWindow * renWin, vtkImageData* ctImage)
{
    ctReslice->SetInputData(0, ctImage);
    ctReslice->Modified();
    renWin->Render();
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

vtkSmartPointer<vtkRenderer> createRender2D(vtkImageReslice * ctReslice, vtkRenderWindow* renWin)
{
    vtkNew<vtkRenderer> ren;
    vtkNew<vtkLookupTable> lookupTable;
    lookupTable->SetRange(0, 255);
    lookupTable->SetValueRange(0.0, 1.0);
    lookupTable->SetSaturationRange(0.0, 0.0);
    lookupTable->SetRampToLinear();
    lookupTable->Build();

    vtkNew<vtkImageMapToColors> mapToColors;
    mapToColors->SetLookupTable(lookupTable);
    mapToColors->SetInputConnection(ctReslice->GetOutputPort());
    mapToColors->Update();

    vtkNew<vtkImageActor> imageActor;
    imageActor->GetMapper()->SetInputConnection(mapToColors->GetOutputPort());
    ren->AddActor(imageActor);
    ren->ResetCamera();

    renWin->AddRenderer(ren);
    vtkNew<vtkRenderWindowInteractor> interactor;
    interactor->SetRenderWindow(renWin);
    vtkNew<vtkInteractorStyleImage> imageStyle;
    interactor->SetInteractorStyle(imageStyle);

    vtkNew<ctResliceCallback> callback;
    callback->SetImageReslice(ctReslice);
    callback->SetMapToColors(mapToColors);
    callback->SetInteractor(interactor);
    callback->SetRenderWindow(renWin);

    imageStyle->AddObserver(vtkCommand::RightButtonPressEvent, callback);
    imageStyle->AddObserver(vtkCommand::RightButtonReleaseEvent, callback);
    imageStyle->AddObserver(vtkCommand::MouseMoveEvent, callback);
    return ren;
}

vtkSmartPointer<vtkImageData> readCT(const char * filePath)
{
    // read in DICOM file
    vtkNew<vtkDICOMImageReader> reader;
    reader->SetDirectoryName(filePath);
    reader->SetDataSpacing(3.2, 3.2, 1.5);
    reader->SetDataByteOrderToLittleEndian();
    reader->Update();

    // convert scalar type
    vtkNew<vtkImageCast> imageCast;
    imageCast->SetInputConnection(reader->GetOutputPort());
    imageCast->SetOutputScalarTypeToShort();
    imageCast->Update();
    return imageCast->GetOutput();
}