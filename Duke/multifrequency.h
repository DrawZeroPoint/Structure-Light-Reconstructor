#ifndef MULTIFREQUENCY_H
#define MULTIFREQUENCY_H

#define MULTI_MAX_NUM 12
#define PI 3.1416

#include <QObject>
#include <opencv/cv.h>
#include <opencv/highgui.h>

class MultiFrequency : public QObject
{
    Q_OBJECT
public:
    MultiFrequency(QObject *parent = 0, int projwidth = 1280, int projheight = 1024);
    void generateMutiFreq();
    int getNumOfImgs();
    cv::Mat getNextMultiFreq();

private:
    bool imgsLoaded;
    int projW;
    int projH;
    cv::Mat MultiFreqImages[MULTI_MAX_NUM];
};

#endif // MULTIFREQUENCY_H
