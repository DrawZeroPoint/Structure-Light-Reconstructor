#ifndef VIRTUALCAMERA_H
#define VIRTUALCAMERA_H

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <QString>

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>

class VirtualCamera
{
public:
    VirtualCamera();
    ~VirtualCamera();
    void loadDistortion(QString path);
    bool loadCameraMatrix(QString path);
    void loadRotationMatrix(QString path);
    void loadTranslationVector(QString path);
    cv::Mat distortion;
    cv::Mat rotationMatrix;
    cv::Mat translationVector;
    cv::Point3f position;//相机的三维坐标原点，在runReconstruction函数中赋值为(0,0,0)
    cv::Point2f fc;
    cv::Point2f cc;
    int width;
    int height;

private:
    int loadMatrix(cv::Mat &matrix, int s1, int s2 , std::string file);
};

#endif // VIRTUALCAMERA_H
