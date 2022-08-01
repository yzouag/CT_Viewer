#include "CT_Viewer.h"
#include "entry_widget.h"
#include "uiHelper.h"
#include "ct_image.h"
#include <QFileDialog>
#include <QMessageBox>

Entry_Widget::Entry_Widget(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);

    connect(ui.openImageButton, SIGNAL(clicked()), this, SLOT(handleOpenImage()));
    connect(ui.openWorkspaceButton, SIGNAL(clicked()), this, SLOT(handleOpenWorkspace()));
}

Entry_Widget::~Entry_Widget()
{
}

void Entry_Widget::handleOpenImage()
{
    // load file directory
    QString filename = QFileDialog::getExistingDirectory(this, "CT file directory", "../");
    QProgressDialog* progressDialog = createProgressDialog(tr("CT Loading"), tr("Loading CT, Please Wait..."), 100);
    CT_Image* ctImage = new CT_Image();
    ctImage->loadDicomFromDirectory(filename.toStdString().c_str(), progressDialog);
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
