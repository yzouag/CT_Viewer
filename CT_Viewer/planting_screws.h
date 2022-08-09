#pragma once
#include <vtkPolyData.h>
#include <vtkActor.h>
#include <vtkBoxWidget.h>
#include <vtkSmartPointer.h>

class CT_2d_Widget;
class CT_3d_Widget;

class PlantingScrews
{
public:
    PlantingScrews(const char* screwName);
    ~PlantingScrews();
    vtkSmartPointer<vtkBoxWidget> getScrewWidget();
    vtkSmartPointer<vtkActor> getScrewActor();
    vtkSmartPointer<vtkPolyData> getScrewSource();
    const char* getScrewName();
    void setMainViewWidget(CT_3d_Widget* mainViewWidget);
    CT_3d_Widget* getMainViewWidget();
    void setSliceWidgets(CT_2d_Widget* sagittalViewWidget, CT_2d_Widget* coronalViewWidget, CT_2d_Widget* axialViewWidget);
    CT_2d_Widget** getSliceWidget();

private:
    vtkSmartPointer<vtkPolyData> screwSource;
    vtkSmartPointer<vtkActor> screwActor;
    vtkSmartPointer<vtkBoxWidget> screwWidget;
    char screwName[30];
    void addCone();
    void addCustomScrew(const char* screwName);
    vtkSmartPointer<vtkBoxWidget> createBoxWidget();
    CT_3d_Widget* mainViewWidget;
    CT_2d_Widget* sliceWidget[3];
};

