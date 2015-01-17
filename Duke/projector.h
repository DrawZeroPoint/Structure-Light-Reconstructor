#ifndef PROJECTOR_H
#define PROJECTOR_H

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <QImage>
#include <QWidget>
#include <QLabel>
#include <QPicture>
#include <QPainter>

#include <QMainWindow>

class Projector : public QWidget
{
public:
    Projector(QWidget *parent, int scanW, int scanH, int projW, int projH, int xos, int yos);
    ~Projector();
    void showMatImg(cv::Mat img);

    void displaySwitch(bool isWhite);
    void opencvWindow();
    void destoryWindow();//delete the projector window created by cv after showImg

    void setCrossVisable(bool flag);
    void paintEvent(QPaintEvent *event);

private:
    bool crossVisible;
    int xoffset;
    int yoffset;
    int height;
    int width;
};

#endif // PROJECTOR_H
