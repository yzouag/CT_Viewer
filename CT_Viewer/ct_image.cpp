#include "ct_image.h"
#include <vtkDICOMImageReader.h>
#include <vtkCommand.h>
#include <vtkImageCast.h>
#include <vtkMatrix4x4.h>
#include <itkImage.h>
#include <itkImageSeriesReader.h>
#include <itkGDCMSeriesFileNames.h>
#include <itkImageToVTKImageFilter.h>
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
#include <vtkImageFlip.h>
#include <itkMinimumMaximumImageCalculator.h>
#include <itkVTKImageToImageFilter.h>
#include <itkImageSeriesWriter.h>
#include <itkNumericSeriesFileNames.h>
#include "uiHelper.h"
#include <vtkPolyDataMapper.h>

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

void CT_Image::loadDicomFromDirectory(QString path, QProgressDialog * dialog)
{
    // load meta data
    this->path = path;
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

    // check if the directory has DICOM files
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
        // if no DICOM file is found
        // terminate the read process and set the load_succeed to false
        this->load_succeed = false;
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

    using ImageCalculatorFilterType = itk::MinimumMaximumImageCalculator<ImageType>;
    ImageCalculatorFilterType::Pointer imageCalculatorFilter = ImageCalculatorFilterType::New();
    imageCalculatorFilter->SetImage(reader->GetOutput());
    imageCalculatorFilter->Compute();
    this->min_pixel_val = imageCalculatorFilter->GetMinimum();
    this->max_pixel_val = imageCalculatorFilter->GetMaximum();

    // create the dictionary of DICOM meta tags
    createMetaInfo(dicomIO);

    // convert itk data to vtk data
    using FilterType = itk::ImageToVTKImageFilter<ImageType>;
    auto filter = FilterType::New();
    filter->SetInput(reader->GetOutput());
    try {
        filter->Update();
    }
    catch (const itk::ExceptionObject & error) {
        std::cerr << "Error: " << error << std::endl;
        this->load_succeed = false;
        return;
    }
    
    // flip the image since ITK and VTK has different coordinate system
    vtkNew<vtkImageFlip> imageFlip;
    imageFlip->SetInputData(filter->GetOutput());
    imageFlip->SetFilteredAxes(1);
    imageFlip->Update();

    vtkNew<vtkImageFlip> imageFlip1;
    imageFlip1->SetInputConnection(imageFlip->GetOutputPort());
    imageFlip1->SetFilteredAxes(0);
    imageFlip1->Update();

    // convert scalar type
    vtkNew<vtkImageCast> imageCast;
    imageCast->SetInputConnection(imageFlip1->GetOutputPort());
    imageCast->SetOutputScalarTypeToShort();
    imageCast->Update();

    // why we need two?
    this->ctImage = imageCast->GetOutput();

    // initialize slice center and model center
    int extent[6];
    double spacing[3];
    double origin[3];

    this->ctImage->GetExtent(extent);
    this->ctImage->GetSpacing(spacing);
    this->ctImage->GetOrigin(origin);
    this->ctImage->GetBounds(this->modelBounds);

    // the center position of the 3D model
    this->modelCenter[0] = origin[0] + spacing[0] * 0.5 * (extent[0] + extent[1]);
    this->modelCenter[1] = origin[1] + spacing[1] * 0.5 * (extent[2] + extent[3]);
    this->modelCenter[2] = origin[2] + spacing[2] * 0.5 * (extent[4] + extent[5]);

    this->sliceCenter[0] = this->modelCenter[0];
    this->sliceCenter[1] = this->modelCenter[1];
    this->sliceCenter[2] = this->modelCenter[2];

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
    int min_val = this->min_pixel_val;
    int max_val = this->max_pixel_val;

    vtkNew<vtkImageAccumulate> imageAccumulate;
    imageAccumulate->SetInputData(this->ctImage);
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

void CT_Image::createReslices(int axis)
{
    vtkNew<vtkMatrix4x4> resliceAxes;
    resliceAxes->DeepCopy(VIEWDIRECTIONMATRIX[axis]);
    resliceAxes->SetElement(0, 3, this->modelCenter[0]);
    resliceAxes->SetElement(1, 3, this->modelCenter[1]);
    resliceAxes->SetElement(2, 3, this->modelCenter[2]);

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

void CT_Image::updateImage(QVector<PlantingScrews*> screwList, QProgressDialog* dialog)
{
    // set the progress dialog
    dialog->setValue(0);
    dialog->setLabelText("Generating New Image...");
    
    // append each screw data one by one
    vtkSmartPointer<vtkImageData> currentImage = this->ctImage;
    int current_progress = 0;
    for (auto screw : screwList) {
        // create an empty image and set the size to be the same as the CT image
        vtkNew<vtkImageData> whiteImage;
        double spacing[3];
        ctImage->GetSpacing(spacing);
        whiteImage->SetSpacing(spacing);
        
        int extent[6];
        ctImage->GetExtent(extent);
        whiteImage->SetExtent(extent);

        double origin[3];
        this->ctImage->GetOrigin(origin);
        whiteImage->SetOrigin(origin);
        whiteImage->AllocateScalars(VTK_SHORT, 1);

        // fill the image with foreground voxels:
        short inval = 20000;
        short outval = 0;
        vtkIdType count = whiteImage->GetNumberOfPoints();
        for (vtkIdType i = 0; i < count; ++i)
            whiteImage->GetPointData()->GetScalars()->SetTuple1(i, inval);

        // create a polygonal data to image stencil filter
        vtkNew<vtkPolyDataToImageStencil> pol2stenc;
        pol2stenc->SetOutputSpacing(spacing);
        pol2stenc->SetOutputWholeExtent(extent);
        pol2stenc->SetOutputOrigin(this->ctImage->GetOrigin());

        // get screw data
        vtkPolyDataMapper* mapper = static_cast<vtkPolyDataMapper*>(screw->getScrewActor()->GetMapper());
        vtkPolyData* screw_data = mapper->GetInput();
        
        pol2stenc->SetInputData(screw_data);
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

        current_progress += 100 / screwList.size();
        dialog->setValue(current_progress);
    }
    this->ctImage = currentImage;
}

QString CT_Image::getFilePath()
{
    return this->path;
}

double * CT_Image::getModelCenter()
{
    return this->modelCenter;
}

double * CT_Image::getSliceCenter()
{
    return this->sliceCenter;
}

double * CT_Image::getModelBounds()
{
    return this->modelBounds;
}

int * CT_Image::getContrastThreshold()
{
    return this->contrastThreshold;
}

void CT_Image::saveImageData(QString diretoryPath, QProgressDialog* dialog)
{
    dialog->setValue(0);
    dialog->setLabelText("Exporting Image...");

    using PixelType = signed short;
    constexpr unsigned int Dimension = 3;
    using ImageType = itk::Image<PixelType, Dimension>;
    
    // convert VTK image to ITK image
    // before conversion, flip the image
    vtkNew<vtkImageFlip> imageFlip;
    imageFlip->SetInputData(this->ctImage);
    imageFlip->SetFilteredAxes(0);
    imageFlip->Update();

    vtkNew<vtkImageFlip> imageFlip1;
    imageFlip1->SetInputConnection(imageFlip->GetOutputPort());
    imageFlip1->SetFilteredAxes(1);
    imageFlip1->Update();

    // convert to itk image
    using FilterType = itk::VTKImageToImageFilter<ImageType>;
    auto filter = FilterType::New();
    filter->SetInput(imageFlip1->GetOutput());
    try {
        filter->Update();
    }
    catch (const itk::ExceptionObject & error) {
        std::cerr << "Error: " << error << std::endl;
        return;
    }

    using ImageIOType = itk::GDCMImageIO;
    auto dicomIO = ImageIOType::New();
    constexpr unsigned int InputDimension = 3;
    constexpr unsigned int OutputDimension = 2;
    using InputImageType = itk::Image<PixelType, InputDimension>;
    using OutputImageType = itk::Image<PixelType, OutputDimension>;
    using SeriesWriterType = itk::ImageSeriesWriter<InputImageType, OutputImageType>;
    using NamesGeneratorType = itk::GDCMSeriesFileNames;
    using ReaderType = itk::ImageSeriesReader<ImageType>;
    auto nameGenerator = NamesGeneratorType::New();
    nameGenerator->SetInputDirectory(this->path.toStdString().c_str());
    auto reader = ReaderType::New();
    reader->SetImageIO(dicomIO);
    using FileNamesContainer = std::vector<std::string>;
    FileNamesContainer fileNames = nameGenerator->GetInputFileNames();
    
    reader->SetFileNames(fileNames);
    reader->ForceOrthogonalDirectionOff();
    reader->Update();
    
    using OutputNamesGeneratorType = itk::NumericSeriesFileNames;
    auto outputNames = OutputNamesGeneratorType::New();
    std::string seriesFormat(diretoryPath.toStdString());
    seriesFormat = seriesFormat + "/" + "IM%d.dcm";
    outputNames->SetSeriesFormat(seriesFormat.c_str());
    outputNames->SetStartIndex(1);
    outputNames->SetEndIndex(fileNames.size());
    
    auto seriesWriter = SeriesWriterType::New();
    seriesWriter->SetInput(filter->GetOutput());
    seriesWriter->SetImageIO(dicomIO);
    seriesWriter->SetFileNames(outputNames->GetFileNames());
    seriesWriter->SetMetaDataDictionaryArray(reader->GetMetaDataDictionaryArray());
    CommandProgressUpdate::Pointer observer = CommandProgressUpdate::New();
    observer->setProgressDialog(dialog);
    seriesWriter->AddObserver(itk::ProgressEvent(), observer);
    try {
        seriesWriter->Update();
    }
    catch (const itk::ExceptionObject & excp) {
        std::cerr << "Exception thrown while writing the series " << std::endl;
        std::cerr << excp << std::endl;
    }
}

void CT_Image::updateSliceCenter(double x, double y, double z)
{
    this->sliceCenter[0] = x;
    this->sliceCenter[1] = y;
    this->sliceCenter[2] = z;
    emit sliceCenterChange(x, y, z);
}

void CT_Image::updateContrastThreshold(int lower, int upper)
{
    this->contrastThreshold[0] = lower;
    this->contrastThreshold[1] = upper;
    emit contrastThresholdChange(lower, upper);
}
