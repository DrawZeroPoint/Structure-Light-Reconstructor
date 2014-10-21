#ifndef CAMERA_H
#define CAMERA_H

#include <QWidget>
#include <QLabel>
#include "Windows.h"//加载此头文件以解决大恒相机头文件类型未定义问题
#include <HVDAILT.h>
#include <Raw2Rgb.h>

#define WM_SNAP_CHANGE (WM_USER + 100)

class Camera : public QWidget
{
    Q_OBJECT
public:
    Camera();
    ~Camera();

    BYTE *rawBuffer_1;
    BYTE *rawBuffer_2;///< 采集图像原始数据缓冲区
    QImage *image_1;
    QImage *image_2;

    int Width;
    int Height;
    BOOL m_bOpen;		///< 初始化标志

    void OnSnapexOpen();
    void OnSnapexStart();
    void OnSnapexStop();
    void OnSnapexClose();
    int OnSnapChange();
private slots:
    void CaptureFrame();
private:
    HHV camera_1;
    HHV camera_2;///< 数字摄像机句柄

    BOOL m_bStart;		///< 启动标志
    long m_lHBlanking;	///< 水平消隐
    long m_lVBlanking;	///< 垂直消隐
    BITMAPINFO *m_pBmpInfo;	///< BITMAPINFO 结构指针，显示图像时使用

    BYTE *m_pImageBuffer;	///< Bayer转换后缓冲区
    char m_chBmpBuf[2048];	///< BIMTAPINFO 存储缓冲区，m_pBmpInfo即指向此缓冲区
    /// 采集回调函数，用户也可以定义为全局函数，如果作为类的成员函数，必须为静态成员函数。
    static int CALLBACK SnapThreadCallback(HV_SNAP_INFO *pInfo);
    /// 设置曝光时间
    HVSTATUS SetExposureTime(int nWindWidth, long lTintUpper, long lTintLower, long HBlanking, HV_SNAP_SPEED SnapSpeed, HV_RESOLUTION Resolution);
};

#endif // CAMERA_H
