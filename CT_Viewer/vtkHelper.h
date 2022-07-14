#pragma once
#ifndef VTKHELPER_H
#define VTKHELPER_H

#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageReslice.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkBoxWidget.h>
#include <vtkActor.h>
#include <qvector.h>

vtkSmartPointer<vtkImageData> readCT(const char* filePath);
vtkSmartPointer<vtkGenericOpenGLRenderWindow> createWindow(int x, int y);
vtkSmartPointer<vtkRenderer> createRender3D(vtkSmartPointer<vtkImageData> ctImage);
vtkSmartPointer<vtkRenderWindowInteractor> createAndBindInteractor(vtkRenderWindow* renWin);
vtkSmartPointer<vtkRenderer> createRender2D(vtkImageReslice* ctReslice, vtkRenderWindow* renWin);
vtkSmartPointer<vtkImageReslice> createReslice(vtkImageData* ctImage, int axis);
void addCone(QVector<QPair<const char*, vtkSmartPointer<vtkBoxWidget>>>& coneList, QVector<vtkSmartPointer<vtkActor>>& coneActorList, vtkRenderer* ren, vtkRenderWindowInteractor* interactor);
void addCustomScrew(const char* path, QVector<QPair<const char*, vtkSmartPointer<vtkBoxWidget>>>& coneList, QVector<vtkSmartPointer<vtkActor>>& coneActorList, vtkRenderer* ren, vtkRenderWindowInteractor* interactor);
void addScrew(int model, QVector<QPair<const char*, vtkSmartPointer<vtkBoxWidget>>>& coneList, QVector<vtkSmartPointer<vtkActor>>& coneActorList, vtkRenderer* ren, vtkRenderWindowInteractor* interactor);
vtkSmartPointer<vtkImageData> updateCTImage(vtkSmartPointer<vtkImageData> ctImage, QVector<QPair<const char*, vtkSmartPointer<vtkBoxWidget>>>& coneList);
void updateRender2D(vtkImageReslice* ctReslice, vtkRenderWindow* renWin, vtkImageData* ctImage);
#endif