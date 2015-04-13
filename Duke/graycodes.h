#ifndef GRAYCODES_H
#define GRAYCODES_H

#include <iostream>
#include <fstream>
using std::ofstream;
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <math.h>
#include "utilities.h"

#define GRAY_MAX_NUM 44

class GrayCodes
{
public:
    GrayCodes(int scanW, int scanH);
    ~GrayCodes();

    cv::Mat grayCodes[GRAY_MAX_NUM];

    int getNumOfImgs();

    void generateGrays();

    static int grayToDec(cv::vector<bool> gray);
    int getNumOfRowBits();
    int getNumOfColBits();

protected:

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
