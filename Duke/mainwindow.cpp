#include "mainwindow.h"
#include "graycodes.h"
#include "set.h"
#include "ui_mainwindow.h"
//#include "cameracalibration.h"

#include <QMenu>
#include <QImage>
#include <QKeySequence>
#include <QToolBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QDesktopWidget>
#include <QVector>

const HV_RESOLUTION Resolution = RES_MODE0;
const HV_SNAP_MODE SnapMode = CONTINUATION;
const HV_BAYER_CONVERT_TYPE ConvertType = BAYER2RGB_NEIGHBOUR1;
const HV_SNAP_SPEED SnapSpeed = HIGH_SPEED;
const long ADCLevel           = ADC_LEVEL2;
const int XStart              = 0;//图像左上角点在相机幅面1280X1024上相对于幅面左上角点坐标
const int YStart              = 0;
int Width;//相机视野
int Height;
int scanWidth;//扫描区域
int scanHeight;
bool inEnglish = true;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    createConnections();

    saveCon = 1;//Save calib images start with 1
    cameraOpened = false;
    isConfigured = false;
    isProjectorOpened = true;

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(readframe()));

    displayModel = new GLWidget(ui->displayWidget);
    QHBoxLayout *displayLayout = new QHBoxLayout(ui->displayWidget);
    displayLayout->setMargin(1);
    displayLayout->addWidget(displayModel);
    ui->displayWidget->setLayout(displayLayout);

    sWindow = new Set(this);//Initialize the set dialog
    getSetInfo();

    cameraWidth = Width;
    cameraHeight = Height;

    getScreenGeometry();//Get mian screen and projector screen geometry
    QDesktopWidget* desktopWidget = QApplication::desktop();
    QRect projRect = desktopWidget->screenGeometry(1);//1 represent projector
    int xOffSet = (projRect.width()-projectorWidth)/2 + screenWidth;
    int yOffSet = (projRect.height()-projectorHeight)/2;

    pj = new Projector(NULL, scanWidth, scanHeight, projectorWidth, projectorHeight, xOffSet, yOffSet);//Initialize the projector
    pj->move(screenWidth,0);//make the window displayed by the projector
    pj->showFullScreen();

    connect(ui->actionExit, SIGNAL(triggered()), pj, SLOT(close()));//解决投影窗口不能关掉的问题
    connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(close()));

}

MainWindow::~MainWindow()
{
    if(cameraOpened){
        OnSnapexStop();
        OnSnapexClose();
        HVSTATUS status = STATUS_OK;
        //	关闭数字摄像机，释放数字摄像机内部资源
        status = EndHVDevice(m_hhv_1);
        status = EndHVDevice(m_hhv_2);
        //	回收图像缓冲区
        delete []m_pRawBuffer_1;
        delete []m_pRawBuffer_2;
    }
    delete pj;
    delete ui;
}

void MainWindow::newproject()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),"/home",QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    projectPath = dir;
    if(projectPath!=""){
        for(int i = 0;i<3;i++){
            generatePath(i);
        }
    }
}

void MainWindow::openproject()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),"/home",QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    projectPath = dir;
}

void MainWindow::opencamera()
{
    HVSTATUS status_1 = STATUS_OK;
    HVSTATUS status_2 = STATUS_OK;
    m_pRawBuffer_1	= NULL;
    m_pRawBuffer_2	= NULL;

    status_1 = BeginHVDevice(1, &m_hhv_1);
    status_2 = BeginHVDevice(2, &m_hhv_2);
    if(status_1==STATUS_OK&&status_2==STATUS_OK){
        cameraOpened = true;
    }
    else{
        cameraOpened = false;
        QMessageBox::warning(NULL,"Cameras not found","Make sure two Daheng cameras have connected to the computer.");
        return;
    }
    HVSetResolution(m_hhv_1, Resolution);//Set the resolution of cameras
    HVSetResolution(m_hhv_2, Resolution);

    HVSetSnapMode(m_hhv_1, SnapMode);//Snap mode include CONTINUATION、TRIGGER
    HVSetSnapMode(m_hhv_2, SnapMode);
    //  设置ADC的级别
    HVADCControl(m_hhv_1, ADC_BITS, ADCLevel);
    HVADCControl(m_hhv_2, ADC_BITS, ADCLevel);
    //  获取设备类型
    HVTYPE type = UNKNOWN_TYPE;
    int size    = sizeof(HVTYPE);
    HVGetDeviceInfo(m_hhv_1,DESC_DEVICE_TYPE, &type, &size);//由于两相机型号相同，故只获取一个

    HVSetOutputWindow(m_hhv_1, XStart, YStart, Width, Height);
    HVSetOutputWindow(m_hhv_2, XStart, YStart, Width, Height);
    //设置采集速度
    HVSetSnapSpeed(m_hhv_1, SnapSpeed);
    HVSetSnapSpeed(m_hhv_2, SnapSpeed);

    m_pRawBuffer_1 = new BYTE[Width * Height];
    m_pRawBuffer_2 = new BYTE[Width * Height];

    OnSnapexOpen();
    OnSnapexStart();
    timer->start(30);

    ui->actionOpenCamera->setDisabled(true);//暂时保证不会启动两次，防止内存溢出
}

