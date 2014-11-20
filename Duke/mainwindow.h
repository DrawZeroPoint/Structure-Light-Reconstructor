#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtGui>

#include <QLabel> 
#include <QImage>
#include <QTimer>

#include "set.h"
#include <opencv/highgui.h>
#include <opencv/cv.h>
#include "projector.h"
#include "reconstruct.h"
#include "meshcreator.h"
#include "glwidget.h"
#include "cameracalibration.h"


#include "Windows.h"//加载此头文件以解决大恒相机头文件类型未定义问题
#include <HVDAILT.h>
#include <Raw2Rgb.h>

#define WM_SNAP_CHANGE		(WM_USER + 100)

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
    Set *setDialog;
    GLWidget *displayModel;

    QString projectPath;
    QString projChildPath;

    int screenWidth;//screen and projector resolution
    int screenHeight;
    int projectorWidth;
    int projectorHeight;
    int cameraWidth;
    int cameraHeight;

private:
    Ui::MainWindow *ui;
    CameraCalibration *calibrator;

    void createConnections();
    void createCentralWindow(QWidget *parent);
    void captureImage(int saveCount, bool dispaly);
    void getScreenGeometry();
    void closeCamera();
    void generatePath(int type);
    void selectPath(int type);

    void OnSnapexOpen();
    void OnSnapexStart();
    void OnSnapexStop();
    void OnSnapexClose();
    int OnSnapChange();

    void progressPop(int up);

    QLabel *msgLabel;//show message in the bottom of the window

    QTimer *timer;

    //QImage *image_1;//由rawbuffer得到的图像指针
    QImage image_1;
    QImage *image_2;
    QPixmap pimage_1;//由图像指针得到的.png格式图像
    QPixmap pimage_2;

    bool cameraOpened;
    bool isProjectorOpened;
    bool isConfigured;
    Projector *pj;
    int saveCon;//count the photo captured.

    QString path_1;
    QString path_2;

    //CameraCalibration *calibrator;

    ///////////////////////////////
    HHV	m_hhv_1;			///< 数字摄像机句柄
    HHV	m_hhv_2;

    BYTE *ppBuf_1[1];
    BYTE *ppBuf_2[1];

    BYTE *m_pRawBuffer_1;		///< 采集图像原始数据缓冲区
    BYTE *m_pRawBuffer_2;

    static int CALLBACK SnapThreadCallback(HV_SNAP_INFO *pInfo);
    ////////////////////////////////////////////////
    ///与set对话框有关的变量
    int black_ ;
    int white_;
    bool isAutoContrast;
    bool isSaveAutoContrast;
    bool isRaySampling;
    bool isExportObj;
    bool isExportPly;

private slots:
    void newproject();
    void openproject();

    void opencamera();
    void readframe();
    void capturecalib();
    void redocapture();
    void projectorcontrol();

    void calib();
    void calibration();
    void scan();
    void reconstruct();
    void set();
    void getSetInfo();

    void changePointSize(int psize);
    void switchlanguage();

};

#endif // MAINWINDOW_H
