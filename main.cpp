#include <QApplication>
#include "ErodedBankScene.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    ErodedBankScene scene;
    scene.setWindowTitle("Berge Érodée — Hydraulic Erosion Scene");
    scene.resize(1200, 700);
    scene.show();
    return app.exec();
}