void MainWindow::OnSnapexOpen()
{
    HVSTATUS status = STATUS_OK;
    status = HVOpenSnap(m_hhv_1, SnapThreadCallback, this);
    status = HVOpenSnap(m_hhv_2, SnapThreadCallback, this);
}

void MainWindow::OnSnapexStart()
{
    HVSTATUS status = STATUS_OK;
    ppBuf_1[0] = m_pRawBuffer_1;
    ppBuf_2[0] = m_pRawBuffer_2;
    status = HVStartSnap(m_hhv_1, ppBuf_1,1);
    status = HVStartSnap(m_hhv_2, ppBuf_2,1);
}

void MainWindow::OnSnapexStop()
{
    HVSTATUS status = STATUS_OK;
    status = HVStopSnap(m_hhv_1);
    status = HVStopSnap(m_hhv_2);
}

void MainWindow::OnSnapexClose()
{
    HVSTATUS status = STATUS_OK;
    status = HVCloseSnap(m_hhv_1);
    status = HVCloseSnap(m_hhv_2);
}

int CALLBACK MainWindow::SnapThreadCallback(HV_SNAP_INFO *pInfo)
{
    return 1;
}

void MainWindow::readframe()
{
    image_1 = new QImage(m_pRawBuffer_1, Width, Height, QImage::Format_Indexed8);
    image_2 = new QImage(m_pRawBuffer_2, Width, Height, QImage::Format_Indexed8);
    pimage_1 = QPixmap::fromImage(*image_1);
    pimage_2 = QPixmap::fromImage(*image_2);
    ui->leftViewLabel->setPixmap(pimage_1);//use label to show the image
    ui->rightViewLabel->setPixmap(pimage_2);
}

void MainWindow::capturecalib()
{
    if(cameraOpened){
        selectPath(0);//0 for calibration and 1 for scan
        captureImage(saveCon,true);
        ui->currentPhotoLabel->setText(QString::number(saveCon));
        saveCon++;
        QString explain = ":/" + QString::number(saveCon) + ".png";
        ui->explainLabel->setPixmap(explain);
        if(saveCon == 13){
            saveCon = 1;
            ui->calibButton->setEnabled(true);
        }
    }
    else
        QMessageBox::warning(this,"Warning","Open cameras failed.");
}

void MainWindow::redocapture()
{
    if(cameraOpened){
        captureImage(saveCon - 1, true);
        }
    else
        QMessageBox::warning(this,"Warning","Open cameras failed.");
}

void MainWindow::captureImage(int saveCount,bool dispaly)
{
    QString savecount = QString::number(saveCount);
    pimage_1.save(projChildPath + "/left/L" + savecount +".png");
    pimage_2.save(projChildPath + "/right/R" + savecount +".png");
    if(dispaly){
        ui->leftCaptureLabel->setPixmap(pimage_1);
        ui->rightCaptureLabel->setPixmap(pimage_2);
    }
}

void MainWindow::generatePath(int type)
{
    selectPath(type);
    QDir *addpath_1 = new QDir;
    QDir *addpath_2 = new QDir;
    if(type==0||type==1){
        addpath_1->mkpath(projChildPath + "/left/");
        addpath_2->mkpath(projChildPath +"/right/");
    }
    else
        addpath_1->mkpath(projChildPath);
}

void MainWindow::selectPath(int type)//decide current childpath
{
    QString t;
    switch(type){
        case 0:
        t = "/calib";
        break;
        case 1:
        t = "/scan";
        break;
        case 2:
        t = "/reconstruction";
        break;
    }
    projChildPath = projectPath + t;
}

void MainWindow::closeCamera()
{
    timer->stop();
    OnSnapexStop();
    OnSnapexClose();
}

