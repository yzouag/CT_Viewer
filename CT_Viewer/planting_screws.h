#pragma once
#include <vtkPolyData.h>
#include <vtkActor.h>
#include <vtkBoxWidget.h>
#include <vtkSmartPointer.h>

class PlantingScrews
{
public:
    PlantingScrews(const char* screwName);
    ~PlantingScrews();
    vtkSmartPointer<vtkBoxWidget> getScrewWidget();
    vtkSmartPointer<vtkActor> getScrewActor();
    const char* getScrewName();

private:
    vtkSmartPointer<vtkActor> screwActor;
    vtkSmartPointer<vtkBoxWidget> screwWidget;
    char screwName[30];
    void addCone();
    void addCustomScrew(const char* screwName);
    vtkSmartPointer<vtkBoxWidget> createBoxWidget();
};

