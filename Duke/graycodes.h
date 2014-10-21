#ifndef GRAYCODES_H
#define GRAYCODES_H

#include <iostream>
#include <fstream>
using std::ofstream;
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <math.h>
#include "utilities.h"

#define GRAY_MAX_NUM 100

class GrayCodes
{
public:
    GrayCodes(int projW, int projH);
    ~GrayCodes();

    void unload();
    int getNumOfImgs();
    IplImage* getNextImg();
    IplImage* getImg(int num);

    void generateGrays();

    void save();
    static int grayToDec(cv::vector<bool> gray);
    int GrayCodes::getNumOfRowBits();
    int GrayCodes::getNumOfColBits();

protected:
    IplImage* grayCodes[GRAY_MAX_NUM];
    IplImage* colorCodes[GRAY_MAX_NUM];

    void calNumOfImgs();
    void allocMemForImgs();

    bool imgsLoaded;
    int numOfImgs;
    int numOfRowImgs;
    int numOfColImgs;
    int currentImgNum;
    int height;
    int width;
};

#endif // GRAYCODES_H
