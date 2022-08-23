#pragma once

#include <QWidget>
#include "ui_actor_list_item.h"
#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <QListWidgetItem>
#include <vtkProp.h>
#include <vtkBoxWidget.h>

// TODO: rename the object

enum ActorType
{
    DICOM_IMAGE, CONE, SCREW, DISTANCE, ANGLE
};

class ActorListItem : public QWidget
{
    Q_OBJECT

public:
    ActorListItem(ActorType actorType, QString actorName, QListWidgetItem* item, QWidget *parent = Q_NULLPTR);
    ~ActorListItem();
    QString getActorName();
    void setCorrespondingActors(QList<vtkProp*> correspondingActors);
    void setCorrespondingBoxWidget(vtkBoxWidget* widget);

signals:
    void actorChanged();
    void widgetDeleted(QWidget* widget, QListWidgetItem* item);
    void colorChanged(vtkBoxWidget* widget, double r, double g, double b);

private:
    Ui::ActorListItem ui;
    QString actorName;
    QList<vtkProp*> correspondingActors;
    vtkBoxWidget* widget = nullptr;
    QListWidgetItem* item;

private slots:
    void onHideButtonToggled();
    void onColorButtonClicked();
    void onDeleteButtonClicked();
};
