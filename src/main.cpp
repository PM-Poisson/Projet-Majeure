#include <QApplication>

#include "UI/myWindow.hpp"
#include "lib/interface/application_qt.hpp"

int main(int argc, char *argv[])
{
    application_qt app(argc, argv);

    myWindow win;
    win.show();

    return app.exec();
}
