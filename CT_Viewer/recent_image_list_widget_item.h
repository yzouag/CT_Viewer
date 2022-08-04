#pragma once

#include <QListWidgetItem>
#include "image_register.h"
class RecentImageListWidgetItem : public QListWidgetItem
{
public:
    RecentImageListWidgetItem(ImageRegister* recentImageInfo, QIcon &icon, const QString &text, QListWidget *parent = nullptr, int type = Type);
    ~RecentImageListWidgetItem();
    bool operator< (const QListWidgetItem &other) const override;
    ImageRegister* getRecentImageInfo();

private:
    ImageRegister* recentImageInfo;
};

