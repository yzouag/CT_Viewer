#pragma once
#ifndef VTKHELPER_H
#define VTKHELPER_H

#include <vtkRenderer.h>

enum ViewMode
{
    Sagittal, Coronal, Axial, ThreeDimension
};

void setHeader(vtkRenderer* ren, int axis);

#endif