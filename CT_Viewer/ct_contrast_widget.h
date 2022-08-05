#pragma once

#include <QWidget>
#include "ui_ct_contrast_widget.h"
#include "contrast_barchart_widget.h"
#include <vtkSmartPointer.h>
#include "ct_image.h"
#include <deque>

class CT_Contrast_Widget : public QWidget
{
    Q_OBJECT

public:
    CT_Contrast_Widget(CT_Image* ctImage, QWidget *parent = Q_NULLPTR);
    ~CT_Contrast_Widget();
    static bool first_init;

signals:
    void contrastAdjusted(int lower, int upper);
    void lineEditUpdated(int lower, int upper);

private:
    static int lower_val;
    static int upper_val;
    
    std::deque<int> hist;
    Ui::Ct_Contrast_Widget ui;
    void getHistogram(vtkSmartPointer<vtkImageAccumulate> imageAccumulate);
    QIntValidator* upperValidator;
    QIntValidator* lowerValidator;
    int histMin;
    int histMax;
    Contrast_Barchart_Widget* barWidget;
    CT_Image* ctImage;

private slots:
    void closeWindow();
    void updateUpperAndLower(int lower, int upper);
    void handleLineInput();
    void handleReset();
    void handleLogBox(int state);
};
