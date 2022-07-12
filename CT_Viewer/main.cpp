#include "CT_Viewer.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CT_Viewer w;
    w.show();
    return a.exec();
}
