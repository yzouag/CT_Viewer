#include "CT_Viewer.h"
#include <QFileDialog.h>
#include <QDir>
#include <QDebug>
#include "vtkHelper.h"
#include "uiHelper.h"
#include <qpushbutton.h>
#include <qmessagebox.h>
#include "ScrewOptionWidget.h"
#include "ct_details_widget.h"
#include "ct_contrast_widget.h"
#include "image_register.h"
#include <vtkCamera.h>
#include <vtkRendererCollection.h>
#include "actor_list_item.h"
#include <QListWidgetItem>
#include <vtkProp.h>

CT_Viewer::CT_Viewer(CT_Image* ctImage, QProgressDialog* dialog, QWidget *parent) : QMainWindow(parent)
{
    ui.setupUi(this);
    this->setWindowTitle("CT Viewer");
    dialog->setLabelText("Initialize UI ...");
    dialog->setValue(0);

    // set the view direction for each 2D widget
    // show the caption for each 2D View
    ui.sagittalViewWidget->setViewMode(Sagittal);
    ui.axialViewWidget->setViewMode(Axial);
    ui.coronalViewWidget->setViewMode(Coronal);

    // hide direction and other screw manipulation buttons
    // they will be recovered after first screw is added
    for (int i = 0; i < ui.gridLayout_2->count(); i++) {
        QWidget* widget = ui.gridLayout_2->itemAt(i)->widget();
        if (widget != nullptr) {
            widget->setVisible(false);
        }
    }
    for (int i = 0; i < ui.gridLayout_3->count(); i++) {
        QWidget* widget = ui.gridLayout_3->itemAt(i)->widget();
        if (widget != nullptr) {
            widget->setVisible(false);
        }
    }

    // connect actions
    connect(ui.actionOpen, SIGNAL(triggered()), this, SLOT(handleOpen()));
    connect(ui.actionSave, SIGNAL(triggered()), this, SLOT(handleSave()));
    connect(ui.addButton, SIGNAL(clicked()), this, SLOT(handleAdd()));
    connect(ui.clearButton, SIGNAL(clicked()), this, SLOT(handleClear()));
    connect(ui.detailButton, SIGNAL(clicked()), this, SLOT(handleDetail()));
    connect(ui.actionSet_Contrast, SIGNAL(triggered()), this, SLOT(handleSetContrast()));
    connect(ui.actionMeasurements, SIGNAL(triggered()), this, SLOT(handleMeasurements()));

    // connect for 2D view interactions (cursor location update, reslice position update)
    connect(ctImage, &CT_Image::sliceCenterChange, ui.axialViewWidget, &CT_2d_Widget::updateWhenSliceCenterChange);
    connect(ctImage, &CT_Image::sliceCenterChange, ui.coronalViewWidget, &CT_2d_Widget::updateWhenSliceCenterChange);
    connect(ctImage, &CT_Image::sliceCenterChange, ui.sagittalViewWidget, &CT_2d_Widget::updateWhenSliceCenterChange);
    
    // connect for 2D view color contrast
    connect(ctImage, &CT_Image::contrastThresholdChange, ui.axialViewWidget, &CT_2d_Widget::updateColorMap);
    connect(ctImage, &CT_Image::contrastThresholdChange, ui.coronalViewWidget, &CT_2d_Widget::updateColorMap);
    connect(ctImage, &CT_Image::contrastThresholdChange, ui.sagittalViewWidget, &CT_2d_Widget::updateColorMap);

    // connect scrollbar movement to the change of slice center in CT Image
    connect(ui.axialScrollBar, &QScrollBar::valueChanged, this, &CT_Viewer::handleAxialScrollBarChange);
    connect(ui.coronalScrollBar, &QScrollBar::valueChanged, this, &CT_Viewer::handleCoronalScrollBarChange);
    connect(ui.sagittalScrollBar, &QScrollBar::valueChanged, this, &CT_Viewer::handleSagittalScrollBarChange);

    // connect 2D, 3D views expand and resume
    connect(ui.d3Button, SIGNAL(clicked()), this, SLOT(handle3DView()));
    connect(ui.axialButton, SIGNAL(clicked()), this, SLOT(handleAxialView()));
    connect(ui.sagittalButton, SIGNAL(clicked()), this, SLOT(handleSagittalView()));
    connect(ui.coronalButton, SIGNAL(clicked()), this, SLOT(handleCoronalView()));

    // connect screen shot buttons
    connect(ui.d3ScreenCapture, SIGNAL(clicked()), this, SLOT(handle3DScreenshot()));
    connect(ui.axialScreenCapture, SIGNAL(clicked()), this, SLOT(handleAxialScreenshot()));
    connect(ui.coronalScreenCapture, SIGNAL(clicked()), this, SLOT(handleCoronalScreenshot()));
    connect(ui.sagittalScreenCapture, SIGNAL(clicked()), this, SLOT(handleSagittalScreenshot()));

    // reset 3D view camera
    connect(ui.d3ResetButton, SIGNAL(clicked()), this, SLOT(handle3DReset()));

    // connect screw manipulation buttons
    connect(ui.upButton, SIGNAL(clicked()), this, SLOT(onScrewButtonClick()));
    connect(ui.downButton, SIGNAL(clicked()), this, SLOT(onScrewButtonClick()));
    connect(ui.rightButton, SIGNAL(clicked()), this, SLOT(onScrewButtonClick()));
    connect(ui.leftButton, SIGNAL(clicked()), this, SLOT(onScrewButtonClick()));
    connect(ui.frontButton, SIGNAL(clicked()), this, SLOT(onScrewButtonClick()));
    connect(ui.backButton, SIGNAL(clicked()), this, SLOT(onScrewButtonClick()));

    // connect spin box and slider
    connect(ui.rotateISSlider, &QSlider::valueChanged, ui.rotateISSpinBox, [=](int value) { ui.rotateISSpinBox->setValue(value/10.0); });
    connect(ui.rotateISSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), ui.rotateISSlider, [=](double value) { ui.rotateISSlider->setValue(static_cast<int>(value*10)); });
    connect(ui.rotateLRSlider, &QSlider::valueChanged, ui.rotateLRSpinBox, [=](int value) { ui.rotateLRSpinBox->setValue(value/10.0); });
    connect(ui.rotateLRSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), ui.rotateLRSlider, [=](double value) { ui.rotateLRSlider->setValue(static_cast<int>(value*10)); });
    // then send signal to manipulate screws
    connect(ui.rotateISSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CT_Viewer::onScrewSliderChange);
    connect(ui.rotateLRSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CT_Viewer::onScrewSliderChange);

    this->ctImage = ctImage;
    this->CT_uploaded = true;

    displayMetaInfo(ui, this->ctImage->getMetaInfo());

    dialog->setValue(100);
    dialog->setLabelText("initialize Four Views ...");
    dialog->setValue(0);

    QList<vtkProp*> actorList;
    // create 3D volume and add to window
    actorList.append(ui.mainViewWidget->setCTImage(this->ctImage->getCTImageData()));
    dialog->setValue(20);

    // create 2D view for each slice
    actorList.append(ui.sagittalViewWidget->renderCTReslice(this->ctImage));
    dialog->setValue(40);
    actorList.append(ui.coronalViewWidget->renderCTReslice(this->ctImage));
    dialog->setValue(60);
    actorList.append(ui.axialViewWidget->renderCTReslice(this->ctImage));
    dialog->setValue(80);
    
    // add the CT Model to the List View
    QListWidgetItem *item = new QListWidgetItem;
    ActorListItem* boneModelItem = new ActorListItem(DICOM_IMAGE, this->ctImage->getMetaInfo()["Patient Name"], item);
    boneModelItem->setCorrespondingActors(actorList);
    item->setSizeHint(QSize(210,24));
    item->setFlags(Qt::NoItemFlags);
    ui.listWidget->addItem(item);
    ui.listWidget->setItemWidget(item, boneModelItem);
    connect(boneModelItem, &ActorListItem::actorChanged, this, &CT_Viewer::updateViews);
    connect(boneModelItem, &ActorListItem::widgetDeleted, this, &CT_Viewer::removeListItem);

    // also set the range of the scroll bar
    ui.sagittalViewWidget->setScrollBar(this->ui.sagittalScrollBar);
    ui.coronalViewWidget->setScrollBar(this->ui.coronalScrollBar);
    ui.axialViewWidget->setScrollBar(this->ui.axialScrollBar);

    dialog->setValue(100);
    // this is to make sure when load a new Dicom image
    // the contrast should reset
    CT_Contrast_Widget::first_init = true;
}