void MainWindow::projectorcontrol()
{
    isProjectorOpened = !isProjectorOpened;
    if(isProjectorOpened){
        pj->displaySwitch(true);
    }
    else{
        pj->displaySwitch(false);
    }
}

void MainWindow::getScreenGeometry()
{
    QDesktopWidget* desktopWidget = QApplication::desktop();
    int screencount = desktopWidget->screenCount();//get screen amount
    if(screencount == 1){
    }
    else{
        QRect screenRect = desktopWidget->screenGeometry(0);
        screenWidth = screenRect.width();
        screenHeight = screenRect.height();
    }
}

void MainWindow::calib()
{
    QMessageBox::information(NULL, tr("Calibration"), tr("Calibration Actived!"));
    ui->tabWidget->setCurrentIndex(0);//go to calibration page
    ui->explainLabel->setPixmap(":/" + QString::number(saveCon) + ".png");
}

void MainWindow::calibration()
{
    /*
    for(int i=1; i<=2; i++)
    {
        calibrator = new CameraCalibration();
        std::string path = "camera";
        path += '0' + i;
        path += '/';

        //load images
        calibrator->loadCameraImgs(path.c_str());
        calibrator->extractImageCorners();
        calibrator->calibrateCamera();
        calibrator->findCameraExtrisics();
        //export txt files
        std::string file_name;

        path += "output/";

        file_name =  path.c_str();
        file_name += "cam_matrix.txt";

        calibrator->exportTxtFiles(file_name.c_str(),CAMCALIB_OUT_MATRIX);

        file_name =  path.c_str();
        file_name += "cam_distortion.txt";

        calibrator->exportTxtFiles(file_name.c_str(),CAMCALIB_OUT_DISTORTION);

        file_name =  path.c_str();
        file_name += "cam_rotation_matrix.txt";

        calibrator->exportTxtFiles(file_name.c_str(),CAMCALIB_OUT_ROTATION);

        file_name =  path.c_str();
        file_name += "cam_trans_vectror.txt";

        calibrator->exportTxtFiles(file_name.c_str(),CAMCALIB_OUT_TRANSLATION);

        file_name =  path.c_str();
        file_name += "calib.xml";
        calibrator->saveCalibData(file_name.c_str());

        // show data on consol
        calibrator->printData();
    }
    */
}

void MainWindow::scan()
{
    if(!cameraOpened){
        QMessageBox::warning(this,"Cameras not opened","Cameras are not opened.");
        return;
    }
    if(projectPath==""){
        QMessageBox::warning(this,"Save path not set","You need create a project first.");
        return;
    }
    selectPath(1);
    ui->tabWidget->setCurrentIndex(1);
    closeCamera();
    pj->displaySwitch(false);
    pj->opencvWindow();
    GrayCodes *grayCode = new GrayCodes(projectorWidth,projectorHeight);
    grayCode->generateGrays();
    pj->showImg(grayCode->getNextImg());
    int grayCount = 0;

    while(true)
    {
        cvWaitKey(100);
        HVSnapShot(m_hhv_1, ppBuf_1, 1);
        image_1 = new QImage(m_pRawBuffer_1, Width, Height, QImage::Format_Indexed8);
        pimage_1 = QPixmap::fromImage(*image_1);
        delete image_1;
        HVSnapShot(m_hhv_2, ppBuf_2, 1);
        image_2 = new QImage(m_pRawBuffer_2, Width, Height, QImage::Format_Indexed8);
        pimage_2 = QPixmap::fromImage(*image_2);
        delete image_2;
        captureImage(grayCount, false);
        grayCount++;
        //show captured result
        if(grayCount == grayCode->getNumOfImgs())
            break;
        pj->showImg(grayCode->getNextImg());
    }
    HVOpenSnap(m_hhv_1,SnapThreadCallback, this);
    HVOpenSnap(m_hhv_2,SnapThreadCallback, this);
    HVStartSnap(m_hhv_1,ppBuf_1,1);
    HVStartSnap(m_hhv_2,ppBuf_2,1);
    timer->start();
    pj->destoryWindow();
    pj->displaySwitch(true);
}

