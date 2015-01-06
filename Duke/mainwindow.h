#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtGui>

#include <QLabel> 
#include <QImage>
#include <QTimer>

#include "set.h"
#include "dotmatch.h"
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

#define PATHCALIB 0
#define PATHSCAN 1
#define PATHRECON 2

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
    DotMatch *dm;
    GLWidget *displayModel;

    QString projectPath;
    QString projChildPath;

    int screenWidth;//screen and projector resolution
    int screenHeight;
    int projectorWidth;
    int projectorHeight;
    int cameraWidth;
    int cameraHeight;

    int scanSquenceNo;//表示当前正在进行的扫描序列数，从0开始

private:
    Ui::MainWindow *ui;
    CameraCalibration *calibrator;
    Projector *pj;

    void createConnections();
    void createCentralWindow(QWidget *parent);
    void captureImage(QString pref, int saveCount, bool dispaly);
    void findPoint();
    void getScreenGeometry();
    void closeCamera();
    void generatePath(int type);

    ///---------------相机相关函数---------------///

    void OnSnapexOpen();
    void OnSnapexStart();
    void OnSnapexStop();
    void OnSnapexClose();
    int OnSnapChange();

    ///---------------辅助功能---------------///

    void progressPop(int up);
    void drawCross(QPainter &p, int x, int y);

    QLabel *msgLabel;//show message in the bottom of the window

    QTimer *timer;
    QImage image_1;
    QImage image_2;
    QPixmap pimage_1;//由图像指针得到的.png格式图像
    QPixmap pimage_2;

    bool cameraOpened;
    bool isProjectorOpened;
    bool isConfigured;
    int saveCount;//count the photo captured.

    QString path_1;
    QString path_2;

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
    bool isRaySampling;
    bool isExportObj;
    bool isExportPly;

private slots:
    void newproject();
    void openproject();

    void opencamera();
    void exposurecontrol();
    void readframe();

    void selectPath(int PATH);

    void capturecalib();
    void redocapture();
    void projectorcontrol();

    void calib();
    void calibration();

    void scan();
    void pointmatch();
    void refindmatch();
    void startscan();

    void reconstruct();
    void startreconstruct();

    void set();
    void getSetInfo();

    void changePointSize(int psize);
    void loadTestModel();
    void switchlanguage();

};

#endif // MAINWINDOW_H