// really ugly design, hope someday I will learn how to do this
// no any experience about using exceptions
CT_Viewer::~CT_Viewer()
{
    qDebug() << QDir(this->ctImage->getFilePath()).dirName() << this->ctImage->getFilePath();
    try {
        ImageRegister imageInfo(QDir(this->ctImage->getFilePath()).dirName(), this->ctImage->getFilePath(), this->ui.axialViewWidget, &this->screwList);
        imageInfo.setContrastThreshold(this->ctImage->getContrastThreshold()[0], this->ctImage->getContrastThreshold()[1]);
        imageInfo.setSliceCenter(this->ctImage->getSliceCenter());
        ui.mainViewWidget->getCameraSettings(imageInfo.getCameraPos(), imageInfo.getFocalPoint());
        imageInfo.save();
    }
    catch(int num){
        qDebug() << "Failed to create directory";
    }
    delete this->ctImage;
}

// ask to input filename, save it as pixmap to local directory
// now only support PNG format
void CT_Viewer::takeScreenshot(QWidget* widget)
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Screen Shot..."),"../screenshot.png",tr("Images (*.png)"));
    QPixmap pixmap(widget->size());
    widget->render(&pixmap);
    QFile file(fileName);
    file.open(QIODevice::WriteOnly);
    pixmap.save(&file, "PNG");
}

