#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtGui>

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QToolBox>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include <QLabel> 
#include <QImage>
#include <QTimer>

#include "set.h"
#include <opencv/highgui.h>
#include <opencv/cv.h>
#include "projector.h"
#include "reconstruct.h"
#include "meshcreator.h"
//#include "cameracalibration.h"

#include "Windows.h"//加载此头文件以解决大恒相机头文件类型未定义问题
#include <HVDAILT.h>
#include <Raw2Rgb.h>

#define WM_SNAP_CHANGE		(WM_USER + 100)

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
    Set *sWindow;

    QString projectPath;
    QString projChildPath;

    QWidget *viewWindow;
    QGridLayout *gridLayout;
    QToolBox *toolBox;
    QWidget *cameraAdjustPage;
    QGridLayout *gridLayout_6;
    QGroupBox *groupBox;
    QFormLayout *formLayout;
    QLabel *lightLabel;
    QSlider *lightSlider1;
    QLabel *speedLabel;
    QSlider *speedSlider1;
    QGroupBox *groupBox_2;
    QFormLayout *formLayout_2;
    QLabel *lightLabel_2;
    QSlider *lightSlider_2;
    QLabel *label_2;
    QSlider *speedSlider_2;
    QWidget *calibrationpage;
    QVBoxLayout *verticalLayout;
    QLabel *label_4;
    QSpacerItem *verticalSpacer_9;
    QLabel *explainLabel;
    QSpacerItem *verticalSpacer_10;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label_8;
    QLabel *currentPhotoLabel;
    QTreeWidget *treeWidget;
    QPushButton *calibButton;
    QPushButton *captureButton;
    QWidget *scanPage;
    QVBoxLayout *verticalLayout_2;
    QTreeWidget *treeWidget_2;
    QPushButton *scanButton;
    QWidget *reconstructionPage;
    QFormLayout *formLayout_3;
    QHBoxLayout *horizontalLayout_4;
    QPushButton *objButton;
    QPushButton *plyButton;
    QHBoxLayout *horizontalLayout;
    QWidget *leftView;
    QGridLayout *gridLayout_2;
    QSpacerItem *verticalSpacer;
    QSpacerItem *horizontalSpacer;
    QLabel *leftViewLabel;
    QSpacerItem *horizontalSpacer_2;
    QSpacerItem *verticalSpacer_2;
    QWidget *leftCapture;
    QGridLayout *gridLayout_3;
    QSpacerItem *verticalSpacer_3;
    QSpacerItem *horizontalSpacer_4;
    QLabel *leftCaptureLabel;
    QSpacerItem *horizontalSpacer_3;
    QSpacerItem *verticalSpacer_4;
    QWidget *rightCapture;
    QGridLayout *gridLayout_4;
    QSpacerItem *verticalSpacer_5;
    QSpacerItem *horizontalSpacer_6;
    QLabel *rightCaptureLabel;
    QSpacerItem *horizontalSpacer_5;
    QSpacerItem *verticalSpacer_6;
    QWidget *rightView;
    QGridLayout *gridLayout_5;
    QSpacerItem *verticalSpacer_7;
    QSpacerItem *horizontalSpacer_8;
    QLabel *rightViewLabel;
    QSpacerItem *horizontalSpacer_7;
    QSpacerItem *verticalSpacer_8;
    QWidget *threeDView;

    int screenWidth;//screen and projector resolution
    int screenHeight;
    int projectorWidth;
    int projectorHeight;
    int cameraWidth;
    int cameraHeight;

private:
    void createActions();
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

    QAction *newAction;
    QAction *openAction;
    QAction *saveAction;
    QAction *closeAction;
    QAction *openCameraAction;
    QAction *projectorAction;
    QAction *calibAction;
    QAction *scanAction;
    QAction *reconstructAction;
    QAction *setAction;

    QLabel *msgLabel;//show message in the bottom of the window

    QTimer *timer;

    QImage *image_1;//由rawbuffer得到的图像指针
    QImage *image_2;
    QPixmap pimage_1;//由图像指针得到的.png格式图像
    QPixmap pimage_2;

    bool cameraOpened;
    bool isProjectorOpened;
    bool isConfigured;
    Projector *pj;
    int saveCon;//count the photo captured.

    int viewWidth;
    QString path_1;
    QString path_2;

    //CameraCalibration *calibrator;

    ///////////////////////////////
    HHV	m_hhv_1;			///< 数字摄像机句柄
    HHV	m_hhv_2;

    BYTE *m_pRawBuffer_1;		///< 采集图像原始数据缓冲区
    BYTE *m_pRawBuffer_2;

    static int CALLBACK SnapThreadCallback(HV_SNAP_INFO *pInfo);
    ////////////////////////////////////////////////

private slots:
    void newproject();
    void openproject();

    void opencamera();
    void readframe();
    void capturecalib();
    void projectorcontrol();

    void calib();
    void calibration();
    void scan();
    void reconstruct();
    void set();
};

#endif // MAINWINDOW_H
