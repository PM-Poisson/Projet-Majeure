#pragma once

#ifndef MY_WINDOW_HPP
#define MY_WINDOW_HPP

#include <QMainWindow>

namespace Ui { class MainWindow; }
class myWidgetGL;

class myWindow : public QMainWindow
{
    Q_OBJECT

public:
    myWindow(QWidget *parent = NULL);
    ~myWindow();

private slots:
    void action_quit();
    void action_draw();
    void action_wireframe();

private:
    Ui::MainWindow *ui;
    myWidgetGL *glWidget;
};

#endif
