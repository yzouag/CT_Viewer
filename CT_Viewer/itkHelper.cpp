#include "itkHelper.h"
#include "itkImageSeriesReader.h"
#include "itkGDCMImageIO.h"
#include "itkGDCMSeriesFileNames.h"

QMap<QString, QString> getMetaInfoFromCTFile(const char * path)
{
    QMap<QString, QString> map;
    
    using PixelType = signed short;
    constexpr unsigned int Dimension = 3;
    using ImageType = itk::Image<PixelType, Dimension>;

    using ReaderType = itk::ImageSeriesReader<ImageType>;
    auto reader = ReaderType::New();
    using ImageIOType = itk::GDCMImageIO;
    auto dicomIO = ImageIOType::New();
    reader->SetImageIO(dicomIO);

    using NamesGeneratorType = itk::GDCMSeriesFileNames;
    auto nameGenerator = NamesGeneratorType::New();
    nameGenerator->SetInputDirectory(path);
    using FileNamesContainer = std::vector<std::string>;
    FileNamesContainer fileNames = nameGenerator->GetInputFileNames();
    reader->SetFileNames(fileNames);

    try {
        reader->Update();
    }
    catch (const itk::ExceptionObject & ex) {
        std::cout << ex << std::endl;
        return map;
    }
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
                map[QString::fromStdString(entryName[i])] = QString::fromStdString(tagvalue);
            }
        }
    }
    return map;
}