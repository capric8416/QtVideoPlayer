#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_QtVideoPlayer.h"

class QtVideoPlayer : public QMainWindow
{
    Q_OBJECT

public:
    QtVideoPlayer(QWidget *parent = Q_NULLPTR);

private:
    Ui::QtVideoPlayerClass ui;
};
