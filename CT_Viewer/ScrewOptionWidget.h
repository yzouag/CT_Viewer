#pragma once

#include "ui_screwOptionWidget.h"
#include <QDialog>
class ScrewOptionWidget : public QDialog
{
    Q_OBJECT
public:
    ScrewOptionWidget(QWidget *parent = nullptr);
    char* getSelectModel();
    bool confirmAction();

private:
    Ui::Dialog ui;
    char selectedModel[30];
    bool confirmAdd;

private slots:
    void updateSelectModel();
    void closeWindow();
};

