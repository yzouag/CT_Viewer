#include "entry_widget.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Entry_Widget* w = new Entry_Widget();
    w->setAttribute(Qt::WA_DeleteOnClose);
    w->show();
    return a.exec();
}
