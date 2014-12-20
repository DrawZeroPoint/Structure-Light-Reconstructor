#include "mainwindow.h"
#include "graycodes.h"
#include "set.h"
#include "ui_mainwindow.h"

#include <QMenu>
#include <QImage>
#include <QKeySequence>
#include <QToolBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QDesktopWidget>
#include <QVector>
#include <QFont>

const HV_RESOLUTION Resolution = RES_MODE0;
const HV_SNAP_MODE SnapMode = CONTINUATION;
const HV_BAYER_CONVERT_TYPE ConvertType = BAYER2RGB_NEIGHBOUR1;
const HV_SNAP_SPEED SnapSpeed = HIGH_SPEED;
long ADCLevel           = ADC_LEVEL2;
const int XStart              = 0;//图像左上角点在相机幅面1280X1024上相对于幅面左上角点坐标
const int YStart              = 0;
int Width;//相机视野
int Height;
int scanWidth;//扫描区域
int scanHeight;
bool inEnglish = true;

int nowProgress = 0;//进度条初始化

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
    ui->displayLayout->addWidget(displayModel);

    setDialog = new Set(this);//Initialize the set dialog
    getSetInfo();

    cameraWidth = Width;
    cameraHeight = Height;

    getScreenGeometry();//Get mian screen and projector screen geometry
    QDesktopWidget* desktopWidget = QApplication::desktop();
    QRect projRect = desktopWidget->screenGeometry(1);//1 represent projector
    int xOffSet = (projRect.width() - scanWidth)/2 + screenWidth;
    int yOffSet = (projRect.height() - scanHeight)/2;

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
    if(projectPath != "")
    {
        for(int i = 0;i<3;i++){
            generatePath(i);
        }
        dm = new DotMatch(this, projectPath);
    }
}

void MainWindow::openproject()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),"/home",QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    projectPath = dir;
}

///---------------------相机-----------------------///
void MainWindow::exposurecontrol()
{
    switch (ui->leftExSlider->value()) {
    case 0:
        ADCLevel = ADC_LEVEL3;
        break;
    case 1:
        ADCLevel = ADC_LEVEL2;
        break;
    case 2:
        ADCLevel = ADC_LEVEL1;
        break;
    case 3:
        ADCLevel = ADC_LEVEL0;
        break;
    }
    HVADCControl(m_hhv_1, ADC_BITS, ADCLevel);
    switch (ui->rightExSlider->value()) {
    case 0:
        ADCLevel = ADC_LEVEL3;
        break;
    case 1:
        ADCLevel = ADC_LEVEL2;
        break;
    case 2:
        ADCLevel = ADC_LEVEL1;
        break;
    case 3:
        ADCLevel = ADC_LEVEL0;
        break;
    }
    HVADCControl(m_hhv_2, ADC_BITS, ADCLevel);
}

