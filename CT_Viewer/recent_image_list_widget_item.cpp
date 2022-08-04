#include "recent_image_list_widget_item.h"
#include <QDateTime>

RecentImageListWidgetItem::RecentImageListWidgetItem(ImageRegister* recentImageInfo, QIcon &icon, const QString &text, QListWidget *parent /*= nullptr*/, int type /*= Type*/)
    : QListWidgetItem(icon, text, parent, type)
{
    this->recentImageInfo = recentImageInfo;
    this->setData(Qt::UserRole, recentImageInfo->getCreatedTime());
}

RecentImageListWidgetItem::~RecentImageListWidgetItem()
{
    delete this->recentImageInfo;
}

bool RecentImageListWidgetItem::operator<(const QListWidgetItem & other) const
{
    QDateTime this_time = QDateTime::fromString(this->recentImageInfo->getCreatedTime(), "yyyyMMdd hh:mm:ss");
    QDateTime that_time = QDateTime::fromString(other.data(Qt::UserRole).toString(), "yyyyMMdd hh:mm:ss");
    return this_time > that_time;
}

ImageRegister * RecentImageListWidgetItem::getRecentImageInfo()
{
    return this->recentImageInfo;
}
