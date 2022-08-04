#include "ct_image.h"
#include <vtkDICOMImageReader.h>
#include <vtkCommand.h>
#include <vtkImageCast.h>
#include <vtkMatrix4x4.h>
#include <itkImage.h>
#include <itkImageSeriesReader.h>
#include <itkGDCMSeriesFileNames.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkImageStencil.h>
#include <vtkConeSource.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkImageBlend.h>
#include <vtkPolyDataReader.h>
#include <vtkPolyDataToImageStencil.h>
#include <itkCommand.h>
#include <QDebug>

CT_Image::CT_Image()
{
}


CT_Image::~CT_Image()
{
    this->ctImage = nullptr;
    for (int i = 0; i < 3; i++) {
        this->ctReslices[i] = nullptr;
    }
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
        this->dialog->setValue((int)(*progress * 100));
    }
private:
    QProgressDialog* dialog;
};

void CT_Image::loadDicomFromDirectory(QString path, QProgressDialog * dialog)
{
    this->load_succeed = false;
    // load meta data
    this->path = path;
    loadMetaInfo(dialog);
    if (!this->load_succeed) {
        return;
    }

    // read in DICOM file
    vtkNew<vtkDICOMImageReader> reader;
    reader->SetDirectoryName(path.toStdString().c_str());
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
    this->ctImage = imageCast->GetOutput();
    this->originImage = imageCast->GetOutput();

    // create three 2D views
    for (int i = 0; i < 3; i++) {
        createReslices(i);
    }
}

vtkSmartPointer<vtkImageData> CT_Image::getCTImageData()
{
    return this->ctImage;
}

vtkSmartPointer<vtkImageReslice> CT_Image::getCTImageReslice(int axis)
{
    return this->ctReslices[axis];
}

vtkSmartPointer<vtkImageAccumulate> CT_Image::getCTImageAccumulate()
{
    int n_bins = 50;
    int min_val = -3100; // TODO: how to get this two number?
    int max_val = 3100;

    vtkNew<vtkImageAccumulate> imageAccumulate;
    imageAccumulate->SetInputData(this->originImage);
    imageAccumulate->SetComponentExtent(0, n_bins-1, 0, 0, 0, 0);
    imageAccumulate->SetComponentOrigin(min_val, 0, 0);
    imageAccumulate->SetComponentSpacing((max_val - min_val) / n_bins, 0, 0);
    imageAccumulate->Update();
    return imageAccumulate;
}

QMap<QString, QString> CT_Image::getMetaInfo()
{
    return this->metaInfo;
}

bool CT_Image::checkLoadSuccess()
{
    return this->load_succeed;
}

class CommandProgressUpdate : public itk::Command
{
public:
    typedef  CommandProgressUpdate   Self;
    typedef  itk::Command            Superclass;
    typedef  itk::SmartPointer<Self> Pointer;
    itkNewMacro(Self);
protected:
    CommandProgressUpdate()
    {
    };
public:
    void Execute(itk::Object *caller, const itk::EventObject & event)
    {
        Execute((const itk::Object *)caller, event);
    }
    void Execute(const itk::Object * object, const itk::EventObject & event)
    {
        const itk::ProcessObject * filter = dynamic_cast<const itk::ProcessObject *>(object);
        if (!itk::ProgressEvent().CheckEvent(&event)) {
            return;
        }
        this->dialog->setValue(static_cast<int>(filter->GetProgress() * 100));
    }
    void setProgressDialog(QProgressDialog* dialog)
    {
        this->dialog = dialog;
    }
private:
    QProgressDialog* dialog;
};

void CT_Image::loadMetaInfo(QProgressDialog* dialog)
{
    using PixelType = signed short;
    constexpr unsigned int Dimension = 3;
    using ImageType = itk::Image<PixelType, Dimension>;
    using ImageIOType = itk::GDCMImageIO;
    auto dicomIO = ImageIOType::New();

    // define file names loader
    using NamesGeneratorType = itk::GDCMSeriesFileNames;
    auto nameGenerator = NamesGeneratorType::New();
    nameGenerator->SetInputDirectory(this->path.toStdString().c_str());
    nameGenerator->SetGlobalWarningDisplay(false);
    using FileNamesContainer = std::vector<std::string>;
    FileNamesContainer fileNames = nameGenerator->GetInputFileNames();

    // check if the directory has Dicom files
    try {
        using SeriesIdContainer = std::vector<std::string>;
        const SeriesIdContainer& seriesUID = nameGenerator->GetSeriesUIDs();
        auto seriesItr = seriesUID.begin();
        auto seriesEnd = seriesUID.end();
        if (seriesItr == seriesEnd) {
            std::cout << "No Dicom Files Found!" << endl;
            return;
        }
    }
    catch (const itk::ExceptionObject & ex) {
        std::cout << ex << std::endl;
        return;
    }

    // create a reader for inputs
    using ReaderType = itk::ImageSeriesReader<ImageType>;
    auto reader = ReaderType::New();
    reader->SetImageIO(dicomIO);
    reader->SetFileNames(fileNames);
    reader->ForceOrthogonalDirectionOff();
    CommandProgressUpdate::Pointer observer = CommandProgressUpdate::New();
    observer->setProgressDialog(dialog);
    reader->AddObserver(itk::ProgressEvent(), observer);
    try {
        reader->Update();
    }
    catch (const itk::ExceptionObject & ex) {
        std::cout << ex << std::endl;
        return;
    }
    createMetaInfo(dicomIO);
    this->load_succeed = true;
}