// trigger when click open, open another image and create a new CT_Viewer
void CT_Viewer::handleOpen()
{
    QString filename = QFileDialog::getExistingDirectory(this, "CT file directory", "../");
    // if no file inputs
    if (filename.length() == 0) {
        return;
    }

    QProgressDialog* progressDialog = createProgressDialog(tr("CT Loading"), tr("Loading CT, Please Wait..."), 101);
    CT_Image* ctImage = new CT_Image();
    ctImage->loadDicomFromDirectory(filename, progressDialog);
    progressDialog->deleteLater();

    // check the path is valid
    if (!ctImage->checkLoadSuccess()) {
        progressDialog->close();
        QMessageBox msgBox;
        msgBox.setText("No Dicom Files in the directory or Loading Failed!");
        msgBox.exec();
        delete ctImage;
        return;
    }
    this->close();

    // load the main window
    CT_Viewer* w = new CT_Viewer(ctImage, progressDialog);
    w->setAttribute(Qt::WA_DeleteOnClose);
    w->show();
    w->init2DViews();
    this->ctImage->deleteLater();
}

// two options for save:
// 1. save the image + screws as a new DICOM image
// 2. save as a project directory (or a JSON file?)
void CT_Viewer::handleSave()
{
    QString dirName = QFileDialog::getExistingDirectory(this, "Save DICOM files to this directory", "../");
    // if no file inputs
    if (dirName.length() == 0) {
        return;
    }
    QProgressDialog* dialog = createProgressDialog("Save", "Exporting Image...", 100);
    this->ctImage->updateImage(screwList, dialog); // TODO: How to exclude removed screws?
    this->ctImage->saveImageData(dirName, dialog);
    dialog->deleteLater();
}

// when add button clicked, add different screw type to 
// the 3D view, pop out message dialog to select screw type
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
    PlantingScrews* screw = new PlantingScrews(widget.getSelectModel());
    this->addScrew(screw);
}

