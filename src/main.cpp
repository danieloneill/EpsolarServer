#include <QCoreApplication>
#include <QSettings>

#include "controller.h"
#ifdef HTTP
# include "resourceserver.h"
#endif

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QSettings settings(SETTINGS, QSettings::IniFormat);

#ifdef HTTP
    ResourceServer r(nullptr);
#endif

    Controller c(&settings, nullptr);

    return a.exec();
}
