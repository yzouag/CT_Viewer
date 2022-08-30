#include "CT_Viewer.h"
#include "entry_widget.h"
#include "uiHelper.h"
#include "ct_image.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include "image_register.h"
#include "recent_image_list_widget_item.h"
#include <QDebug>

Entry_Widget::Entry_Widget(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);
    this->setWindowTitle("CT Viewer");
    connect(ui.openImageButton, SIGNAL(clicked()), this, SLOT(handleOpenImage()));

    ui.tabWidget->setCurrentWidget(ui.tab);
    this->loadRecentImages();
    this->loadRecentWorkspaces();
}

Entry_Widget::~Entry_Widget()
{
}

void Entry_Widget::loadRecentImages()
{
    // load the directory, if not exists, skip, or put on list
    if (!QDir("../cache/").exists()) {
        return;
    }
    ui.tabWidget->setCurrentWidget(this->ui.tab_2);
    QStringList recentImageNameList = QDir("../cache/imageInfo").entryList(QDir::Files);
    for (QString fileName : recentImageNameList) {
        QString completePath = "../cache/imageInfo/" + fileName;
        ImageRegister* im = new ImageRegister(completePath); // will be deleted in RecentImageListWidgetItem
        RecentImageListWidgetItem* newItem = new RecentImageListWidgetItem(im, QIcon(im->getImageThumbnail()), im->getFileName(), ui.recentImageListWidget);
        newItem->setForeground(QBrush(Qt::white));
    }
    ui.recentImageListWidget->setGridSize(QSize(200, 210));
    ui.recentImageListWidget->setIconSize(QSize(200, 200));

    connect(ui.recentImageListWidget, &QListWidget::itemDoubleClicked, this, &Entry_Widget::handleSelectHistoryImage);
}

void Entry_Widget::loadRecentWorkspaces()
{
    // load the directory, if not exists, skip, or put on list
    if (!QDir("../cache/").exists()) {
        return;
    }
    QStringList recentImageNameList = QDir("../cache/imageInfo").entryList(QDir::Files);
    for (QString fileName : recentImageNameList) {
        QString completePath = "../cache/imageInfo/" + fileName;
        ImageRegister* im = new ImageRegister(completePath); // will be deleted in RecentImageListWidgetItem
        RecentImageListWidgetItem* newItem = new RecentImageListWidgetItem(im, QIcon(im->getWorkspaceThumbnail()), im->getFileName(), ui.recentWorkspaceListWidget);
        newItem->setForeground(QBrush(Qt::white));
    }
    ui.recentWorkspaceListWidget->setGridSize(QSize(200, 210));
    ui.recentWorkspaceListWidget->setIconSize(QSize(200, 200));
    connect(ui.recentWorkspaceListWidget, &QListWidget::itemDoubleClicked, this, &Entry_Widget::handleSelectHistoryWorkspace);
}

// block all signals that may try to open a new image
void Entry_Widget::blockAllSignals(bool block)
{
    ui.recentImageListWidget->blockSignals(block);
    ui.openImageButton->blockSignals(block);
}

void Entry_Widget::handleOpenImage()
{
    blockAllSignals(true);
    // load file directory
    QString filename = QFileDialog::getExistingDirectory(this, "CT file directory", "../");
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
        blockAllSignals(false);
        return;
    }

    // load the main window
    CT_Viewer* w = new CT_Viewer(ctImage, progressDialog);
    w->setAttribute(Qt::WA_DeleteOnClose);
    w->show();
    w->init2DViews();
    this->close();
}

void Entry_Widget::handleSelectHistoryImage(QListWidgetItem* item)
{
    blockAllSignals(true);
    RecentImageListWidgetItem* selectedItem = static_cast<RecentImageListWidgetItem*>(item);
    ImageRegister* selectedImage = selectedItem->getRecentImageInfo();

    QProgressDialog* progressDialog = createProgressDialog(tr("CT Loading"), tr("Loading CT, Please Wait..."), 101);
    CT_Image* ctImage = new CT_Image();
    ctImage->loadDicomFromDirectory(selectedImage->getFilePath(), progressDialog);

    // check the path is valid
    if (!ctImage->checkLoadSuccess()) {
        progressDialog->close();
        QMessageBox msgBox;
        msgBox.setText("No Dicom Files in the directory or Loading Failed!");
        msgBox.exec();
        delete ctImage;
        blockAllSignals(false);
        return;
    }

    CT_Viewer* w = new CT_Viewer(ctImage, progressDialog);
    w->loadSliceAndThreshold(selectedImage->getSliceCenter(), selectedImage->getContrastThreshold());
    w->loadCameraSettings(selectedImage->getCameraPos(), selectedImage->getFocalPoint());
    w->setAttribute(Qt::WA_DeleteOnClose);
    w->show();
    w->init2DViews();
    this->close();
    progressDialog->close();
    progressDialog->deleteLater();
}

void Entry_Widget::handleSelectHistoryWorkspace(QListWidgetItem* item)
{
    blockAllSignals(true);
    RecentImageListWidgetItem* selectedItem = static_cast<RecentImageListWidgetItem*>(item);
    ImageRegister* selectedImage = selectedItem->getRecentImageInfo();
    QProgressDialog* progressDialog = createProgressDialog(tr("CT Loading"), tr("Loading CT, Please Wait..."), 101);
    CT_Image* ctImage = new CT_Image();
    ctImage->loadDicomFromDirectory(selectedImage->getFilePath(), progressDialog);

    // check the path is valid
    if (!ctImage->checkLoadSuccess()) {
        progressDialog->close();
        QMessageBox msgBox;
        msgBox.setText("No DICOM Files in the directory or Loading Failed!");
        msgBox.exec();
        delete ctImage;
        blockAllSignals(false);
        return;
    }

    CT_Viewer* w = new CT_Viewer(ctImage, progressDialog);
    w->loadSliceAndThreshold(selectedImage->getSliceCenter(), selectedImage->getContrastThreshold());
    w->loadCameraSettings(selectedImage->getCameraPos(), selectedImage->getFocalPoint());
    w->loadScrews(selectedImage->getScrewList());
    w->setAttribute(Qt::WA_DeleteOnClose);
    w->show();
    w->init2DViews();
    this->close();
    progressDialog->close();
    progressDialog->deleteLater();
}