void CT_Viewer::addScrew(PlantingScrews* screw)
{
    screw->setMainViewWidget(this->ui.mainViewWidget);
    screw->setSliceWidgets(this->ui.sagittalViewWidget, this->ui.coronalViewWidget, this->ui.axialViewWidget);
    screwList.append(screw);

    QList<vtkProp*> actorList;
    ui.mainViewWidget->addScrew(screw);
    actorList.append(ui.sagittalViewWidget->addScrew(screw));
    actorList.append(ui.coronalViewWidget->addScrew(screw));
    actorList.append(ui.axialViewWidget->addScrew(screw));

    actorList.append(screw->getScrewActor());
    QListWidgetItem *item = new QListWidgetItem;
    ActorListItem* screwModelItem;
    if (strncmp("cone", screw->getScrewName(), 5) == 0) {
        screwModelItem = new ActorListItem(CONE, "cone", item);
    } else {
        screwModelItem = new ActorListItem(SCREW, screw->getScrewName(), item);
    }
    screwModelItem->setCorrespondingBoxWidget(screw->getScrewWidget());
    screwModelItem->setCorrespondingActors(actorList);
    item->setSizeHint(QSize(210, 24));
    item->setFlags(Qt::NoItemFlags);
    ui.listWidget->addItem(item);
    ui.listWidget->setItemWidget(item, screwModelItem);
    connect(screwModelItem, &ActorListItem::actorChanged, this, &CT_Viewer::updateViews);
    connect(screwModelItem, &ActorListItem::colorChanged, this, &CT_Viewer::updateColors);
    connect(screwModelItem, &ActorListItem::widgetDeleted, this, &CT_Viewer::removeListItem);

    // display the direction and other buttons for manipulating the screws
    for (int i = 0; i < ui.gridLayout_2->count(); i++) {
        QWidget* widget = ui.gridLayout_2->itemAt(i)->widget();
        if (widget != nullptr) {
            widget->setVisible(true);
        }
    }
    for (int i = 0; i < ui.gridLayout_3->count(); i++) {
        QWidget* widget = ui.gridLayout_3->itemAt(i)->widget();
        if (widget != nullptr) {
            widget->setVisible(true);
        }
    }
}

// remove all existing screws
void CT_Viewer::handleClear()
{
    if (!this->CT_uploaded) {
        return;
    }
    if (this->screwList.size() >= 1) {
        ui.mainViewWidget->removeAll(this->screwList);
        ui.sagittalViewWidget->removeAll();
        ui.coronalViewWidget->removeAll();
        ui.axialViewWidget->removeAll();
        this->screwList.clear();
    }
    QMessageBox msgBox;
    msgBox.setText("Clear!");
    msgBox.exec();
}

// pop out a message box with a table for detailed CT info
void CT_Viewer::handleDetail()
{
    if (!this->CT_uploaded) {
        return;
    }
    CT_Details_Widget* detail_widget = new CT_Details_Widget(this);
    detail_widget->setAttribute(Qt::WA_DeleteOnClose);
    detail_widget->setTableContent(this->ctImage->getMetaInfo());
    detail_widget->show();
}

void CT_Viewer::handleMeasurements()
{
    /*vtkNew<vtkNamedColors> colors;
    vtkNew<vtkAngleWidget> angleWidget;
    angleWidget->SetInteractor(renderWindowInteractor);
    vtkNew<vtkAngleRepresentation2D> angleRep;
    angleRep->GetRay1()->GetProperty()->SetColor(255, 0, 0);
    angleRep->GetRay2()->GetProperty()->SetColor(255, 0, 0);
    angleRep->GetArc()->GetProperty()->SetColor(colors->GetColor3d("cadmium_lemon").GetData());
    angleRep->GetArc()->GetLabelTextProperty()->SetColor(colors->GetColor3d("naples_yellow_deep").GetData());
    angleWidget->SetRepresentation(angleRep);
    angleWidget->On();*/
}

void CT_Viewer::updateViews()
{
    ui.mainViewWidget->GetRenderWindow()->Render();
    ui.sagittalViewWidget->GetRenderWindow()->Render();
    ui.coronalViewWidget->GetRenderWindow()->Render();
    ui.axialViewWidget->GetRenderWindow()->Render();
}

