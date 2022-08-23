#include "actor_list_item.h"
#include <QColorDialog>
#include <QMessageBox>
#include <vtkProperty.h>
#include <QDebug>

ActorListItem::ActorListItem(ActorType actorType, QString actorName, QListWidgetItem* item, QWidget *parent)
    : QWidget(parent)
{
    this->actorName = actorName;
    this->item = item;
    ui.setupUi(this);

    // set the icon, actor name, and the actor color
    ui.label->setText(actorName);
    QPixmap pixmap(16, 16);
    switch (actorType) {
    case DICOM_IMAGE:
        ui.icon->setPixmap(QPixmap(QString::fromUtf8(":/CT_Viewer/resources/bone.png")));
        pixmap.fill(QColor(255, 255, 255));
        ui.colorButton->setDisabled(true); // no color change for vtkVolume
        break;
    case CONE:
        ui.icon->setPixmap(QPixmap(QString::fromUtf8(":/CT_Viewer/resources/cone.png")));
        pixmap.fill(QColor(0, 206, 209));
        break;
    case SCREW:
        ui.icon->setPixmap(QPixmap(QString::fromUtf8(":/CT_Viewer/resources/screw.png")));
        pixmap.fill(QColor(255, 255, 255));
        break;
    case DISTANCE:
        ui.icon->setPixmap(QPixmap(QString::fromUtf8(":/CT_Viewer/resources/measure_length.png")));
        pixmap.fill(QColor(255, 0, 0));
        break;
    case ANGLE:
        ui.icon->setPixmap(QPixmap(QString::fromUtf8(":/CT_Viewer/resources/measure_angle.png")));
        pixmap.fill(QColor(255, 0, 0));
        break;
    }
    ui.colorButton->setIcon(pixmap);

    // connect button event
    connect(ui.hideButton, SIGNAL(clicked()), this, SLOT(onHideButtonToggled()));
    connect(ui.colorButton, SIGNAL(clicked()), this, SLOT(onColorButtonClicked()));
    connect(ui.deleteButton, SIGNAL(clicked()), this, SLOT(onDeleteButtonClicked()));
}

ActorListItem::~ActorListItem()
{

}

QString ActorListItem::getActorName()
{
    return this->actorName;
}

void ActorListItem::setCorrespondingActors(QList<vtkProp*> correspondingActors)
{
    this->correspondingActors = correspondingActors;
}

void ActorListItem::setCorrespondingBoxWidget(vtkBoxWidget * widget)
{
    this->widget = widget;
}

void ActorListItem::onHideButtonToggled()
{
    bool checked = ui.hideButton->isChecked();
    if (checked) {
        ui.hideButton->setIcon(QIcon(QString::fromUtf8(":/CT_Viewer/resources/hide.png")));
        if (this->widget != nullptr) {
            this->widget->SetEnabled(false);
        }
        for each (auto actor in this->correspondingActors)
        {
            actor->VisibilityOff();
        }
    }
    else {
        ui.hideButton->setIcon(QIcon(QString::fromUtf8(":/CT_Viewer/resources/see.png")));
        if (this->widget != nullptr) {
            this->widget->SetEnabled(true);
        }
        for each (auto actor in this->correspondingActors)
        {
            actor->VisibilityOn();
        }
    }
    emit actorChanged();
}

void ActorListItem::onColorButtonClicked()
{
    const QColor color = QColorDialog::getColor(Qt::green, this, "Select Color");

    if (color.isValid()) {
        QPixmap pixmap(16, 16);
        pixmap.fill(color);
        ui.colorButton->setIcon(pixmap);
        for each (auto actor in this->correspondingActors)
        {
            static_cast<vtkActor*>(actor)->GetProperty()->SetColor(color.red() / 256.0, color.green() / 256.0, color.blue() / 256.0);
        }
        emit colorChanged(this->widget, color.red() / 256.0, color.green() / 256.0, color.blue() / 256.0);
    }
}

void ActorListItem::onDeleteButtonClicked()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Confirmation", "Are you sure to delete this actor?", QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        if (this->widget != nullptr) {
            this->widget->SetEnabled(false);
        }
        for each (auto actor in this->correspondingActors)
        {
            actor->VisibilityOff();
        }
        emit actorChanged();
        emit widgetDeleted(this, item);
    }
}