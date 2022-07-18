// these initializations are needed for compiling project using Visual Studio but not using cmake
#include <vtkAutoInit.h> 
VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingVolumeOpenGL2);
VTK_MODULE_INIT(vtkRenderingFreeType);

// start of the header files
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
#include <vtkPolyDataReader.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkCursor2D.h>
#include <vtkCoordinate.h>
#include <vtkTextProperty.h>
#include <QDebug>

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

vtkSmartPointer<vtkBoxWidget> createBoxWidget(vtkSmartPointer<vtkActor> actor, vtkRenderWindowInteractor* interactor)
{
    vtkNew<vtkBoxWidget> boxWidget;
    boxWidget->SetInteractor(interactor);
    boxWidget->SetProp3D(actor);
    boxWidget->SetPlaceFactor(1.25);
    boxWidget->PlaceWidget();
    vtkNew<widgetCallback> callback;
    boxWidget->AddObserver(vtkCommand::InteractionEvent, callback);
    boxWidget->On();
    return boxWidget;
}

void addCone(QVector<QPair<const char*, vtkSmartPointer<vtkBoxWidget>>>& coneList, QVector<vtkSmartPointer<vtkActor>>& coneActorList,vtkRenderer* ren, vtkRenderWindowInteractor* interactor)
{
    vtkNew<vtkNamedColors> colors;
    
    // create the cone source
    vtkNew<vtkConeSource> cone;
    cone->SetHeight(40.0);
    cone->SetRadius(10.0);
    cone->SetResolution(100);

    // add source to mapper and then create actor
    vtkNew<vtkPolyDataMapper> coneMapper;
    coneMapper->SetInputConnection(cone->GetOutputPort());
    vtkNew<vtkActor> coneActor;
    coneActor->SetMapper(coneMapper);
    coneActor->GetProperty()->SetColor(colors->GetColor3d("DarkTurquoise").GetData());
    coneActor->GetProperty()->SetDiffuse(0.7);
    coneActor->GetProperty()->SetSpecular(0.4);
    coneActor->GetProperty()->SetSpecularPower(20);
    ren->AddActor(coneActor);

    // create box widget and attach to coneActor
    vtkSmartPointer<vtkBoxWidget> boxWidget = createBoxWidget(coneActor, interactor);
    
    // track the widget and actor
    coneActorList.push_back(coneActor);
    coneList.push_back(QPair<const char*, vtkSmartPointer<vtkBoxWidget>>{"cone", boxWidget});
}

void addCustomScrew(const char * path, QVector<QPair<const char*, vtkSmartPointer<vtkBoxWidget>>>& coneList, QVector<vtkSmartPointer<vtkActor>>& coneActorList, vtkRenderer * ren, vtkRenderWindowInteractor * interactor)
{
    // read the .vtk models in /screwModels
    vtkNew<vtkPolyDataReader> reader;
    reader->SetFileName(path);
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(reader->GetOutputPort());
    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    ren->AddActor(actor);

    // create interactive widgets, disable the scaling (screw size should be fixed)
    vtkSmartPointer<vtkBoxWidget> boxWidget = createBoxWidget(actor, interactor);
    boxWidget->HandlesOff();
    boxWidget->ScalingEnabledOff();
    
    coneActorList.push_back(actor);
    coneList.push_back(QPair<const char*, vtkSmartPointer<vtkBoxWidget>>{path, boxWidget});
}

void addScrew(int model, QVector<QPair<const char*, vtkSmartPointer<vtkBoxWidget>>>& coneList, QVector<vtkSmartPointer<vtkActor>>& coneActorList, vtkRenderer * ren, vtkRenderWindowInteractor * interactor)
{
    switch (model) {
    case 0:
        addCone(coneList, coneActorList, ren, interactor);
        break;
    case 1:
        addCustomScrew("./screwModels/scaled_475x30.vtk", coneList, coneActorList, ren, interactor);
        break;
    case 2:
        addCustomScrew("./screwModels/scaled_700x50.vtk", coneList, coneActorList, ren, interactor);
        break;
    }
}

