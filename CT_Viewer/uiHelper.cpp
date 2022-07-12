#include "uiHelper.h"

void displayMetaInfo(Ui::CT_ViewerClass & ui, QMap<QString, QString>& map)
{
    ui.name_label->setText(map["Patient Name"]);
    QString date = map["Instance Creation Date"];
    QString dateDisplay = date.left(4) + '.' + date.right(4).left(2) + '.' + date.right(2);
    ui.date_label->setText(dateDisplay);
    ui.descrip_label->setText(map["Study Description"]);
}

QString displayDetails(QMap<QString, QString>& map)
{
    QMap<QString, QString>::iterator iter = map.begin();
    QString details = "";
    while (iter != map.end()) {
        details.append(iter.key() + ": ");
        if (iter.value().size() < 1) {
            details.append("null\n");
        } else
            details.append(iter.value() + "\n");
        iter++;
    }
    return details;
}
