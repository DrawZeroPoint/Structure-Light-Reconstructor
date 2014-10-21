#include "projector.h"
#include <QLayout>
Projector::Projector(int projW, int projH, int scrnW)
{
    width = projW;
    height = projH;
    scrnwidth = scrnW;//main screen width
    pW = new QWidget();//projector window
    pW->move(scrnwidth,0);//make the window displayed by the projector
    pW->showFullScreen();
    imageLabel = new QLabel(pW);
}

Projector::~Projector()
{
}

void Projector::opencvWindow()
{
    cvNamedWindow("Projector Window",CV_WINDOW_NORMAL);
    cvResizeWindow("Projector Window",width,height);
    cvMoveWindow("Projector Window", scrnwidth, 0);
    cvSetWindowProperty("Projector Window", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
}

void Projector::showImg(IplImage *img)
{
    cvShowImage("Projector Window", img );
}

void Projector::showMatImg(cv::Mat img)
{
    //cv::imshow(pw, img );
}

void Projector::destoryWindow()
{
    cvDestroyWindow("Projector Window");
}

void Projector::displaySwitch(bool isWhite)
{
    pW->setAutoFillBackground(true);//necessary when using palette
    if(isWhite)
        pW->setPalette(Qt::white);//set the background color to be white, aka open the light
    else
        pW->setPalette(Qt::black);
}

QImage* Projector::IplImageToQPixmap(const IplImage *img)
{
    QImage *image;
    //cvCvtColor(img,img,CV_BGR2GRAY);
    uchar *imgData=(uchar *)img->imageData;
    image=new QImage(imgData,img->width,img->height,QImage::Format_Indexed8);
    return image;
    delete imgData;
    /*
    const uchar* imgData = (const uchar*)(img->imageData);
    QPixmap *qpix;
    qpix->loadFromData(imgData,8);
    return qpix;
    delete imgData;

    unsigned char* ptrQImage;
    if(img->nChannels==1){
        for(int row=0;row<img->height;row++){
            unsigned char* ptr=(unsigned char*)(img->imageData+row*img->widthStep);
            for(int col=0;col<img->width;col++){
                *(ptrQImage)=*(ptr+col);
                *(ptrQImage+1)=*(ptr+col);
                *(ptrQImage+2)=*(ptr+col);
                *(ptrQImage+3)=0;
                ptrQImage+=4;
            }
        }
    }
    */
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
