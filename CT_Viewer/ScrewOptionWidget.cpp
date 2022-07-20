#include "ScrewOptionWidget.h"

ScrewOptionWidget::ScrewOptionWidget(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    
    connect(ui.okButton, SIGNAL(clicked()), this, SLOT(updateSelectModel()));
    connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(closeWindow()));
}

char* ScrewOptionWidget::getSelectModel()
{
    return this->selectedModel;
}

bool ScrewOptionWidget::confirmAction()
{
    return this->confirmAdd;
}

void ScrewOptionWidget::closeWindow()
{
    this->confirmAdd = false;
    this->close();
}

void ScrewOptionWidget::updateSelectModel()
{
    this->confirmAdd = true;
    strcpy(this->selectedModel, ui.comboBox->currentText().toStdString().c_str());
    this->close();
}