vtkSmartPointer<vtkImageData> updateCTImage(vtkSmartPointer<vtkImageData> ctImage, QVector<QPair<const char*, vtkSmartPointer<vtkBoxWidget>>>& coneList)
{
    vtkSmartPointer<vtkImageData> currentImage = ctImage;

    for (auto coneWidget : coneList) {
        // get the geometry information of the bounding box widget
        vtkNew<vtkPolyData> pd;
        coneWidget.second->GetPolyData(pd);
        double pos[9][3];
        for (int i = 0; i < 8; i++) {
            pd->GetPoints()->GetPoint(i, pos[i]);
        }
        pd->GetPoints()->GetPoint(14, pos[8]);

        // create an empty image and set the size to be the same as the CT image
        vtkNew<vtkImageData> whiteImage;
        double spacing[3];
        ctImage->GetSpacing(spacing);
        whiteImage->SetSpacing(spacing);
        int extent[6];

        ctImage->GetExtent(extent);
        whiteImage->SetExtent(extent);
        whiteImage->SetOrigin(-pos[8][0], -pos[8][1], -pos[8][2]); // why there must be a minus sign????
        whiteImage->AllocateScalars(VTK_SHORT, 1);

        // fill the image with foreground voxels:
        short inval = 20000;
        short outval = 0;
        vtkIdType count = whiteImage->GetNumberOfPoints();
        for (vtkIdType i = 0; i < count; ++i)
            whiteImage->GetPointData()->GetScalars()->SetTuple1(i, inval);
        
        // create a polygonal data to image stencil filter
        vtkNew<vtkPolyDataToImageStencil> pol2stenc;
        pol2stenc->SetOutputOrigin(-pos[8][0], -pos[8][1], -pos[8][2]);
        pol2stenc->SetOutputSpacing(spacing);
        pol2stenc->SetOutputWholeExtent(extent);
        
        // create the image source
        if (strncmp(coneWidget.first, "cone", 5) == 0) {
            double height = std::sqrt(vtkMath::Distance2BetweenPoints(pos[0], pos[1]));
            double radius = 0.5 * std::sqrt(vtkMath::Distance2BetweenPoints(pos[1], pos[2]));
            vtkNew<vtkConeSource> coneSource;
            coneSource->SetRadius(radius);
            coneSource->SetHeight(height);
            coneSource->SetDirection(pos[1][0] - pos[0][0], pos[1][1] - pos[0][1], pos[1][2] - pos[0][2]);
            coneSource->SetResolution(500);
            coneSource->Update();
            // polygonal data --> image stencil:
            pol2stenc->SetInputConnection(coneSource->GetOutputPort());
        }
        else {
            vtkNew<vtkPolyDataReader> reader;
            reader->SetFileName(coneWidget.first);
            
            // get the orientation of the screw added
            double orientation[3];
            vtkNew<vtkTransform> targetTransform;
            coneWidget.second->GetTransform(targetTransform);
            targetTransform->GetOrientation(orientation);
            
            // add the orientation to the polygonal data
            vtkNew<vtkTransform> t;
            t->RotateZ(orientation[2]);
            t->RotateX(orientation[0]);
            t->RotateY(orientation[1]);
            vtkNew<vtkTransformPolyDataFilter> transformFilter;
            transformFilter->SetTransform(t);
            transformFilter->SetInputConnection(reader->GetOutputPort());

            // polygonal data --> image stencil
            pol2stenc->SetInputConnection(transformFilter->GetOutputPort());
        }
        
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

void setHeader(vtkRenderer * ren, int axis)
{
    vtkNew<vtkNamedColors> colors;
    vtkNew<vtkCornerAnnotation> cornerAnnotation;
    cornerAnnotation->GetTextProperty()->SetFontFamilyToArial();
    cornerAnnotation->GetTextProperty()->BoldOn();
    cornerAnnotation->GetTextProperty()->SetFontSize(20);
    cornerAnnotation->GetTextProperty()->SetColor(colors->GetColor3d("Azure").GetData());
    cornerAnnotation->SetLinearFontScaleFactor(2);
    cornerAnnotation->SetNonlinearFontScaleFactor(1);
    cornerAnnotation->SetMaximumFontSize(20);
    switch (axis) {
    case 0:
        cornerAnnotation->SetText(2, "Sagittal");
        break;
    case 1:
        cornerAnnotation->SetText(2, "Coronal");
        break;
    case 2:
        cornerAnnotation->SetText(2, "Axial");
        break;
    case 3:
        cornerAnnotation->SetText(2, "3D View");
        break;
    }
    ren->AddViewProp(cornerAnnotation);
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
            int pos[2];
            this->interactor->GetEventPosition(pos);
            vtkNew<vtkCoordinate> coordinateSystem;
            coordinateSystem->SetCoordinateSystemToDisplay();
            coordinateSystem->SetValue(1.0*pos[0], 1.0*pos[1]);
            double* worldPos = coordinateSystem->GetComputedWorldValue(this->ren);
            this->cursor->SetFocalPoint(worldPos);
            this->cursor->Modified();
            this->interactor->Render();
        } else {
            if (!isToggled) {
                return;
            }
            int pos[2];
            this->interactor->GetEventPosition(pos);
            vtkNew<vtkCoordinate> coordinateSystem;
            coordinateSystem->SetCoordinateSystemToDisplay();
            coordinateSystem->SetValue(1.0*pos[0], 1.0*pos[1]);
            double* worldPos = coordinateSystem->GetComputedWorldValue(this->ren);
            this->cursor->SetFocalPoint(worldPos);
            this->cursor->Modified();
            this->interactor->Render();
        }
    }

private:
    vtkSmartPointer<vtkCursor2D> cursor;
    vtkSmartPointer<vtkRenderWindowInteractor> interactor;
    vtkSmartPointer<vtkRenderer> ren;
    bool isToggled = false;
};

