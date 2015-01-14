#ifndef RECONSTRUCT_H
#define RECONSTRUCT_H

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include "graycodes.h"
#include "virtualcamera.h"
#include "pointcloudimage.h"
#include "set.h"

#define USE_STEREOCALIB_DATA

class Reconstruct
{
public:
    Reconstruct();
    ~Reconstruct();

    bool loadCameras();
    bool runReconstruction();
    VirtualCamera	*cameras;
    QString *calibFolder;
    PointCloudImage *points3DProjView;
    void setBlackThreshold(int val);
    void setWhiteThreshold(int val);
    void setCalibPath(QString path1st, int cam_no );
    void saveShadowImg(const char path[]);
    void saveDecodedRowImg(const char path[]);
    void saveDecodedColImg(const char path[]);

    void enableRaySampling();
    void disableRaySampling();

    void cam2WorldSpace(VirtualCamera cam, cv::Point3f &p);

    void getParameters(int scanw, int scanh,  int camw, int camh, int scanSN, bool autocontrast, QString savePath);
    QString savePath_;//same as projectPath

private:
    int numOfCams;
    VirtualCamera *camera;//general functions use this instead of camera1 or camera2
    cv::vector<cv::Point> **camsPixels;
    cv::vector<cv::Point> *camPixels; //general functions use this instead of cam1Pixels or cam2Pixels
    bool Reconstruct::loadCamImgs(QString folder, QString prefix, QString suffix);
    void unloadCamImgs();
    void computeShadows();

    bool Reconstruct::getProjPixel(int row, int col, cv::Point &p_out);
    void decodePaterns();
    void Reconstruct::triangulation(cv::vector<cv::Point> *cam1Pixels, VirtualCamera cameras1, cv::vector<cv::Point> *cam2Pixels, VirtualCamera cameras2, int cam1index, int cam2index);

    QString *scanFolder;
    QString *imgPrefix;
    QString imgSuffix;
    int numberOfImgs;
    int numOfColBits;
    int numOfRowBits;
    int blackThreshold;
    int whiteThreshold;
    cv::vector<cv::Mat> camImgs;
    cv::vector<cv::Mat> colorImgs;
    cv::Mat color;
    cv::Mat mask;					//matrix with vals 0 and 1 , CV_8U , uchar
    cv::Mat decRows;
    cv::Mat decCols;
    bool pathSet;
    bool autoContrast_;
    bool raySampling_;
    int cameraWidth;
    int cameraHeight;
    int scanSN_;//表示当前重建的扫描数据序列号，也是输出模型的序列号

    //access
    int Reconstruct::ac(int x,int y)
    {
        return x*proj_h + y;
    }

    int proj_w;
    int proj_h;
};

#endif // RECONSTRUCT_H
