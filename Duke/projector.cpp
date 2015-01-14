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
    cvNamedWindow("Projector Window",CV_WINDOW_NORMAL);
    cvResizeWindow("Projector Window",width,height);
    cvMoveWindow("Projector Window", xoffset, yoffset);
    //cvSetWindowProperty("Projector Window", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
}

void Projector::showImg(IplImage *img)
{
    cvShowImage("Projector Window", img );
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

QImage* Projector::IplImageToQPixmap(const IplImage *img)
{
    QImage *image;
    //cvCvtColor(img,img,CV_BGR2GRAY);
    uchar *imgData=(uchar *)img->imageData;
    image=new QImage(imgData,img->width,img->height,QImage::Format_Indexed8);
    return image;
    delete imgData;
}

IplImage* Projector::QImageToIplImage(const QImage *qImage)
{
    qImage->scaled(width,height);
    CvSize Size;
    Size.height = height;
    Size.width = width;
    IplImage *IplImageBuffer = cvCreateImage(Size, IPL_DEPTH_8U, 3);
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            QRgb rgb = qImage->pixel(x, y);
            cvSet2D(IplImageBuffer, y, x, CV_RGB(qRed(rgb), qGreen(rgb), qBlue(rgb)));
        }
    }
    return IplImageBuffer;
    cvReleaseImage(&IplImageBuffer);
}

void Projector::setCrossVisable(bool flag)
{
    crossVisible = flag;
    this->update();
}
