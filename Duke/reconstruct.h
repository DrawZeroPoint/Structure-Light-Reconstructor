#ifndef RECONSTRUCT_H
#define RECONSTRUCT_H

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include "graycodes.h"
#include "virtualcamera.h"
#include "pointcloudimage.h"
#include "set.h"
#include "stereorect.h"

//#define USE_STEREOCALIB_DATA

class Reconstruct
{
public:
    Reconstruct(bool useEpi);
    ~Reconstruct();

    bool loadCameras();

    bool runReconstruction();
    bool runReconstruction_GE();

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

    void getParameters(int scanw, int scanh,  int camw, int camh, bool autocontrast, QString savePath);
    QString savePath_;//same as projectPath
    int scanSN;//表示当前重建的扫描数据序列号，也是输出模型的序列号

private:
    bool EPI;//是否使用极线校正
    int numOfCams;
    VirtualCamera *camera;//general functions use this instead of camera1 or camera2
    stereoRect *sr;

    cv::vector<cv::Point> **camsPixels;
    cv::vector<int> **camsPixels_GE;
    cv::vector<cv::Point> *camPixels; //general functions use this instead of cam1Pixels or cam2Pixels
    cv::vector<int> *camPixels_GE;

    bool loadCamImgs(QString folder, QString prefix, QString suffix);

    void unloadCamImgs();//对不同编码模式都适用的卸载图像方法
    void computeShadows();

    ///不同编码模式对应的图像点身份获取方法
    bool getProjPixel(int row, int col, cv::Point &p_out);//GRAY_ONLY
    bool getProjPixel_GE(int row, int col, int &xDec);//GRAY_EPI

    void decodePaterns();
    void decodePatterns_GE();

    void triangulation(cv::vector<cv::Point> *cam1Pixels, VirtualCamera cameras1, cv::vector<cv::Point> *cam2Pixels, VirtualCamera cameras2);
    void triangulation_ge(cv::vector<int> *cam1Pixels, VirtualCamera camera1, cv::vector<int> *cam2Pixels, VirtualCamera camera2);

    QString *scanFolder;
    QString *imgPrefix;
    QString imgSuffix;
    int numberOfImgs;
    int numOfColBits;
    int numOfRowBits;
    int blackThreshold;
    int whiteThreshold;

    cv::vector<cv::Mat> camImgs;//用来存放条纹图像序列，不同编码方式通用

    cv::Mat mask;//matrix with vals 0 and 1 , CV_8U , uchar
    cv::Mat decRows;
    cv::Mat decCols;
    bool pathSet;
    bool autoContrast_;
    bool raySampling_;
    int cameraWidth;
    int cameraHeight;

    //access
    int Reconstruct::ac(int x,int y)
    {
        return x*scan_h + y;
    }

    int scan_w;
    int scan_h;
};

#endif // RECONSTRUCT_H
