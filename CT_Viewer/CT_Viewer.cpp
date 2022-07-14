#include "CT_Viewer.h"
#include <QFileDialog.h>
#include <QDebug>
#include "vtkHelper.h"
#include "itkHelper.h"
#include "uiHelper.h"
#include <qpushbutton.h>
#include <qmessagebox.h>
#include "ScrewOptionWidget.h"

CT_Viewer::CT_Viewer(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    // initialize QVTK_Widget windows
    this->renWin3D = createWindow(700, 370);
    for (int i = 0; i < 3; i++)
        this->renWin[i] = createWindow(230, 180);
    ui.mainViewWidget->SetRenderWindow(this->renWin3D);
    ui.axialViewWidget->SetRenderWindow(this->renWin[0]);
    ui.coronalViewWidget->SetRenderWindow(this->renWin[1]);
    ui.sagittalViewWidget->SetRenderWindow(this->renWin[2]);

    // connect actions
    connect(ui.actionOpen, SIGNAL(triggered()), this, SLOT(loadCT()));
    connect(ui.addButton, SIGNAL(clicked()), this, SLOT(handleAdd()));
    connect(ui.clearButton, SIGNAL(clicked()), this, SLOT(handleClear()));
    connect(ui.confirmButton, SIGNAL(clicked()), this, SLOT(handleConfirm()));
    connect(ui.detailButton, SIGNAL(clicked()), this, SLOT(handleDetail()));
}

void CT_Viewer::loadCT()
{
    if (this->ren[0] != nullptr) {
        this->renWin3D->RemoveRenderer(this->ren[0]);
    }
    
    // load file directory
    this->filename = QFileDialog::getExistingDirectory(this, "CT file directory", "../");
    this->ctImage = readCT(this->filename.toStdString().c_str());
    this->updatedImage = readCT(this->filename.toStdString().c_str());
    
    // load CT using ITK and convert to VTK image data
    CT_Data data = loadCTSeries(this->filename.toStdString().c_str());
    
    // check the path is valid
    if (!data.loadSucceed) {
        QMessageBox msgBox;
        msgBox.setText("No Dicom Files in the directory or Loading Failed!");
        msgBox.exec();
        return;
    }
    //this->ctImage = data.CTImage;
    //this->updatedImage = data.CTImage;
    
    // store meta data and display on labels
    this->dicomMetaDictionary = data.metaInfo;
    displayMetaInfo(ui, this->dicomMetaDictionary);
    this->CT_uploaded = true;

    // create 3D volume and add to window
    this->ren[3] = createRender3D(this->ctImage);
    this->renWin3D->AddRenderer(this->ren[3]);
    this->interactor3D = createAndBindInteractor(this->renWin3D); // set trackball camera style
    this->renWin3D->Render();

    // create 2D view for each slice
    for (int i = 0; i < 3; i++) {
        this->ctReslice[i] = createReslice(this->ctImage, i);
        this->ren[i] = createRender2D(this->ctReslice[i], this->renWin[i]);
        this->renWin[i]->Render();
    }
}

void CT_Viewer::handleAdd()
{
    if (!this->CT_uploaded) {
        return;
    }
    ScrewOptionWidget widget;
    widget.exec();
    if (!widget.confirmAction()) {
        return;
    }
    int model = widget.getSelectModel();
    qDebug() << "select: " << model;
    addScrew(model, this->screwList, this->screwActorList, this->ren[3], this->interactor3D);
}

void CT_Viewer::handleConfirm()
{
    if (!this->CT_uploaded) {
        return;
    }
    if (screwList.size() < 1) {
        QMessageBox msgBox;
        msgBox.setText("No cone is added!");
        msgBox.exec();
    }
    this->updatedImage = updateCTImage(this->updatedImage, screwList);
    for (int i = 0; i < 3; i++) {
        updateRender2D(this->ctReslice[i], this->renWin[i], this->updatedImage);
    }
    QMessageBox msgBox;
    msgBox.setText("Cones Are Added!");
    msgBox.exec();
}

void CT_Viewer::handleClear()
{
    if (!this->CT_uploaded) {
        return;
    }
    if (screwList.size() >= 1) {
        for (int i = 0; i < screwList.size(); i++) {
            screwList[i].second->SetEnabled(0);
            this->ren[3]->RemoveActor(screwActorList[i]);
        }
        screwList.clear();
        screwActorList.clear();
        for (int i = 0; i < 3; i++) {
            this->ctReslice[i]->SetInputData(0, this->ctImage);
            this->ctReslice[i]->Modified();
            this->renWin[i]->Render();
        }
    }
    this->updatedImage = readCT(this->filename.toStdString().c_str());
    QMessageBox msgBox;
    msgBox.setText("Clear!");
    msgBox.exec();
}

void CT_Viewer::handleDetail()
{
    if (!this->CT_uploaded) {
        return;
    }
    QMessageBox msgBox;
    QString detailText = displayDetails(this->dicomMetaDictionary);
    msgBox.setText(detailText);
    msgBox.exec();
}
