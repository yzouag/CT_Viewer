#include "vtkHelper.h"
#include <vtkNew.h>
#include <vtkNamedColors.h>
#include <vtkTextProperty.h>
#include <vtkCornerAnnotation.h>

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