#include "box_widget_callback.h"
#include <vtkNamedColors.h>
#include <vtkTransform.h>
#include <vtkPolyDataMapper.h>
#include <vtkTransformPolyDataFilter.h>

BoxWidgetCallback::~BoxWidgetCallback()
{
    this->originData->Delete();
}

void BoxWidgetCallback::Execute(vtkObject* caller, unsigned long, void*)
{
    vtkNew<vtkTransform> t;
    vtkBoxWidget* widget = reinterpret_cast<vtkBoxWidget*>(caller);
    widget->GetTransform(t);
    vtkNew<vtkTransformPolyDataFilter> filter;
    filter->SetTransform(t);
    filter->SetInputData(this->originData);
    filter->Update();
    data->DeepCopy(filter->GetOutput());
    data->Modified();
    for (int i = 0; i < 3; i++) {
        this->screw->getSliceWidget()[i]->GetRenderWindow()->Render();
    }

    // if the current active widget is the same, return
    if (this->screw->getMainViewWidget()->getActiveScrew() == widget)
        return;

    // first reset the property of last picked actor if this is not the first screw created
    if (this->screw->getMainViewWidget()->getActiveScrew() != nullptr) {
        vtkActor* lastPickedActor = reinterpret_cast<vtkActor*>(this->screw->getMainViewWidget()->getActiveScrew()->GetProp3D());
        lastPickedActor->GetProperty()->DeepCopy(this->screw->getMainViewWidget()->getLastPickedProperty());
    }
    this->screw->getMainViewWidget()->setActiveScrew(widget);
    // update the last picked actor to current actor
    vtkActor* currentPickedActor = reinterpret_cast<vtkActor*>(widget->GetProp3D());
    this->screw->getMainViewWidget()->getLastPickedProperty()->DeepCopy(currentPickedActor->GetProperty());

    // highlight the current chosen actor
    vtkNew<vtkNamedColors> colors;
    currentPickedActor->GetProperty()->SetColor(colors->GetColor3d("cadmium_lemon").GetData());
    currentPickedActor->GetProperty()->SetDiffuse(1.0);
    currentPickedActor->GetProperty()->SetSpecular(0.0);
}

void BoxWidgetCallback::setScrew(PlantingScrews* screw)
{
    this->screw = screw;
    vtkPolyDataMapper* mapper = static_cast<vtkPolyDataMapper*>(this->screw->getScrewActor()->GetMapper());
    data = mapper->GetInput();
    originData = vtkPolyData::New();
    originData->DeepCopy(mapper->GetInput());
}
