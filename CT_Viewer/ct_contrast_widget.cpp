#include "ct_contrast_widget.h"
#include <vtkImageData.h>
#include <vtkImageAccumulate.h>
#include <QDebug>

int CT_Contrast_Widget::lower_val = 0;
int CT_Contrast_Widget::upper_val = 0;
bool CT_Contrast_Widget::first_init = true;

CT_Contrast_Widget::CT_Contrast_Widget(CT_Image* ctImage, QWidget *parent) : QWidget(parent, Qt::Window)
{
    ui.setupUi(this);

    // process image to get the histogram
    this->ctImage = ctImage;
    getHistogram(ctImage->getCTImageAccumulate());
    
    // create the bar chart, add it to widget and get the layout width, height
    this->barWidget = new Contrast_Barchart_Widget(&this->hist, this->histMin, this->histMax, this);
    ui.horizontalLayout->addWidget(this->barWidget);
    if (first_init) {
        this->barWidget->initPoints(ui.horizontalLayoutWidget->geometry().height(), ui.horizontalLayoutWidget->geometry().width(), -1000, 1000);
        first_init = false;
    }
    else {
        this->barWidget->initPoints(ui.horizontalLayoutWidget->geometry().height(), ui.horizontalLayoutWidget->geometry().width(), lower_val, upper_val);
    }
    // limit the input to histMin and histMax
    this->upperValidator = new QIntValidator(histMin, histMax, this);
    this->lowerValidator = new QIntValidator(histMin, histMax, this);
    ui.lowerEdit->setText(QString::number(this->histMin));
    ui.upperEdit->setText(QString::number(this->histMax));

    ui.upperEdit->setValidator(this->upperValidator);
    ui.lowerEdit->setValidator(this->lowerValidator);

    // connect UI widgets with actions
    connect(ui.closeButton, SIGNAL(clicked()), this, SLOT(closeWindow()));
    connect(this->barWidget, &Contrast_Barchart_Widget::thresholdUpdated, this, &CT_Contrast_Widget::updateUpperAndLower);
    connect(ui.lowerEdit, SIGNAL(editingFinished()), this, SLOT(handleLineInput()));
    connect(ui.upperEdit, SIGNAL(editingFinished()), this, SLOT(handleLineInput()));
    connect(this, &CT_Contrast_Widget::lineEditUpdated, this->barWidget, &Contrast_Barchart_Widget::updatePoints);
    connect(ui.resetButton, SIGNAL(clicked()), this, SLOT(handleReset()));
    connect(ui.logBox, SIGNAL(stateChanged(int)), this, SLOT(handleLogBox(int)));
}

CT_Contrast_Widget::~CT_Contrast_Widget()
{
}

void CT_Contrast_Widget::getHistogram(vtkSmartPointer<vtkImageAccumulate> imageAccumulate)
{
    // transfer the data in imageAccumulate to the deque
    int n_bins = imageAccumulate->GetComponentExtent()[1];
    for (vtkIdType bin = 0; bin < n_bins; ++bin) {
        hist.push_back(*(static_cast<int*>(imageAccumulate->GetOutput()->GetScalarPointer(bin, 0, 0))));
    }

    // get min and max intensity of the picture and set them
    this->histMax = imageAccumulate->GetMax()[0];
    this->histMin = imageAccumulate->GetMin()[0];
    ui.minEdit->setText(QString::number(this->histMin));
    ui.maxEdit->setText(QString::number(this->histMax));
}

void CT_Contrast_Widget::closeWindow()
{
    lower_val = ui.lowerEdit->text().toInt();
    upper_val = ui.upperEdit->text().toInt();
    this->close();
}

void CT_Contrast_Widget::updateUpperAndLower(int lower, int upper)
{
    // change the line edit since bar chart is moved
    ui.upperEdit->setText(QString::number(upper));
    ui.lowerEdit->setText(QString::number(lower));

    // update the validator
    this->lowerValidator->setTop(upper);
    this->upperValidator->setBottom(lower);

    // send signal to CT image for update
    this->ctImage->updateContrastThreshold(lower, upper);
}

void CT_Contrast_Widget::handleLineInput()
{
    // read the input
    int lower = ui.lowerEdit->text().toInt();
    int upper = ui.upperEdit->text().toInt();

    // update the validator
    this->lowerValidator->setTop(lower);
    this->upperValidator->setBottom(upper);

    // send signal to bar chart and CT image
    this->ctImage->updateContrastThreshold(lower, upper);
    emit lineEditUpdated(lower, upper);
}

void CT_Contrast_Widget::handleReset()
{
    this->barWidget->reset();
}

void CT_Contrast_Widget::handleLogBox(int state)
{
    if (state == Qt::Checked) {
        this->barWidget->setLogScale(true);
    }
    else {
        this->barWidget->setLogScale(false);
    }
}
