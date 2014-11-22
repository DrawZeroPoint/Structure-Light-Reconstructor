#ifndef PROJECTOR_H
#define PROJECTOR_H

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <QImage>
#include <QWidget>
#include <QLabel>
#include <QPicture>

#include <QMainWindow>

class Projector : public QWidget
{
public:
    Projector(QWidget *parent, int scanW, int scanH, int projW, int projH, int xos, int yos);
    ~Projector();
    void showImg(IplImage *img);
    void showMatImg(cv::Mat img);
    QImage *IplImageToQPixmap(const IplImage *img);
    IplImage* QImageToIplImage(const QImage *qImage);
    QLabel *imageLabel;//hold image.
    void displaySwitch(bool isWhite);
    void opencvWindow();
    void destoryWindow();//delete the projector window created by cv after showImg

protected:
        void paintEvent(QPaintEvent *event);

private:
    int xoffset;
    int yoffset;
    int height;
    int width;
};

#endif // PROJECTOR_H