void CT_Viewer::updateColors(vtkBoxWidget* widget, double r, double g, double b)
{
    if (widget == ui.mainViewWidget->getActiveScrew()) {
        ui.mainViewWidget->getLastPickedProperty()->SetColor(r, g, b);
    }
    updateViews();
}

void CT_Viewer::removeListItem(QWidget* widget, QListWidgetItem* item)
{
    updateViews();
    delete widget;
    delete item;
}

void CT_Viewer::handleAxialScrollBarChange(int val)
{
    double* sliceCenter = this->ctImage->getSliceCenter();
    this->ctImage->updateSliceCenter(sliceCenter[0], sliceCenter[1], val);
}

void CT_Viewer::handleCoronalScrollBarChange(int val)
{
    double* sliceCenter = this->ctImage->getSliceCenter();
    this->ctImage->updateSliceCenter(sliceCenter[0], val, sliceCenter[2]);
}

void CT_Viewer::handleSagittalScrollBarChange(int val)
{
    double* sliceCenter = this->ctImage->getSliceCenter();
    this->ctImage->updateSliceCenter(val, sliceCenter[1], sliceCenter[2]);
}

// control the zoom in and zoom out of the 3D view
void CT_Viewer::handle3DView()
{
    bool checked = ui.d3Button->isChecked();
    if (checked) {
        ui.verticalLayout_3->setStretch(0, 0);
        ui.verticalLayout_3->setStretch(1, 0);
        ui.d3Button->setIcon(QIcon(":/CT_Viewer/resources/dl_fourviews.png"));
        ui.axialWidget->hide();
        ui.coronalWidget->hide();
        ui.sagittalWidget->hide();
    }
    else {
        ui.verticalLayout_3->setStretch(0, 3);
        ui.verticalLayout_3->setStretch(1, 2);
        ui.d3Button->setIcon(QIcon(":/CT_Viewer/resources/dl_3d.png"));
        ui.axialWidget->show();
        ui.coronalWidget->show();
        ui.sagittalWidget->show();
    }
}

// control the zoom in and zoom out of the axial view
void CT_Viewer::handleAxialView()
{
    bool checked = ui.axialButton->isChecked();
    if (checked) {
        ui.verticalLayout_3->setStretch(0, 0);
        ui.verticalLayout_3->setStretch(1, 0);
        ui.axialButton->setIcon(QIcon(":/CT_Viewer/resources/dl_fourviews.png"));
        ui.mainWidget->hide();
        ui.coronalWidget->hide();
        ui.sagittalWidget->hide();
    } else {
        ui.verticalLayout_3->setStretch(0, 3);
        ui.verticalLayout_3->setStretch(1, 2);
        ui.axialButton->setIcon(QIcon(":/CT_Viewer/resources/dl_axial.png"));
        ui.mainWidget->show();
        ui.coronalWidget->show();
        ui.sagittalWidget->show();
    }
}

// control the zoom in and zoom out of the coronal view
void CT_Viewer::handleCoronalView()
{
    bool checked = ui.coronalButton->isChecked();
    if (checked) {
        ui.verticalLayout_3->setStretch(0, 0);
        ui.verticalLayout_3->setStretch(1, 0);
        ui.coronalButton->setIcon(QIcon(":/CT_Viewer/resources/dl_fourviews.png"));
        ui.axialWidget->hide();
        ui.mainWidget->hide();
        ui.sagittalWidget->hide();
    } else {
        ui.verticalLayout_3->setStretch(0, 3);
        ui.verticalLayout_3->setStretch(1, 2);
        ui.coronalButton->setIcon(QIcon(":/CT_Viewer/resources/dl_coronal.png"));
        ui.axialWidget->show();
        ui.mainWidget->show();
        ui.sagittalWidget->show();
    }
}