vtkSmartPointer<vtkRenderer> createRender2D(vtkImageReslice * ctReslice, vtkRenderWindow* renWin)
{
    // define look up table and create the image actor
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
    
    // create 2D cursor cross lines
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
    double pos[3];
    imageActor->GetPosition(pos);
    cursorActor->SetPosition(pos[0], pos[1], pos[2] + 1);
    
    // create render and add actors to the render
    vtkNew<vtkRenderer> ren;
    ren->AddActor(imageActor);
    ren->AddActor(cursorActor);
    ren->ResetCamera();

    // add render to the render window, create interactions
    renWin->AddRenderer(ren);
    vtkNew<vtkRenderWindowInteractor> interactor;
    interactor->SetRenderWindow(renWin);
    vtkNew<vtkInteractorStyleImage> imageStyle;
    interactor->SetInteractorStyle(imageStyle);

    // define logics for reslice the model
    vtkNew<ctResliceCallback> callback;
    callback->SetImageReslice(ctReslice);
    callback->SetMapToColors(mapToColors);
    callback->SetInteractor(interactor);
    callback->SetRenderWindow(renWin);

    imageStyle->AddObserver(vtkCommand::RightButtonPressEvent, callback);
    imageStyle->AddObserver(vtkCommand::RightButtonReleaseEvent, callback);
    imageStyle->AddObserver(vtkCommand::MouseMoveEvent, callback);

    vtkNew<vtkImageInteractionCallback> cursorCallback;
    cursorCallback->setCursor(cursor);
    cursorCallback->setInteractor(interactor);
    cursorCallback->setRender(ren);

    imageStyle->AddObserver(vtkCommand::LeftButtonPressEvent, cursorCallback);
    imageStyle->AddObserver(vtkCommand::LeftButtonReleaseEvent, cursorCallback);
    imageStyle->AddObserver(vtkCommand::MouseMoveEvent, cursorCallback);
    return ren;
}

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
        this->dialog->setValue((int)(*progress*100));
    }
private:
    QProgressDialog* dialog;
};

vtkSmartPointer<vtkImageData> readCT(const char * filePath, QProgressDialog* dialog)
{
    // read in DICOM file
    vtkNew<vtkDICOMImageReader> reader;
    reader->SetDirectoryName(filePath);
    reader->SetDataSpacing(3.2, 3.2, 1.5);
    reader->SetDataByteOrderToLittleEndian();

    vtkNew<ReadCTProgressUpdate> callback;
    callback->setDialog(dialog);
    reader->AddObserver(vtkCommand::ProgressEvent, callback);
    reader->Update();

    // convert scalar type
    vtkNew<vtkImageCast> imageCast;
    imageCast->SetInputConnection(reader->GetOutputPort());
    imageCast->SetOutputScalarTypeToShort();
    imageCast->Update();
    return imageCast->GetOutput();
}