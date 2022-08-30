#pragma once

#include <QWidget>
#include "ui_entry_widget.h"
#include <QListWidgetItem>

class Entry_Widget : public QWidget
{
    Q_OBJECT

public:
    Entry_Widget(QWidget *parent = Q_NULLPTR);
    ~Entry_Widget();

private:
    Ui::Entry_Widget ui;
    void loadRecentImages();
    void loadRecentWorkspaces();
    void blockAllSignals(bool block);

private slots:
    void handleOpenImage();
    void handleSelectHistoryImage(QListWidgetItem* item);
    void handleSelectHistoryWorkspace(QListWidgetItem* item);
};