void CT_Image::createReslices(int axis)
{
    int extent[6];
    double spacing[3];
    double origin[3];
    double center[3];

    this->ctImage->GetExtent(extent);
    this->ctImage->GetSpacing(spacing);
    this->ctImage->GetOrigin(origin);

    // the center position of the 3D model
    center[0] = origin[0] + spacing[0] * 0.5 * (extent[0] + extent[1]);
    center[1] = origin[1] + spacing[1] * 0.5 * (extent[2] + extent[3]);
    center[2] = origin[2] + spacing[2] * 0.5 * (extent[4] + extent[5]);

    vtkNew<vtkMatrix4x4> resliceAxes;
    resliceAxes->DeepCopy(VIEWDIRECTIONMATRIX[axis]);
    resliceAxes->SetElement(0, 3, center[0]);
    resliceAxes->SetElement(1, 3, center[1]);
    resliceAxes->SetElement(2, 3, center[2]);

    vtkNew<vtkImageReslice> imageReslice;
    imageReslice->SetInputData(this->ctImage);
    imageReslice->SetOutputDimensionality(2);
    imageReslice->SetResliceAxes(resliceAxes);
    imageReslice->SetInterpolationModeToLinear();
    this->ctReslices[axis] = imageReslice;
}

void CT_Image::createMetaInfo(itk::SmartPointer<itk::GDCMImageIO> dicomIO)
{
    QMap<QString, QString> metaInfo;
    using DictionaryType = itk::MetaDataDictionary;
    const DictionaryType & dictionary = dicomIO->GetMetaDataDictionary();
    using MetaDataStringType = itk::MetaDataObject<std::string>;

    auto itr = dictionary.Begin();
    auto end = dictionary.End();

    std::string entryId[19] = {"0010|0010", "0008|0012", "0008|0032", "0008|0060", "0008|0080",
        "0008|0070", "0008|1030", "0008|103e", "0008|1050", "0008|1060",
        "0008|1070", "0008|1090", "0010|0020", "0010|0030", "0010|0040",
        "0010|1010", "0018|0050", "0018|0060", "0018|0088"};

    std::string entryName[19] = {"Patient Name", "Instance Creation Date", "Acquisition Time", "Modality", "Institution Name",
        "Manufacturer", "Study Description", "Series Description", "Performing Physician's Name", "Name of Physician(s) Reading Study",
        "Operators' Name", "Manufacturer's Model Name", "Patient ID", "Patient's Birth Date", "Patient's Sex",
        "Patient's Age", "Slice Thickness", "KVP", "Spacing Between Slices"};

    for (int i = 0; i < 19; i++) {
        auto tagItr = dictionary.Find(entryId[i]);
        if (tagItr != end) {
            MetaDataStringType::ConstPointer entryvalue = dynamic_cast<const MetaDataStringType *>(tagItr->second.GetPointer());
            if (entryvalue) {
                std::string tagvalue = entryvalue->GetMetaDataObjectValue();
                metaInfo[QString::fromStdString(entryName[i])] = QString::fromStdString(tagvalue);
            }
        }
    }
    this->metaInfo = metaInfo;
}

void CT_Image::updateImage(QVector<PlantingScrews*> screwList)
{
    vtkSmartPointer<vtkImageData> currentImage = this->ctImage;
    for (auto screw : screwList) {
        // get the geometry information of the bounding box widget
        vtkNew<vtkPolyData> pd;
        screw->getScrewWidget()->GetPolyData(pd);
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
        if (strncmp(screw->getScrewName(), "cone", 5) == 0) {
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
        } else {
            vtkNew<vtkPolyDataReader> reader;
            std::string screwPath("./screwModels/");
            screwPath.append(screw->getScrewName());
            screwPath.append(".vtk");
            reader->SetFileName(screwPath.c_str());

            // get the orientation of the screw added
            double orientation[3];
            vtkNew<vtkTransform> targetTransform;
            screw->getScrewWidget()->GetTransform(targetTransform);
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
        ctBlender->SetOpacity(0, 1.0);
        ctBlender->SetOpacity(1, 0.5);
        ctBlender->Update();
        currentImage = ctBlender->GetOutput();
    }
    this->ctImage = currentImage;
}

void CT_Image::resetImage()
{
    this->ctImage->DeepCopy(this->originImage);
}

QString CT_Image::getFilePath()
{
    return this->path;
}
