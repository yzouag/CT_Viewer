#include "planting_screws.h"
#include <vtkNamedColors.h>
#include <vtkConeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkCommand.h>
#include <vtkTransform.h>
#include <vtkPolyDataReader.h>

PlantingScrews::PlantingScrews(const char* screwName)
{
    strcpy(this->screwName, screwName);
    
    if (strncmp("cone", screwName, 5) == 0) {
        addCone();
    }
    else {
        addCustomScrew(screwName);
    }
}

PlantingScrews::~PlantingScrews()
{
    this->screwActor = nullptr;
    this->screwWidget = nullptr; // the potential of memory leak!!! Need to check later!
}

vtkSmartPointer<vtkBoxWidget> PlantingScrews::getScrewWidget()
{
    return this->screwWidget;
}

vtkSmartPointer<vtkActor> PlantingScrews::getScrewActor()
{
    return this->screwActor;
}

const char * PlantingScrews::getScrewName()
{
    return this->screwName;
}

// add cone actor which can change shape and size    
void PlantingScrews::addCone()
{
    vtkNew<vtkNamedColors> colors;

    // create the cone source
    vtkNew<vtkConeSource> cone;
    cone->SetHeight(40.0);
    cone->SetRadius(10.0);
    cone->SetResolution(100);
    cone->SetDirection(0, -1, 0);

    // add source to mapper and then create actor
    vtkNew<vtkPolyDataMapper> coneMapper;
    coneMapper->SetInputConnection(cone->GetOutputPort());
    vtkNew<vtkActor> coneActor;
    coneActor->SetMapper(coneMapper);
    coneActor->GetProperty()->SetColor(colors->GetColor3d("DarkTurquoise").GetData());
    coneActor->GetProperty()->SetDiffuse(0.7);
    coneActor->GetProperty()->SetSpecular(0.4);
    coneActor->GetProperty()->SetSpecularPower(20);
    this->screwActor = coneActor;

    // create box widget and attach to coneActor
    this->screwWidget = createBoxWidget();
}

void PlantingScrews::addCustomScrew(const char * screwName)
{
    // read the .vtk models in /screwModels
    vtkNew<vtkPolyDataReader> reader;
    std::string screwPath("./screwModels/");
    screwPath.append(screwName);
    screwPath.append(".vtk");
    reader->SetFileName(screwPath.c_str());
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(reader->GetOutputPort());
    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    this->screwActor = actor;

    // create interactive widgets, disable the scaling (screw size should be fixed)
    this->screwWidget = createBoxWidget();
    this->screwWidget->HandlesOff();
    this->screwWidget->ScalingEnabledOff();
}

vtkSmartPointer<vtkBoxWidget> PlantingScrews::createBoxWidget()
{
    vtkNew<vtkBoxWidget> boxWidget;
    boxWidget->SetProp3D(this->screwActor);
    boxWidget->SetPlaceFactor(1.25);
    boxWidget->SetHandleSize(0.001);
    boxWidget->PlaceWidget();
    return boxWidget;
}
