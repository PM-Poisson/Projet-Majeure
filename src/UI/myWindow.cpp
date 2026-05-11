#include "myWindow.hpp"
#include "myWidgetGL.hpp"
#include "../lib/common/error_handling.hpp"
#include "ui_mainwindow.h"

#include <iostream>

myWindow::myWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    try
    {
        ui->setupUi(this);

        QGLFormat qglFormat;
        qglFormat.setVersion(1, 2);

        glWidget = new myWidgetGL(qglFormat);
        ui->layout_scene->addWidget(glWidget);
    }
    catch(cpe::exception_cpe const& e)
    {
        std::cout << e.report_exception() << std::endl;
    }

    connect(ui->quit,      SIGNAL(clicked()), this, SLOT(action_quit()));
    connect(ui->draw,      SIGNAL(clicked()), this, SLOT(action_draw()));
    connect(ui->wireframe, SIGNAL(clicked()), this, SLOT(action_wireframe()));
}

myWindow::~myWindow()
{}

void myWindow::action_quit()      { close(); }
void myWindow::action_draw()      { glWidget->change_draw_state(); }
void myWindow::action_wireframe() { glWidget->wireframe(ui->wireframe->isChecked()); }
