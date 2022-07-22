#include "CT_Viewer.h"
#include <QFileDialog.h>
#include <QDebug>
#include "vtkHelper.h"
#include "uiHelper.h"
#include <qpushbutton.h>
#include <qmessagebox.h>
#include "ScrewOptionWidget.h"

CT_Viewer::CT_Viewer(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    ui.sagittalViewWidget->setViewMode(Sagittal);
    ui.axialViewWidget->setViewMode(Axial);
    ui.coronalViewWidget->setViewMode(Coronal);

    // connect actions
    connect(ui.actionOpen, SIGNAL(triggered()), this, SLOT(loadCT()));
    connect(ui.addButton, SIGNAL(clicked()), this, SLOT(handleAdd()));
    connect(ui.clearButton, SIGNAL(clicked()), this, SLOT(handleClear()));
    connect(ui.confirmButton, SIGNAL(clicked()), this, SLOT(handleConfirm()));
    connect(ui.detailButton, SIGNAL(clicked()), this, SLOT(handleDetail()));

    // connect for 2D view interactions
    // is there anyway to connect the signals more convenient?
    connect(ui.sagittalViewWidget, &CT_2d_Widget::cursorPosChange, ui.coronalViewWidget, &CT_2d_Widget::updateWhenCursorPosChange);
    connect(ui.sagittalViewWidget, &CT_2d_Widget::cursorPosChange, ui.axialViewWidget, &CT_2d_Widget::updateWhenCursorPosChange);
    connect(ui.coronalViewWidget, &CT_2d_Widget::cursorPosChange, ui.sagittalViewWidget, &CT_2d_Widget::updateWhenCursorPosChange);
    connect(ui.coronalViewWidget, &CT_2d_Widget::cursorPosChange, ui.axialViewWidget, &CT_2d_Widget::updateWhenCursorPosChange);
    connect(ui.axialViewWidget, &CT_2d_Widget::cursorPosChange, ui.sagittalViewWidget, &CT_2d_Widget::updateWhenCursorPosChange);
    connect(ui.axialViewWidget, &CT_2d_Widget::cursorPosChange, ui.coronalViewWidget, &CT_2d_Widget::updateWhenCursorPosChange);

    connect(ui.sagittalViewWidget, &CT_2d_Widget::reslicePosChange, ui.coronalViewWidget, &CT_2d_Widget::updateWhenReslicePosChange);
    connect(ui.sagittalViewWidget, &CT_2d_Widget::reslicePosChange, ui.axialViewWidget, &CT_2d_Widget::updateWhenReslicePosChange);
    connect(ui.coronalViewWidget, &CT_2d_Widget::reslicePosChange, ui.sagittalViewWidget, &CT_2d_Widget::updateWhenReslicePosChange);
    connect(ui.coronalViewWidget, &CT_2d_Widget::reslicePosChange, ui.axialViewWidget, &CT_2d_Widget::updateWhenReslicePosChange);
    connect(ui.axialViewWidget, &CT_2d_Widget::reslicePosChange, ui.sagittalViewWidget, &CT_2d_Widget::updateWhenReslicePosChange);
    connect(ui.axialViewWidget, &CT_2d_Widget::reslicePosChange, ui.coronalViewWidget, &CT_2d_Widget::updateWhenReslicePosChange);

    // connect screw manipulation buttons
    connect(ui.upButton, SIGNAL(clicked()), this, SLOT(onScrewButtonClick()));
    connect(ui.downButton, SIGNAL(clicked()), this, SLOT(onScrewButtonClick()));
    connect(ui.rightButton, SIGNAL(clicked()), this, SLOT(onScrewButtonClick()));
    connect(ui.leftButton, SIGNAL(clicked()), this, SLOT(onScrewButtonClick()));
    connect(ui.frontButton, SIGNAL(clicked()), this, SLOT(onScrewButtonClick()));
    connect(ui.backButton, SIGNAL(clicked()), this, SLOT(onScrewButtonClick()));

    // connect spin box and slider, then send signal to manipulate screws
    connect(ui.rotateISSlider, &QSlider::valueChanged, ui.rotateISSpinBox, [=](int value) { ui.rotateISSpinBox->setValue(value/10.0); });
    connect(ui.rotateISSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), ui.rotateISSlider, [=](double value) { ui.rotateISSlider->setValue(static_cast<int>(value*10)); });
    connect(ui.rotateLRSlider, &QSlider::valueChanged, ui.rotateLRSpinBox, [=](int value) { ui.rotateLRSpinBox->setValue(value/10.0); });
    connect(ui.rotateLRSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), ui.rotateLRSlider, [=](double value) { ui.rotateLRSlider->setValue(static_cast<int>(value*10)); });
    
    connect(ui.rotateISSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CT_Viewer::onScrewSliderChange);
    connect(ui.rotateLRSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CT_Viewer::onScrewSliderChange);
}