// control the zoom in and zoom out of the sagittal view
void CT_Viewer::handleSagittalView()
{
    bool checked = ui.sagittalButton->isChecked();
    if (checked) {
        ui.verticalLayout_3->setStretch(0, 0);
        ui.verticalLayout_3->setStretch(1, 0);
        ui.sagittalButton->setIcon(QIcon(":/CT_Viewer/resources/dl_fourviews.png"));
        ui.mainWidget->hide();
        ui.coronalWidget->hide();
        ui.axialWidget->hide();
    } else {
        ui.verticalLayout_3->setStretch(0, 3);
        ui.verticalLayout_3->setStretch(1, 2);
        ui.sagittalButton->setIcon(QIcon(":/CT_Viewer/resources/dl_sagittal.png"));
        ui.mainWidget->show();
        ui.coronalWidget->show();
        ui.axialWidget->show();
    }
}

// take screen shot of the 3D view widget window
void CT_Viewer::handle3DScreenshot()
{
    takeScreenshot(this->ui.mainViewWidget);
}

// take screen shot of the axial view widget window
void CT_Viewer::handleAxialScreenshot()
{
    takeScreenshot(this->ui.axialViewWidget);
}

// take screen shot of the coronal view widget window
void CT_Viewer::handleCoronalScreenshot()
{
    takeScreenshot(this->ui.coronalViewWidget);
}

// take screen shot of the sagittal view widget window
void CT_Viewer::handleSagittalScreenshot()
{
    takeScreenshot(this->ui.sagittalViewWidget);
}

// TODO: reset the 3D scene to the front view
void CT_Viewer::handle3DReset()
{
    qDebug() << "reset";
    this->ui.mainViewWidget->reset();
}

// pop out a widget for contrast adjustment, the widget will connect with 2D views
void CT_Viewer::handleSetContrast()
{
    if (!this->CT_uploaded) {
        return;
    }
    CT_Contrast_Widget* contrast_widget = new CT_Contrast_Widget(this->ctImage, this);
    contrast_widget->setAttribute(Qt::WA_DeleteOnClose);
    contrast_widget->show();
}

// direction buttons and how screws will move when one direction button is clicked
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

// when slider change, update the spin box and also 3D scene screw orientation
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

// this is required, don't know why
// must be called after constructor
// put this code in constructor will lead to an empty scene of 2D views
void CT_Viewer::init2DViews()
{
    ui.axialViewWidget->GetRenderWindow()->Render();
    ui.coronalViewWidget->GetRenderWindow()->Render();
    ui.sagittalViewWidget->GetRenderWindow()->Render();
}

void CT_Viewer::loadSliceAndThreshold(double* sliceCenter, int* contrastThreshold)
{
    // load color contrast
    ui.sagittalViewWidget->updateColorMap(contrastThreshold[0], contrastThreshold[1]);
    ui.coronalViewWidget->updateColorMap(contrastThreshold[0], contrastThreshold[1]);
    ui.axialViewWidget->updateColorMap(contrastThreshold[0], contrastThreshold[1]);

    // load slice center
    ui.sagittalViewWidget->updateWhenSliceCenterChange(sliceCenter[0], sliceCenter[1], sliceCenter[2]);
    ui.coronalViewWidget->updateWhenSliceCenterChange(sliceCenter[0], sliceCenter[1], sliceCenter[2]);
    ui.axialViewWidget->updateWhenSliceCenterChange(sliceCenter[0], sliceCenter[1], sliceCenter[2]);
}

void CT_Viewer::loadCameraSettings(double * cameraPos, double * focalPoint)
{
    this->ui.mainViewWidget->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->GetActiveCamera()->SetPosition(cameraPos);
    this->ui.mainViewWidget->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->GetActiveCamera()->SetFocalPoint(focalPoint);
}

void CT_Viewer::loadScrews(QVector<QPair<QString, QVector<double>>>& screwActorList)
{
    for (int i = 0; i < screwActorList.size(); i++) {
        QString screwName = screwActorList.at(i).first;
        PlantingScrews* screw = new PlantingScrews(screwName.toStdString().c_str(), screwActorList.at(i).second);
        this->addScrew(screw);
    }
}
