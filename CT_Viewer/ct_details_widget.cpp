#include "ct_details_widget.h"
#include <QDebug>

CT_Details_Widget::CT_Details_Widget(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);

    // set the table view
    ui.tableWidget->horizontalHeader()->setVisible(true);
    QStringList header;
    header << "Attribute" << "Value";
    ui.tableWidget->setHorizontalHeaderLabels(header);
    ui.tableWidget->verticalHeader()->setVisible(false);
    ui.tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui.tableWidget->setColumnWidth(0, 240);
    ui.tableWidget->setColumnWidth(1, 240);
    ui.tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    // connect close signal
    connect(ui.okButton, SIGNAL(clicked()), this, SLOT(handleOK()));
}

CT_Details_Widget::~CT_Details_Widget()
{
}

void CT_Details_Widget::setTableContent(QMap<QString, QString>& content)
{
    ui.tableWidget->setRowCount(content.count());
    int i = 0;
    QMap<QString, QString>::iterator iter = content.begin();
    while (iter != content.end())
    {
        ui.tableWidget->setItem(i, 0, new QTableWidgetItem(iter.key()));
        ui.tableWidget->setItem(i, 1, new QTableWidgetItem(iter.value()));
        i++;
        iter++;
    }
    ui.tableWidget->show();
}

void CT_Details_Widget::handleOK()
{
    this->close();
}