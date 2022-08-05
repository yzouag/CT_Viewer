#include "CT_Viewer.h"
#include "entry_widget.h"
#include "uiHelper.h"
#include "ct_image.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include "image_register.h"
#include "recent_image_list_widget_item.h"

Entry_Widget::Entry_Widget(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);

    connect(ui.openImageButton, SIGNAL(clicked()), this, SLOT(handleOpenImage()));
    connect(ui.openWorkspaceButton, SIGNAL(clicked()), this, SLOT(handleOpenWorkspace()));

    ui.tabWidget->setCurrentWidget(ui.tab);
    loadRecentImages();
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
        RecentImageListWidgetItem* newItem = new RecentImageListWidgetItem(im, QIcon(im->getThumbnail()), im->getFileName(), ui.recentImageListWidget);
        newItem->setForeground(QBrush(Qt::white));
    }
    ui.recentImageListWidget->setGridSize(QSize(200, 210));
    ui.recentImageListWidget->setIconSize(QSize(200, 200));

    connect(ui.recentImageListWidget, &QListWidget::itemDoubleClicked, this, &Entry_Widget::handleSelectHistoryImage);
}

// block all signals that may try to open a new image
void Entry_Widget::blockAllSignals()
{
    ui.recentImageListWidget->blockSignals(true);
    ui.openImageButton->blockSignals(true);
    ui.openWorkspaceButton->blockSignals(true);
}

void Entry_Widget::handleOpenImage()
{
    blockAllSignals();
    // load file directory
    QString filename = QFileDialog::getExistingDirectory(this, "CT file directory", "../");
    QProgressDialog* progressDialog = createProgressDialog(tr("CT Loading"), tr("Loading CT, Please Wait..."), 100);
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

    // load the main window
    CT_Viewer* w = new CT_Viewer(ctImage);
    w->setAttribute(Qt::WA_DeleteOnClose);
    w->show();
    w->init2DViews();
    this->close();
}

void Entry_Widget::handleOpenWorkspace()
{

}

void Entry_Widget::handleSelectHistoryImage(QListWidgetItem* item)
{
    blockAllSignals();
    RecentImageListWidgetItem* selectedItem = static_cast<RecentImageListWidgetItem*>(item);
    ImageRegister* selectedImage = selectedItem->getRecentImageInfo();

    QProgressDialog* progressDialog = createProgressDialog(tr("CT Loading"), tr("Loading CT, Please Wait..."), 100);
    CT_Image* ctImage = new CT_Image();
    ctImage->loadDicomFromDirectory(selectedImage->getFilePath(), progressDialog);

    CT_Viewer* w = new CT_Viewer(ctImage);
    w->loadSliceAndThreshold(selectedImage->getSliceCenter(), selectedImage->getContrastThreshold());
    w->loadCameraSettings(selectedImage->getCameraPos(), selectedImage->getFocalPoint());
    w->setAttribute(Qt::WA_DeleteOnClose);
    w->show();
    w->init2DViews();
    this->close();
}
