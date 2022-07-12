#include "CT_Viewer.h"
#include <QFileDialog.h>
#include "vtkHelper.h"
#include <qpushbutton.h>
#include <qmessagebox.h>

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
    addCone(this->coneList, this->coneActorList, this->ren[3], this->interactor3D);
}

void CT_Viewer::handleConfirm()
{
    if (coneList.size() < 1) {
        QMessageBox msgBox;
        msgBox.setText("No cone is added!");
        msgBox.exec();
    }
    this->updatedImage = updateCTImage(this->updatedImage, coneList);
    for (int i = 0; i < 3; i++) {
        updateRender2D(this->ctReslice[i], this->renWin[i], this->updatedImage);
    }
    QMessageBox msgBox;
    msgBox.setText("Cones Are Added!");
    msgBox.exec();
}

void CT_Viewer::handleClear()
{
    if (coneList.size() >= 1) {
        for (int i = 0; i < coneList.size(); i++) {
            coneList[i]->SetEnabled(0);
            this->ren[3]->RemoveActor(coneActorList[i]);
        }
        coneList.clear();
        coneActorList.clear();
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
