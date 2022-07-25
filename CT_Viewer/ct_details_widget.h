#pragma once

#include <QWidget>
#include "ui_ct_details_widget.h"

class CT_Details_Widget : public QWidget
{
    Q_OBJECT

public:
    CT_Details_Widget(QWidget *parent = Q_NULLPTR);
    ~CT_Details_Widget();
    void setTableContent(QMap<QString, QString>& content);

private:
    Ui::Form ui;

private slots:
    void handleOK();
};