void MainWindow::opencamera()
{
    HVSTATUS status_1 = STATUS_OK;
    HVSTATUS status_2 = STATUS_OK;
    m_pRawBuffer_1	= NULL;
    m_pRawBuffer_2	= NULL;

    status_1 = BeginHVDevice(1, &m_hhv_1);
    status_2 = BeginHVDevice(2, &m_hhv_2);
    if(status_1==STATUS_OK&&status_2==STATUS_OK)
        cameraOpened = true;
    else{
        cameraOpened = false;
        QMessageBox::warning(NULL, tr("Cameras not found"), tr("Make sure two Daheng cameras have connected to the computer."));
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
    ui->leftExSlider->setEnabled(true);//激活曝光调整滑块
    ui->rightExSlider->setEnabled(true);
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

void MainWindow::closeCamera()
{
    timer->stop();
    OnSnapexStop();
    OnSnapexClose();
}

int CALLBACK MainWindow::SnapThreadCallback(HV_SNAP_INFO *pInfo)
{
    return 1;
}

void MainWindow::readframe()
{
    image_1 = QImage(m_pRawBuffer_1, Width, Height, QImage::Format_Indexed8);
    image_2 = QImage(m_pRawBuffer_2, Width, Height, QImage::Format_Indexed8);
    pimage_1 = QPixmap::fromImage(image_1);
    pimage_2 = QPixmap::fromImage(image_2);
    ui->leftViewLabel->setPixmap(pimage_1);
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
        QMessageBox::warning(this, tr("Warning"), tr("Open cameras failed."));
}

void MainWindow::redocapture()
{
    if(cameraOpened){
        captureImage(saveCon - 1, true);
        }
    else
        QMessageBox::warning(this,tr("Warning"), tr("Open cameras failed."));
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

///////////////////标定/////////////////////
void MainWindow::calib()
{
    QMessageBox::information(NULL, tr("Calibration"), tr("Calibration Actived!"));
    ui->tabWidget->setCurrentIndex(0);//go to calibration page
    ui->explainLabel->setPixmap(":/" + QString::number(saveCon) + ".png");
}


void MainWindow::calibration()
{
    ui->progressBar->reset();
    nowProgress = 0;
    calibrator = new CameraCalibration();
    QString path;

    for(int i = 1; i <= 2; i++)
    {
        path = projectPath + "/calib/";

        if (i == 1)
        {
            path += "left/L";
            calibrator->isleft = true;
        }
        else
        {
            path += "right/R";
            calibrator->isleft = false;
        }

        //load images
        calibrator->loadCameraImgs(path);
        progressPop(5);

        calibrator->extractImageCorners();
        progressPop(15);

        calibrator->calibrateCamera();
        progressPop(10);

        calibrator->findCameraExtrisics();
        progressPop(10);

        //export txt files
        QString file_name;
        if(i == 1)
            path = projectPath + "/calib/left/";
        else
            path = projectPath + "/calib/right/";

        file_name =  path;
        file_name += "cam_matrix.txt";
        calibrator->exportTxtFiles(file_name.toLocal8Bit(),CAMCALIB_OUT_MATRIX);

        file_name =  path;
        file_name += "cam_distortion.txt";
        calibrator->exportTxtFiles(file_name.toLocal8Bit(),CAMCALIB_OUT_DISTORTION);

        file_name =  path;
        file_name += "cam_rotation_matrix.txt";
        calibrator->exportTxtFiles(file_name.toLocal8Bit(),CAMCALIB_OUT_ROTATION);

        file_name =  path;
        file_name += "cam_trans_vectror.txt";
        calibrator->exportTxtFiles(file_name.toLocal8Bit(),CAMCALIB_OUT_TRANSLATION);
        progressPop(10);
    }
    path = projectPath + "/calib/fundamental_mat.txt";
    calibrator->findFundamental();
    calibrator->exportTxtFiles(path.toLocal8Bit(), CAMCALIB_OUT_FUNDAMENTAL);

    ui->progressBar->setValue(100);
}

///-----------------------扫描---------------------------///
void MainWindow::scan()
{
    ui->progressBar->reset();
    nowProgress = 0;

    if(!cameraOpened){
        QMessageBox::warning(this, tr("Cameras not opened"), tr("Cameras are not opened."));
        return;
    }
    if(projectPath==""){
        QMessageBox::warning(this,tr("Save path not set"), tr("You need create a project first."));
        return;
    }
    selectPath(1);
    ui->tabWidget->setCurrentIndex(1);
    ui->findPointButton->setEnabled(true);
    ui->startScanButton->setEnabled(true);
    pj->crossVisible = false;
    pj->update();//自动调用paintevent重绘窗口，取消十字的显示
}

void MainWindow::pointmatch()
{
    //dm->bwThreshold = ui->thresholdBox->value();
    findPoint();
}

void MainWindow::findPoint()
{
    if (dm->dotForMark.size() != 0)
    {
        dm->dotForMark.clear();
    }
    cv::Mat mat_1 = cv::Mat(Height, Width, CV_8UC1, m_pRawBuffer_1);//直接从内存缓冲区获得图像数据是可行的
    cv::Mat mat_2 = cv::Mat(Height, Width, CV_8UC1, m_pRawBuffer_2);
    //imshow("d",mat_1);
    //cvWaitKey(10);
    dm->matchDot(mat_1,mat_2);
    QPixmap pcopy_1 = pimage_1;
    QPixmap pcopy_2 = pimage_2;
    QPainter pt_1(&pcopy_1);
    QPainter pt_2(&pcopy_2);
    QFont textf("Calibri",50);
    QColor greencolor(0,255,0);
    QColor yellowcolor(255,255,0);
    pt_1.setFont(textf);
    pt_2.setFont(textf);

    for(int i = 0;i < dm->dotForMark.size();i++)
    {
        if (dm->dotForMark[i][4] == 1)
        {
            pt_1.setPen(greencolor);
            pt_2.setPen(greencolor);
            pt_1.drawText(dm->dotForMark[i][0],dm->dotForMark[i][1],QString::number(dm->dotForMark[i][5]));
            pt_2.drawText(dm->dotForMark[i][2],dm->dotForMark[i][3],QString::number(dm->dotForMark[i][5]));
            drawCross(pt_1, dm->dotForMark[i][0] ,dm->dotForMark[i][1]);
            drawCross(pt_2, dm->dotForMark[i][2], dm->dotForMark[i][3]);
        }
        else
        {
            pt_1.setPen(yellowcolor);
            pt_2.setPen(yellowcolor);
            drawCross(pt_1, dm->dotForMark[i][0] ,dm->dotForMark[i][1]);
            drawCross(pt_2, dm->dotForMark[i][2], dm->dotForMark[i][3]);
        }
    }
    ui->leftCaptureLabel->setPixmap(pcopy_1);
    ui->rightCaptureLabel->setPixmap(pcopy_2);
}

void MainWindow::startscan()
{
    /*
    if (dm->dotInOrder.size()<3)
    {
        QMessageBox::warning(this,tr("Not enough match point"), tr("Active Find Point to match marked point first."));
        return;
    }
    */
    closeCamera();
    pj->displaySwitch(false);
    pj->opencvWindow();
    GrayCodes *grayCode = new GrayCodes(projectorWidth,projectorHeight);
    grayCode->generateGrays();
    progressPop(10);

    pj->showImg(grayCode->getNextImg());
    int grayCount = 0;

    while(true)
    {
        cvWaitKey(100);
        HVSnapShot(m_hhv_1, ppBuf_1, 1);
        //image_1 = new QImage(m_pRawBuffer_1, Width, Height, QImage::Format_Indexed8);
        image_1 = QImage(m_pRawBuffer_1, Width, Height, QImage::Format_Indexed8);
        pimage_1 = QPixmap::fromImage(image_1);

        HVSnapShot(m_hhv_2, ppBuf_2, 1);
        image_2 = QImage(m_pRawBuffer_2, Width, Height, QImage::Format_Indexed8);
        pimage_2 = QPixmap::fromImage(image_2);

        captureImage(grayCount, false);
        grayCount++;
        //show captured result
        if(grayCount == grayCode->getNumOfImgs())
            break;
        pj->showImg(grayCode->getNextImg());
        progressPop(2);
    }
    HVOpenSnap(m_hhv_1,SnapThreadCallback, this);
    HVOpenSnap(m_hhv_2,SnapThreadCallback, this);
    HVStartSnap(m_hhv_1,ppBuf_1,1);
    HVStartSnap(m_hhv_2,ppBuf_2,1);
    timer->start();
    pj->destoryWindow();
    pj->displaySwitch(true);

    ui->progressBar->setValue(100);
}

////////////////////////重建////////////////////////
void MainWindow::reconstruct()
{
    ui->progressBar->reset();
    nowProgress = 0;

    if(cameraOpened)
        closeCamera();
    if(isConfigured == false){
        QMessageBox::warning(this,tr("Warning"), tr("Press 'Set' button to configure the settings."));
        return;
    }
    ui->tabWidget->setCurrentIndex(2);
    selectPath(2);//set current path to :/reconstruct
    Reconstruct *reconstructor= new Reconstruct();
    reconstructor->getParameters(scanWidth, scanHeight, cameraWidth, cameraHeight, isAutoContrast, isSaveAutoContrast,projectPath);

    reconstructor->setCalibPath(projectPath+"/calib/left/", 0);
    reconstructor->setCalibPath(projectPath+"/calib/right/", 1);
    bool loaded = reconstructor->loadCameras();//load camera matrix
    if(!loaded)
        return;
    progressPop(5);

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
    progressPop(15);

    bool runSucess = reconstructor->runReconstruction();
    if(!runSucess)
        return;
    progressPop(50);

    MeshCreator *meshCreator=new MeshCreator(reconstructor->points3DProjView);//Export mesh
    progressPop(10);

    if(isExportObj)
        meshCreator->exportObjMesh(projectPath + "/reconstruction/result.obj");
    if(isExportPly || !(isExportObj || isExportPly))
        meshCreator->exportPlyMesh(projectPath + "/reconstruction/result.ply");
    delete meshCreator;
    delete reconstructor;
    opencamera();
    ui->progressBar->setValue(100);
}


void MainWindow::set()
{
    setDialog->show();
    setDialog->saveSetPath = projectPath;
    isConfigured = true;
    connect(setDialog,SIGNAL(outputSet()),this,SLOT(getSetInfo()));
}


void MainWindow::getSetInfo()
{
    Width = setDialog->cam_w;
    Height = setDialog->cam_h;
    projectorWidth = setDialog->proj_w;
    projectorHeight = setDialog->proj_h;
    scanWidth = setDialog->scan_w;
    scanHeight = setDialog->scan_h;
    black_ = setDialog->black_threshold;
    white_ = setDialog->white_threshold;
    isAutoContrast = setDialog->autoContrast;
    isSaveAutoContrast = setDialog->saveAutoContrast;
    isRaySampling = setDialog->raySampling;
    isExportObj = setDialog->exportObj;
    isExportPly = setDialog->exportPly;
}


void MainWindow::createConnections()
{
    connect(ui->actionNew, SIGNAL(triggered()), this, SLOT(newproject()));
    connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(openproject()));

    connect(ui->actionOpenCamera, SIGNAL(triggered()), this, SLOT(opencamera()));
    connect(ui->leftExSlider,SIGNAL(valueChanged(int)),this,SLOT(exposurecontrol()));
    connect(ui->rightExSlider,SIGNAL(valueChanged(int)),this,SLOT(exposurecontrol()));

    connect(ui->actionProjector,SIGNAL(triggered()),this,SLOT(projectorcontrol()));

    connect(ui->actionCalib, SIGNAL(triggered()), this, SLOT(calib()));
    connect(ui->captureButton, SIGNAL(clicked()), this, SLOT(capturecalib()));
    connect(ui->redoButton, SIGNAL(clicked()), this, SLOT(redocapture()));
    connect(ui->calibButton,SIGNAL(clicked()),this,SLOT(calibration()));

    connect(ui->actionScan,SIGNAL(triggered()),this,SLOT(scan()));
    connect(ui->findPointButton,SIGNAL(clicked()),this,SLOT(pointmatch()));
    connect(ui->thresholdBox,SIGNAL(valueChanged(int)),this,SLOT(pointmatch()));
    connect(ui->startScanButton, SIGNAL(clicked()), this, SLOT(startscan()));

    connect(ui->actionReconstruct,SIGNAL(triggered()),this,SLOT(reconstruct()));
    connect(ui->actionSet, SIGNAL(triggered()), this, SLOT(set()));
    connect(ui->actionChinese, SIGNAL(triggered()), this, SLOT(switchlanguage()));
    connect(ui->actionEnglish, SIGNAL(triggered()), this, SLOT(switchlanguage()));
    connect(ui->pSizeValue, SIGNAL(valueChanged(int)), this, SLOT(changePointSize(int)));
}

////////////////*辅助功能*/////////////////

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
    setDialog->switchLang();
}

void MainWindow::changePointSize(int psize)
{
    displayModel->setPoint(psize);
}


void MainWindow::progressPop(int up)
{
    nowProgress += up;
    ui->progressBar->setValue(nowProgress);
}

void MainWindow::drawCross(QPainter &p, int x, int y)
{
    int len = 25;
    p.drawLine(x - len, y, x + len, y);
    p.drawLine(x, y - len, x, y + len);
}