void CT_Viewer::loadCT()
{
    ui.statusBar->showMessage("Loading CT...");
    
    // load file directory
    QString filename = QFileDialog::getExistingDirectory(this, "CT file directory", "../");
    QProgressDialog* progressDialog = createProgressDialog(tr("CT Loading"), tr("Loading CT, Please Wait..."), 100);
    this->ctImage.loadDicomFromDirectory(filename.toStdString().c_str(), progressDialog);
    
    // check the path is valid
    if (!this->ctImage.checkLoadSuccess()) {
        QMessageBox msgBox;
        msgBox.setText("No Dicom Files in the directory or Loading Failed!");
        msgBox.exec();
        return;
    }
    
    displayMetaInfo(ui, this->ctImage.getMetaInfo());
    this->CT_uploaded = true;

    // create 3D volume and add to window
    ui.mainViewWidget->setCTImage(this->ctImage.getCTImageData());
    ui.mainViewWidget->loadCT();

    // create 2D view for each slice
    ui.sagittalViewWidget->renderCTReslice(this->ctImage.getCTImageReslice(Sagittal));
    ui.coronalViewWidget->renderCTReslice(this->ctImage.getCTImageReslice(Coronal));
    ui.axialViewWidget->renderCTReslice(this->ctImage.getCTImageReslice(Axial));

    progressDialog->deleteLater();
    ui.statusBar->clearMessage();
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
    qDebug() << "select: " << widget.getSelectModel();
    ui.mainViewWidget->addScrew(widget.getSelectModel());
}

void CT_Viewer::handleConfirm()
{
    if (!this->CT_uploaded) {
        return;
    }
    if (ui.mainViewWidget->getScrewList().size() < 1) {
        QMessageBox msgBox;
        msgBox.setText("No Screw Is Added!");
        msgBox.exec();
    }
    
    // TODO: UNDO and REDO
    this->ctImage.updateImage(ui.mainViewWidget->getScrewList());
    ui.mainViewWidget->confirmActors();
    ui.sagittalViewWidget->updateCTReslice(this->ctImage.getCTImageData());
    ui.coronalViewWidget->updateCTReslice(this->ctImage.getCTImageData());
    ui.axialViewWidget->updateCTReslice(this->ctImage.getCTImageData());

    QMessageBox msgBox;
    msgBox.setText("Screws Are Updated!");
    msgBox.exec();
}

void CT_Viewer::handleClear()
{
    if (!this->CT_uploaded) {
        return;
    }
    if (ui.mainViewWidget->getScrewList().size() >= 1) {
        ui.mainViewWidget->removeAll();
        this->ctImage.resetImage();
        ui.sagittalViewWidget->updateCTReslice(this->ctImage.getCTImageData());
        ui.coronalViewWidget->updateCTReslice(this->ctImage.getCTImageData());
        ui.axialViewWidget->updateCTReslice(this->ctImage.getCTImageData());
    }
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
    QString detailText = displayDetails(this->ctImage.getMetaInfo());
    msgBox.setText(detailText);
    msgBox.exec();
}

void CT_Viewer::onScrewButtonClick()
{
    QObject* obj = sender();
    if (obj == ui.upButton) {
        ui.mainViewWidget->moveScrew(UP);
    }
    else if (obj == ui.downButton) {
        ui.mainViewWidget->moveScrew(DOWN);
    }
    else if (obj == ui.rightButton) {
        ui.mainViewWidget->moveScrew(RIGHT);
    }
    else if (obj == ui.leftButton) {
        ui.mainViewWidget->moveScrew(LEFT);
    }
    else if (obj == ui.frontButton) {
        ui.mainViewWidget->moveScrew(FRONT);
    }
    else {
        ui.mainViewWidget->moveScrew(BACK);
    }
}

void CT_Viewer::onScrewSliderChange(double value)
{
    QObject* obj = sender();
    if (obj == ui.rotateISSpinBox) {
        ui.mainViewWidget->moveScrew(ROTATE_IS, value);
    }
    else if (obj == ui.rotateLRSpinBox) {
        ui.mainViewWidget->moveScrew(ROTATE_LR, value);
    }
}
