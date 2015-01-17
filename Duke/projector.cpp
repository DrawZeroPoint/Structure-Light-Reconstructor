#include "projector.h"
#include <QLayout>
#include <QEvent>
#include <QPaintEvent>

int proj_w;
int proj_h;

Projector::Projector(QWidget *parent, int scanW, int scanH, int projW, int projH, int xos, int yos)
    : QWidget(parent)
{
    width = scanW;
    height = scanH;
    proj_w = projW;
    proj_h = projH;
    xoffset = xos;
    yoffset = yos;
    crossVisible = true;
}

Projector::~Projector()
{
}

void Projector::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    if(crossVisible)//QEvent::User 1000
    {
        painter.setPen(QPen(Qt::yellow, 5));
    //else
        //painter.setPen(QPen(Qt::white, 5));
        painter.drawLine(proj_w/2 - 60, proj_h/2, proj_w/2 + 60, proj_h/2);
        painter.drawLine(proj_w/2, proj_h/2 - 60, proj_w/2, proj_h/2 + 60);
    }
}

void Projector::opencvWindow()
{
    cvNamedWindow("Projector Window",CV_WINDOW_AUTOSIZE|CV_WINDOW_KEEPRATIO|CV_WINDOW_NORMAL);
    cvResizeWindow("Projector Window",width,height);
    cvMoveWindow("Projector Window", xoffset, yoffset);
}

void Projector::showMatImg(cv::Mat img)
{
    cv::imshow("Projector Window", img);
}

void Projector::destoryWindow()
{
    cvDestroyWindow("Projector Window");
}

void Projector::displaySwitch(bool isWhite)
{
    if(isWhite)
        this->setPalette(Qt::white);
    else
        this->setPalette(Qt::black);
}

void Projector::setCrossVisable(bool flag)
{
    crossVisible = flag;
    this->update();
}
