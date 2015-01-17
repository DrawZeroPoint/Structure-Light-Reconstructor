#ifndef DAHENGCAMERA_H
#define DAHENGCAMERA_H

#include <QObject>

#include "Windows.h"//加载此头文件以解决大恒相机头文件类型未定义问题
#include <HVDAILT.h>
#include <Raw2Rgb.h>

class DaHengCamera : public QObject
{
    Q_OBJECT
public:
    DaHengCamera(QObject *parent = 0);
    ~DaHengCamera();

    bool cameraOpened;

    void daHengExposure(int leftexposure, int rightexposure);
    void openDaHengCamera(int camerawidth, int cameraheight);
    void daHengSnapShot(int camNo);
    void closeCamera();

    BYTE *m_pRawBuffer_1;
    BYTE *m_pRawBuffer_2;

private:
    HHV	m_hhv_1;
    HHV	m_hhv_2;

    int cam_w;
    int cam_h;

    static int CALLBACK SnapThreadCallback(HV_SNAP_INFO *pInfo);

    ///---------------相机相关函数---------------///
    void OnSnapexOpen();
    void OnSnapexStart();
    void OnSnapexStop();
    void OnSnapexClose();

};

#endif // DAHENGCAMERA_H
