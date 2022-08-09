#pragma once
#include "vtkCommand.h"
#include "ct_3d_widget.h"
#include "ct_2d_widget.h"
#include "planting_screws.h"

class BoxWidgetCallback : public vtkCommand
{
public:
    static BoxWidgetCallback* New()
    {
        return new BoxWidgetCallback;
    }
    ~BoxWidgetCallback();
    virtual void Execute(vtkObject* caller, unsigned long, void*);
    void setScrew(PlantingScrews* screw);

private:
    PlantingScrews* screw;
    vtkPolyData* data;
    vtkPolyData* originData;
};

