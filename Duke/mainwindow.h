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
#include "blobdetector.h"
#include "graycodes.h"
#include "multifrequency.h"

#include "dahengcamera.h"

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

    int screenWidth;//主屏幕几何尺寸
    int screenHeight;
    int projectorWidth;//投影屏幕几何尺寸
    int projectorHeight;
    int scanWidth;//扫描区域尺寸
    int scanHeight;
    int cameraWidth;//相机分辨率
    int cameraHeight;

    int scanSquenceNo;//表示当前正在进行的扫描序列数，从0开始

private:
    Ui::MainWindow *ui;
    CameraCalibration *calibrator;
    BlobDetector *blob;
    GrayCodes *grayCode;
    MultiFrequency *mf;
    Projector *pj;

    DaHengCamera *DHC;

    void createConnections();
    void createCentralWindow(QWidget *parent);
    void captureImage(QString pref, int saveCount, bool dispaly);
    void findPoint();
    void getScreenGeometry();
    void closeCamera();
    void generatePath(int type);

    ///---------------辅助功能---------------///

    void progressPop(int up);
    void drawCross(QPainter &p, int x, int y);

    QLabel *msgLabel;//show message in the bottom of the window

    QTimer *timer;
    QImage image_1;
    QImage image_2;
    QPixmap pimage_1;//由图像指针得到的.png格式图像
    QPixmap pimage_2;

    bool isProjectorOpened;
    bool isConfigured;
    int saveCount;//count the photo captured.

    QString path_1;
    QString path_2;

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
    void setexposure();
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
    void testmulitfreq();

    void reconstruct();
    void startreconstruct();

    void set();
    void getSetInfo();

    void changePointSize(int psize);
    void loadTestModel();
    void switchlanguage();

};

#endif // MAINWINDOW_H
