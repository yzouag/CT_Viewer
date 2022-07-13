#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_CT_Viewer.h"
#include <vtkImageData.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkImageReslice.h>
#include <vtkBoxWidget.h>
#include <vtkActor.h>
#include <QMap>
#include <QString>

class CT_Viewer : public QMainWindow
{
    Q_OBJECT

public:
    CT_Viewer(QWidget *parent = Q_NULLPTR);

private:
    bool CT_uploaded = false;     // CT_uploaded allows other buttons to function
    QString filename;
    QMap<QString, QString> dicomMetaDictionary;
    Ui::CT_ViewerClass ui;
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> renWin3D;
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> renWin[3];
    vtkSmartPointer<vtkImageData> ctImage;
    vtkSmartPointer<vtkImageData> updatedImage;
    vtkSmartPointer<vtkRenderer> ren[4];
    vtkSmartPointer<vtkRenderWindowInteractor> interactor3D;
    vtkSmartPointer<vtkImageReslice> ctReslice[3];
    QVector<vtkSmartPointer<vtkBoxWidget>> coneList;
    QVector<vtkSmartPointer<vtkActor>> coneActorList;

private slots:
    void loadCT();
    void handleAdd();
    void handleConfirm();
    void handleClear();
    void handleDetail();
};
