#pragma once

#include "ui_screwOptionWidget.h"
#include <QDialog>
class ScrewOptionWidget : public QDialog
{
    Q_OBJECT
public:
    ScrewOptionWidget(QWidget *parent = nullptr);
    int getSelectModel();
    bool confirmAction();

private:
    Ui::Dialog ui;
    int selectedModel = 0;
    bool confirmAdd;

private slots:
    void updateSelectModel();
    void closeWindow();
};