void MainWindow::reconstruct()
{
    if(cameraOpened)
        closeCamera();
    if(isConfigured == false){
        QMessageBox::warning(this,"Warning","Press 'Set' button to configure the settings.");
        return;
    }
    ui->tabWidget->setCurrentIndex(2);
    selectPath(2);//set current path to :/reconstruct
    Reconstruct *reconstructor= new Reconstruct(2,projectPath);
    reconstructor->getParameters(projectorWidth, projectorHeight, cameraWidth, cameraHeight, isAutoContrast, isSaveAutoContrast,projectPath);
    reconstructor->setCalibPath(projectPath+"/calib/left/","L",".png",0);
    reconstructor->setCalibPath(projectPath+"/calib/right/","R",".png",1);
    bool loaded = reconstructor->loadCameras();//load camera matrix
    if(!loaded)
        return;
    reconstructor->setBlackThreshold(black_);
    reconstructor->setWhiteThreshold(white_);
    if(isSaveAutoContrast)
        reconstructor->enableSavingAutoContrast();
    else
        reconstructor->disableSavingAutoContrast();
    if(isRaySampling)
        reconstructor->enableRaySampling();
    else
        reconstructor->disableRaySampling();

    bool runSucess = reconstructor->runReconstruction();
    if(!runSucess)
        return;

    MeshCreator *meshCreator=new MeshCreator(reconstructor->points3DProjView);//Export mesh
    if(isExportObj)
        meshCreator->exportObjMesh(projectPath + "/reconstruction/result.obj");
    if(isExportPly || !(isExportObj || isExportPly))
        meshCreator->exportPlyMesh(projectPath + "/reconstruction/result.ply");
    delete meshCreator;
    delete reconstructor;
    /*
    HVOpenSnap(m_hhv_1,SnapThreadCallback, this);
    HVOpenSnap(m_hhv_2,SnapThreadCallback, this);
    HVStartSnap(m_hhv_1,ppBuf_1,1);
    HVStartSnap(m_hhv_2,ppBuf_2,1);
    timer->start();
    */
}

void MainWindow::set()
{
    sWindow->show();
    sWindow->saveSetPath = projectPath;
    isConfigured = true;
    connect(sWindow,SIGNAL(outputSet()),this,SLOT(getSetInfo()));
}

void MainWindow::getSetInfo()
{
    Width = sWindow->cam_w;
    Height = sWindow->cam_h;
    projectorWidth = sWindow->proj_w;
    projectorHeight = sWindow->proj_h;
    scanWidth = sWindow->scan_w;
    scanHeight = sWindow->scan_h;
    black_ = sWindow->black_threshold;
    white_ = sWindow->white_threshold;
    isAutoContrast = sWindow->autoContrast;
    isSaveAutoContrast = sWindow->saveAutoContrast;
    isRaySampling = sWindow->raySampling;
    isExportObj = sWindow->exportObj;
    isExportPly = sWindow->exportPly;
}

void MainWindow::createConnections()
{
    connect(ui->actionNew, SIGNAL(triggered()), this, SLOT(newproject()));
    connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(openproject()));
    connect(ui->actionOpenCamera, SIGNAL(triggered()), this, SLOT(opencamera()));
    connect(ui->actionProjector,SIGNAL(triggered()),this,SLOT(projectorcontrol()));
    connect(ui->actionCalib, SIGNAL(triggered()), this, SLOT(calib()));
    connect(ui->captureButton, SIGNAL(clicked()), this, SLOT(capturecalib()));
    connect(ui->redoButton, SIGNAL(clicked()), this, SLOT(redocapture()));
    connect(ui->calibButton,SIGNAL(clicked()),this,SLOT(calibration()));
    connect(ui->actionScan,SIGNAL(triggered()),this,SLOT(scan()));
    connect(ui->actionReconstruct,SIGNAL(triggered()),this,SLOT(reconstruct()));
    connect(ui->actionSet, SIGNAL(triggered()), this, SLOT(set()));
    connect(ui->actionChinese, SIGNAL(triggered()), this, SLOT(switchlanguage()));
    connect(ui->actionEnglish, SIGNAL(triggered()), this, SLOT(switchlanguage()));
}

void MainWindow::switchlanguage()
{
    QString qmPath = ":/";//表示从资源文件夹中加载
    QString local;
    if(inEnglish)
    {
        local = "zh.pm";
        inEnglish = false;
        ui->actionEnglish->setEnabled(true);
        ui->actionChinese->setDisabled(true);
    }
    else
    {
        local = "en.pm";
        inEnglish = true;
        ui->actionEnglish->setEnabled(false);
        ui->actionChinese->setDisabled(false);
    }
    QTranslator trans;
    trans.load(local, qmPath);
    qApp->installTranslator(&trans);
    ui->retranslateUi(this);
    sWindow->switchLang();
}
