#include "camera.h"
#include <QImage>
#include <QTimer>

#define  MY_ZERO 0.000000001

const HV_RESOLUTION Resolution = RES_MODE0;
const HV_SNAP_MODE SnapMode = CONTINUATION;
const HV_BAYER_CONVERT_TYPE ConvertType = BAYER2RGB_NEIGHBOUR1;

const long Gain = 10;
const long ExposureTint_Upper = 60;
const long ExposureTint_Lower = 1000;
const long ShutterDelay       = 0;
const long ADCLevel           = ADC_LEVEL2;
const int XStart              = 0;
const int YStart              = 0;
const long lVBlanking         = 0;
const HV_SNAP_SPEED SnapSpeed = HIGH_SPEED;

Camera::Camera()
{
    Width = 640;
    Height = 480;
    rawBuffer_1	= NULL;
    rawBuffer_2	= NULL;
    m_pImageBuffer	= NULL;
    m_lHBlanking      = 0;
    m_lVBlanking      = 0;
    HVSTATUS status = STATUS_OK;

    status = BeginHVDevice(1, &camera_1);//打开数字摄像机 1
    if(status==STATUS_OK)
        m_bOpen = true;
    else
        return;
    status = BeginHVDevice(2, &camera_2);//打开数字摄像机 2

    HVSetResolution(camera_1, Resolution);//	设置数字摄像机分辨率
    HVSetResolution(camera_2, Resolution);
    HVSetSnapMode(camera_1, SnapMode);//采集模式，包括 CONTINUATION(连续)、TRIGGER(外触发)
    HVSetSnapMode(camera_2, SnapMode);
    HVADCControl(camera_1, ADC_BITS, ADCLevel);//设置ADC的级别
    HVADCControl(camera_2, ADC_BITS, ADCLevel);

    HVTYPE type = UNKNOWN_TYPE;//获取设备类型
    int size = sizeof(HVTYPE);
    HVGetDeviceInfo(camera_1,DESC_DEVICE_TYPE, &type, &size);

    HVSetBlanking(camera_1, m_lHBlanking, m_lVBlanking);//设置消隐
    HVSetBlanking(camera_2, m_lHBlanking, m_lVBlanking);
    HVSetOutputWindow(camera_1, XStart, YStart, Width, Height);
    HVSetOutputWindow(camera_2, XStart, YStart, Width, Height);
    HVSetSnapSpeed(camera_1, SnapSpeed);//设置采集速度
    HVSetSnapSpeed(camera_2, SnapSpeed);

    SetExposureTime(Width, ExposureTint_Upper, ExposureTint_Lower, m_lHBlanking, SnapSpeed, Resolution);//设置曝光时间

    //	m_pBmpInfo即指向m_chBmpBuf缓冲区，用户可以自己分配BTIMAPINFO缓冲区
    m_pBmpInfo = (BITMAPINFO *)m_chBmpBuf;
    //	初始化BITMAPINFO 结构，此结构在保存bmp文件、显示采集图像时使用
    m_pBmpInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    //	图像宽度，一般为输出窗口宽度
    m_pBmpInfo->bmiHeader.biWidth	= Width;
    //	图像宽度，一般为输出窗口高度
    m_pBmpInfo->bmiHeader.biHeight = Height;

    m_pBmpInfo->bmiHeader.biPlanes = 1;
    m_pBmpInfo->bmiHeader.biBitCount	= 24;
    m_pBmpInfo->bmiHeader.biCompression = BI_RGB;
    m_pBmpInfo->bmiHeader.biSizeImage		= 0;
    m_pBmpInfo->bmiHeader.biXPelsPerMeter	= 0;
    m_pBmpInfo->bmiHeader.biYPelsPerMeter	= 0;
    m_pBmpInfo->bmiHeader.biClrUsed			= 0;
    m_pBmpInfo->bmiHeader.biClrImportant	= 0;

    rawBuffer_1 = new BYTE[Width * Height];
    rawBuffer_2 = new BYTE[Width * Height];
    m_pImageBuffer = new BYTE[Width * Height * 3];

    QTimer *timer = new QTimer();
    timer->start(30);
    connect(timer, SIGNAL(timeout()), this, SLOT(CaptureFrame()));
}

Camera::~Camera()
{
    HVSTATUS status = STATUS_OK;
    //	关闭数字摄像机，释放数字摄像机内部资源
    status = EndHVDevice(camera_1);
    status = EndHVDevice(camera_2);
    OnSnapexStop();
    OnSnapexClose();
    //	回收图像缓冲区
    delete []rawBuffer_1;
    delete []rawBuffer_2;
    delete []m_pImageBuffer;
}

void Camera::OnSnapexOpen()
{
    HVSTATUS status = STATUS_OK;
    status = HVOpenSnap(camera_1, SnapThreadCallback, this);
    status = HVOpenSnap(camera_2, SnapThreadCallback, this);
}

void Camera::OnSnapexStart()
{
    HVSTATUS status = STATUS_OK;
    BYTE *ppBuf_1[1];
    BYTE *ppBuf_2[1];
    ppBuf_1[0] = rawBuffer_1;
    ppBuf_2[0] = rawBuffer_2;
    status = HVStartSnap(camera_1, ppBuf_1,1);
    status = HVStartSnap(camera_2, ppBuf_2,1);
}

void Camera::OnSnapexStop()
{
    HVSTATUS status = STATUS_OK;
    status = HVStopSnap(camera_1);
    status = HVStopSnap(camera_2);
}

void Camera::OnSnapexClose()
{
    HVSTATUS status = STATUS_OK;
    status = HVCloseSnap(camera_1);
    status = HVCloseSnap(camera_2);
}

int CALLBACK Camera::SnapThreadCallback(HV_SNAP_INFO *pInfo)
{
    return 1;
}

void Camera::CaptureFrame()
{
    image_1 = new QImage(rawBuffer_1, Width, Height, QImage::Format_Indexed8);
    image_2 = new QImage(rawBuffer_2, Width, Height, QImage::Format_Indexed8);
}

int Camera::OnSnapChange()
{
    return 1;
}

HVSTATUS Camera::SetExposureTime(int nWindWidth, long lTintUpper, long lTintLower, long HBlanking, HV_SNAP_SPEED SnapSpeed, HV_RESOLUTION Resolution)
{
    HVTYPE type = UNKNOWN_TYPE;
    int size    = sizeof(HVTYPE);
    HVGetDeviceInfo(camera_1,DESC_DEVICE_TYPE, &type, &size);

    int nOutputWid = nWindWidth;

    double dExposure = 0.0;
    double dTint = max((double)lTintUpper/(double)lTintLower,MY_ZERO);

    double lClockFreq = 0.0;

        lClockFreq = (SnapSpeed == HIGH_SPEED)? 24000000:12000000;
        long lTb = HBlanking;
        lTb += 9;
        lTb -= 19;
        if(lTb <= 0) lTb =0;
        if(((double)nOutputWid + 244.0 + lTb ) > 552)
            dExposure = (dTint* lClockFreq + 180.0)/((double)nOutputWid + 244.0 + lTb);
        else
            dExposure = ((dTint * lClockFreq)+ 180.0) / 552 ;

        if((dExposure-(int)dExposure) >0.5)
            dExposure += 1.0;
        if(dExposure <= 0)
            dExposure = 1;
        else if(dExposure > 16383)
            dExposure = 16383;

    return HVAECControl(camera_1, AEC_EXPOSURE_TIME, (long)dExposure);
}





















