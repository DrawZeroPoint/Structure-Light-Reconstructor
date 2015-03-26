#include "mainwindow.h"
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


bool inEnglish = true;
int nowProgress = 0;//进度条初始化

QFont textf("Calibri",50);
QColor greencolor(0,255,0);
QColor orangecolor(238,76,0);

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    /*****生成主窗口UI*****/
    ui->setupUi(this);

    /*****声明全局变量*****/
    saveCount = 1;//Save calib images start with 1
    scanSN = -1;
    isConfigured = false;
    isProjectorOpened = true;

    /****生成计时器并连接*****/
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(readframe()));

    /****声明相机****/
    usebc = false;
    DHC = new DaHengCamera(this);

    /****生成对焦辅助窗口****/
    fa = new FocusAssistant();
    showFocus = false;

    /*****生成OpenGL窗口并加载*****/
    displayModel = new GLWidget(ui->displayWidget);
    ui->displayLayout->addWidget(displayModel);

    /*****生成设置窗口并输出默认设置*****/
    setDialog = new Set(this);//Initialize the set dialog
    getSetInfo();

    /*****获取屏幕尺寸信息*****/
    getScreenGeometry();//Get mian screen and projector screen geometry
    QDesktopWidget* desktopWidget = QApplication::desktop();
    QRect projRect = desktopWidget->screenGeometry(1);//1 represent projector
    int xOffSet = (projRect.width() - scanWidth)/2 + screenWidth;
    int yOffSet = (projRect.height() - scanHeight)/2;

    /*****初始化投影窗口*****/
    pj = new Projector(NULL, scanWidth, scanHeight, projectorWidth, projectorHeight, xOffSet, yOffSet);//Initialize the projector
    pj->move(screenWidth,0);//make the window displayed by the projector
    pj->showFullScreen();

    /*****建立连接*****/
    createConnections();

    /*****初始化圆点探测*****/
    blob = new BlobDetector();
}

MainWindow::~MainWindow()
{
    delete DHC;
    delete pj;
    delete fa;
    delete blob;
    delete ui;
}

void MainWindow::newproject()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),"/home",QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    projectPath = dir;
    if(projectPath != ""){
        for(int i = 0;i<3;i++){
            generatePath(i);
        }
        dm = new DotMatch(this, projectPath, ui->matchAssistant->isChecked());
        connect(dm,SIGNAL(receivedmanualmatch()),this,SLOT(finishmanualmatch()));
    }
}

void MainWindow::openproject()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),"/home",QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    projectPath = dir;
}

/// ----------相机控制---------- ///
void MainWindow::setexposure()
{
    if (DHC->cameraOpened)
        DHC->daHengExposure(ui->leftExSlider->value(),ui->rightExSlider->value());
    else
        return;
}

void MainWindow::opencamera()
{
    if (!usebc){
        DHC->openDaHengCamera(cameraWidth,cameraHeight);
    }
    else {
        BC->openCamera();
    }
        ui->actionOpenCamera->setDisabled(true);//暂时保证不会启动两次，防止内存溢出
        ui->leftExSlider->setEnabled(true);//激活曝光调整滑块
        ui->rightExSlider->setEnabled(true);
        ui->actionFocusAssistant->setEnabled(true);//激活对焦辅助
        timer->start();
        image_1 = QImage(cameraWidth, cameraHeight, QImage::Format_Indexed8);
        image_2 = QImage(cameraWidth, cameraHeight, QImage::Format_Indexed8);
}

void MainWindow::startfocusassistant()
{
    fa->showFullScreen();
    showFocus = true;
    connect(fa, SIGNAL(winhide()), this, SLOT(closefocus()));
}

void MainWindow::closefocus()
{
    showFocus = false;
}

void MainWindow::readframe()
{
    if (!usebc){
    image_1 = QImage(DHC->m_pRawBuffer_1, cameraWidth, cameraHeight, QImage::Format_Indexed8);
    image_2 = QImage(DHC->m_pRawBuffer_2, cameraWidth, cameraHeight, QImage::Format_Indexed8);
    }
    else{
        image_1 = QImage(BC->pImageBuffer, cameraWidth, cameraHeight, QImage::Format_Indexed8);
    }
    pimage_1 = QPixmap::fromImage(image_1);
    pimage_2 = QPixmap::fromImage(image_2);
    ui->leftViewLabel->setPixmap(pimage_1);
    ui->rightViewLabel->setPixmap(pimage_2);
    if(showFocus){
        if(fa->displayLeft)
            fa->playImage(pimage_1);
        else
            fa->playImage(pimage_2);
    }
}

void MainWindow::usebasler()
{
    usebc = true;
    BC = new BaslerCamera(this);

}

///-------------------标定-------------------///
void MainWindow::calib()
{
    QMessageBox::information(NULL, tr("Calibration"), tr("Calibration Actived!"));
    selectPath(PATHCALIB);
    ui->tabWidget->setCurrentIndex(0);//go to calibration page
    ui->explainLabel->setPixmap(":/" + QString::number(saveCount) + ".png");
    ui->captureButton->setEnabled(true);
    ui->redoButton->setEnabled(true);
    ui->calibButton->setEnabled(true);
}


void MainWindow::capturecalib()
{
    if(DHC->cameraOpened){
        captureImage("", saveCount, true);
        ui->currentPhotoLabel->setText(QString::number(saveCount));
        saveCount++;
        QString explain = ":/" + QString::number(saveCount) + ".png";
        ui->explainLabel->setPixmap(explain);
        if(saveCount == CALIBIMGNUM+1){
            saveCount = 1;
            ui->calibButton->setEnabled(true);
        }
    }
    else
        QMessageBox::warning(this, tr("Warning"), tr("Open cameras failed."));
}


void MainWindow::redocapture()
{
    if(DHC->cameraOpened){
        captureImage("", saveCount - 1, true);
        }
    else
        QMessageBox::warning(this,tr("Warning"), tr("Open cameras failed."));
}


void MainWindow::captureImage(QString pref, int saveCount,bool dispaly)
{
    pimage_1.save(projChildPath + "/left/" + pref + "L" + QString::number(saveCount) +".png");
    pimage_2.save(projChildPath + "/right/" + pref + "R" + QString::number(saveCount) +".png");
    if(dispaly){
        for (size_t camCount = 0; camCount < 2; camCount++){
                BYTE *buffer = (camCount == 0)?(image_1.bits()):(image_2.bits());
                cv::Mat mat = cv::Mat(cameraHeight, cameraWidth, CV_8UC1, buffer);//直接从内存缓冲区获得图像数据是可行的
                //int bwThreshold = dm->OSTU_Region(mat);
                cv::Mat bimage = mat >= 60;
                cv::bitwise_not(bimage, bimage);
                vector<cv::Point2d> centers;
                blob->findBlobs(bimage,centers);
            if(camCount==0){
                QPixmap pcopy = pimage_1;
                QPainter pt(&pcopy);
                pt.setPen(greencolor);
                for (size_t i = 0; i < centers.size();i++){
                    drawCross(pt,centers[i].x,centers[i].y);
                }
                ui->leftCaptureLabel->setPixmap(pcopy);
            }
            else{
                QPixmap pcopy_1 = pimage_2;
                QPainter pt_1(&pcopy_1);
                pt_1.setPen(greencolor);
                for (size_t i = 0; i < centers.size();i++)
                {
                    drawCross(pt_1,centers[i].x,centers[i].y);
                }
                ui->rightCaptureLabel->setPixmap(pcopy_1);
            }
        }
    }
}


void MainWindow::calibration()
{
    ui->progressBar->reset();
    nowProgress = 0;
    calibrator = new CameraCalibration();
    calibrator->setSquareSize(cvSize(setDialog->cell_w,setDialog->cell_h));
    calibrator->useSymmetric = ui->useSymmetric->isChecked();
    QString path;

    for(int i = 1; i <= 2; i++)
    {
        path = projectPath + "/calib/";

        if (i == 1){
            path += "left/L";
            calibrator->isleft = true;
        }
        else{
            path += "right/R";
            calibrator->isleft = false;
        }

        //load images
        calibrator->loadCameraImgs(path);
        progressPop(5);
        int reval = calibrator->extractImageCorners();
        if(reval){
            ui->progressBar->reset();
            if(reval != CALIBIMGNUM+1)
                saveCount = reval;
            return;
        }
        progressPop(15);
        if(!calibrator->calibrateCamera())
            return;
        //显示单个相机标定误差
        (i==1)?(ui->leftRMS->setText(QString::number(calibrator->rms))):(ui->rightRMS->setText(QString::number(calibrator->rms)));
        progressPop(10);
        calibrator->findCameraExtrisics();
        progressPop(10);

        //export txt files
        QString file_name;
        path = (i==1)?(projectPath + "/calib/left/"):(projectPath + "/calib/right/");

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
    calibrator->findFundamental();//计算基础矩阵并进行立体标定（若定义）
    calibrator->exportTxtFiles(path.toLocal8Bit(), CAMCALIB_OUT_FUNDAMENTAL);
    path = projectPath + "/calib/H1_mat.txt";
    calibrator->exportTxtFiles(path.toLocal8Bit(), CAMCALIB_OUT_H1);
    path = projectPath + "/calib/H2_mat.txt";
    calibrator->exportTxtFiles(path.toLocal8Bit(), CAMCALIB_OUT_H2);
    path = projectPath + "/calib/status_mat.txt";
    calibrator->exportTxtFiles(path.toLocal8Bit(), CAMCALIB_OUT_STATUS);
#ifdef TEST_STEREO
    ui->StereoRMS->setText(QString::number(calibrator->rms));
    path = projectPath + "/calib/fundamental_stereo.txt";
    calibrator->exportTxtFiles(path.toLocal8Bit(), STEREOCALIB_OUT_F);
    path = projectPath + "/calib/R_stereo.txt";
    calibrator->exportTxtFiles(path.toLocal8Bit(), STEREOCALIB_OUT_R);
    path = projectPath + "/calib/T_stereo.txt";
    calibrator->exportTxtFiles(path.toLocal8Bit(), STEREOCALIB_OUT_T);
    for(int i=1;i<=2;i++){
        QString file_name;
        path=(i==1)?(projectPath + "/calib/left/"):(projectPath + "/calib/right/");
        file_name=path;
        file_name+="cam_stereo.txt";
        calibrator->exportTxtFiles(file_name.toLocal8Bit(), (i==1)?(STEREOCALIB_OUT_MATRIXL):(STEREOCALIB_OUT_MATRIXR));
        file_name=path;
        file_name+="distortion_stereo.txt";
        calibrator->exportTxtFiles(file_name.toLocal8Bit(), (i==1)?(STEREOCALIB_OUT_DISL):(STEREOCALIB_OUT_DISR));
    }
#endif
    ui->progressBar->setValue(100);
}

///-----------------------扫描---------------------------///
void MainWindow::scan()
{
    ui->progressBar->reset();
    nowProgress = 0;

    if(!DHC->cameraOpened){
        QMessageBox::warning(this, tr("Cameras are not Opened"), tr("Cameras are not opened."));
        return;
    }
    if(projectPath==""){
        QMessageBox::warning(this,tr("Save Path hasn't been Set"), tr("You need create a project first."));
        return;
    }
    selectPath(PATHSCAN);
    ui->tabWidget->setCurrentIndex(PATHSCAN);
    ui->findPointButton->setEnabled(true);
    ui->reFindButton->setEnabled(true);
    ui->startScanButton->setEnabled(true);
    pj->setCrossVisable(false);
}

void MainWindow::pointmatch()
{
    dm->blocksize = ui->binarySlider->value();
    findPoint();
}

void MainWindow::refindmatch()
{
    if (dm->scanSN == 0)
        dm->firstFind = true;
    findPoint();
}

void MainWindow::findPoint()
{
    if (dm->dotForMark.size() != 0)
    {
        dm->dotForMark.clear();
    }
    cv::Mat mat_1 = cv::Mat(cameraHeight, cameraWidth, CV_8UC1, DHC->m_pRawBuffer_1);//直接从内存缓冲区获得图像数据是可行的
    cv::Mat mat_2 = cv::Mat(cameraHeight, cameraWidth, CV_8UC1, DHC->m_pRawBuffer_2);
    //imshow("d",mat_1);
    //cvWaitKey(10);
    bool success = dm->matchDot(mat_1,mat_2);

    ///保证运行activeManual时mm已经生成
    if (dm->scanSN != 0){
        if (ui->matchAssistant->isChecked()){
            dm->activeManual();
        }
    }
    else{
        if (success){
            paintPoints();
            if (QMessageBox::information(NULL,tr("Finished"), tr("Is the result right for reconstruction?"),
                QMessageBox::Yes,QMessageBox::No)== QMessageBox::Yes){
                finishmanualmatch();
            }
        }
    }
}

void MainWindow::finishmanualmatch()
{
    scanSN = dm->scanSN;//这里不能放在finishMatch后面
    ui->scanSNLabel->setText(QString::number(scanSN+1));//表示已经进行了的扫描次数（实际是查找点次数）
    dm->finishMatch();
    paintPoints();
}

void MainWindow::paintPoints()
{
    QPixmap pcopy_1 = pimage_1;
    QPixmap pcopy_2 = pimage_2;
    QPainter pt_1(&pcopy_1);
    QPainter pt_2(&pcopy_2);
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
            pt_1.setPen(orangecolor);
            pt_2.setPen(orangecolor);
            drawCross(pt_1, dm->dotForMark[i][0] ,dm->dotForMark[i][1]);
            drawCross(pt_2, dm->dotForMark[i][2], dm->dotForMark[i][3]);
        }
    }
    ui->leftCaptureLabel->setPixmap(pcopy_1);
    ui->rightCaptureLabel->setPixmap(pcopy_2);
}

void MainWindow::startscan()
{
    if (scanSN < 0)
    {
        if (QMessageBox::warning(this,tr("Mark Point Need to be Found"), tr("Scan result can't be aligned,continue?")
                                 ,QMessageBox::Yes,QMessageBox::No) == QMessageBox::No)
            return;
        else
            scanSN = 0;
    }
    ui->progressBar->reset();
    nowProgress = 0;

    DHC->closeCamera();
    timer->stop();
    pj->displaySwitch(false);
    pj->opencvWindow();
    if (ui->useGray->isChecked()){
        grayCode = new GrayCodes(scanWidth,scanHeight);
        grayCode->generateGrays();
        pj->showMatImg(grayCode->getNextImg());
    }
    else{
        mf = new MultiFrequency(this, scanWidth, scanHeight);
        mf->generateMutiFreq();
        pj->showMatImg(mf->getNextMultiFreq());
    }
    progressPop(10);

    int imgCount = 0;

    QString pref = QString::number(scanSN) + "/";
    QDir *addpath_1 = new QDir;
    QDir *addpath_2 = new QDir;
    addpath_1->mkpath(projChildPath + "/left/" + pref);
    addpath_2->mkpath(projChildPath +"/right/" + pref);

    while(true){
        cv::waitKey(100);
        DHC->daHengSnapShot(1);
        image_1 = QImage(DHC->m_pRawBuffer_1, cameraWidth, cameraHeight, QImage::Format_Indexed8);
        pimage_1 = QPixmap::fromImage(image_1);
        DHC->daHengSnapShot(2);
        image_2 = QImage(DHC->m_pRawBuffer_2, cameraWidth, cameraHeight, QImage::Format_Indexed8);
        pimage_2 = QPixmap::fromImage(image_2);

        captureImage(pref, imgCount, false);
        imgCount++;

        if (ui->useGray->isChecked()){
            if(imgCount == grayCode->getNumOfImgs())
                break;
            pj->showMatImg(grayCode->getNextImg());
            progressPop(2);
        }
        else{
            if(imgCount == mf->getNumOfImgs())
                break;
            pj->showMatImg(mf->getNextMultiFreq());
            progressPop(7);
        }
    }
    DHC->openDaHengCamera(cameraWidth,cameraHeight);
    timer->start();
    pj->destoryWindow();
    pj->displaySwitch(true);

    ui->progressBar->setValue(100);
}

void MainWindow::testmulitfreq()
{
    pj->displaySwitch(false);
    pj->opencvWindow();
    MultiFrequency *mf = new MultiFrequency(this, scanWidth, scanHeight);
    mf->generateMutiFreq();
    cv::Mat play = mf->getNextMultiFreq();
    pj->showMatImg(play);
}

///-----------------重建-------------------///
void MainWindow::reconstruct()
{
    ui->progressBar->reset();
    nowProgress = 0;
    ui->tabWidget->setCurrentIndex(2);
    selectPath(PATHRECON);//set current path to :/reconstruct
}

void MainWindow::startreconstruct()
{
    ui->progressBar->reset();
    nowProgress = 0;
    if(DHC->cameraOpened)
        DHC->closeCamera();
    if(isConfigured == false){
        if(QMessageBox::warning(this,tr("Warning"), tr("You may want to change the settings, continue with default settings?"),
                QMessageBox::Yes,QMessageBox::No) == QMessageBox::No)
            return;
        else
            isConfigured = true;
    }

    reconstructor = new Reconstruct();
    reconstructor->scanSN = (ui->manualReconstruction->isChecked())?(ui->reconstructionCount->value()):(scanSN);
    reconstructor->getParameters(scanWidth, scanHeight, cameraWidth, cameraHeight, isAutoContrast, projectPath);

    reconstructor->setCalibPath(projectPath+"/calib/left/", 0);
    reconstructor->setCalibPath(projectPath+"/calib/right/", 1);
    bool loaded = reconstructor->loadCameras();//load camera matrix

    if(!loaded){
        ui->progressBar->reset();
        nowProgress = 0;
        return;
    }
    progressPop(5);

    reconstructor->setBlackThreshold(black_);
    reconstructor->setWhiteThreshold(white_);

    if(isRaySampling)
        reconstructor->enableRaySampling();
    else
        reconstructor->disableRaySampling();
    progressPop(15);

    bool runSucess = reconstructor->runReconstruction();
    if(!runSucess){
        ui->progressBar->reset();
        nowProgress = 0;
        return;
    }
    progressPop(50);

    MeshCreator *meshCreator=new MeshCreator(reconstructor->points3DProjView);//Export mesh
    progressPop(10);

    if(isExportObj)
        meshCreator->exportObjMesh(projChildPath + QString::number(reconstructor->scanSN) + ".obj");

    if(isExportPly || !(isExportObj || isExportPly)){
        QString outPlyPath = projChildPath + QString::number(reconstructor->scanSN) + ".ply";
        meshCreator->exportPlyMesh(outPlyPath);
        displayModel->LoadModel(outPlyPath);
    }
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
    cameraWidth = setDialog->cam_w;
    cameraHeight = setDialog->cam_h;
    projectorWidth = setDialog->proj_w;
    projectorHeight = setDialog->proj_h;
    scanWidth = setDialog->scan_w;
    scanHeight = setDialog->scan_h;
    black_ = setDialog->black_threshold;
    white_ = setDialog->white_threshold;
    isAutoContrast = setDialog->autoContrast;
    isRaySampling = setDialog->raySampling;
    isExportObj = setDialog->exportObj;
    isExportPly = setDialog->exportPly;
}


void MainWindow::createConnections()
{
    connect(ui->actionNew, SIGNAL(triggered()), this, SLOT(newproject()));
    connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(openproject()));

    connect(ui->actionOpenCamera, SIGNAL(triggered()), this, SLOT(opencamera()));
    connect(ui->actionFocusAssistant, SIGNAL(triggered()), this, SLOT(startfocusassistant()));
    connect(ui->leftExSlider,SIGNAL(valueChanged(int)),this,SLOT(setexposure()));
    connect(ui->rightExSlider,SIGNAL(valueChanged(int)),this,SLOT(setexposure()));

    connect(ui->actionBasler, SIGNAL(triggered()), this, SLOT(usebasler()));//暂时用来调试的功能

    connect(ui->actionProjector,SIGNAL(triggered()),this,SLOT(projectorcontrol()));

    connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(selectPath(int)));

    connect(ui->actionCalib, SIGNAL(triggered()), this, SLOT(calib()));
    connect(ui->captureButton, SIGNAL(clicked()), this, SLOT(capturecalib()));
    connect(ui->redoButton, SIGNAL(clicked()), this, SLOT(redocapture()));
    connect(ui->calibButton,SIGNAL(clicked()),this,SLOT(calibration()));

    connect(ui->actionScan,SIGNAL(triggered()),this,SLOT(scan()));
    connect(ui->findPointButton,SIGNAL(clicked()),this,SLOT(pointmatch()));
    connect(ui->reFindButton,SIGNAL(clicked()),this,SLOT(refindmatch()));
    connect(ui->startScanButton, SIGNAL(clicked()), this, SLOT(startscan()));
    connect(ui->multiFreqTest, SIGNAL(clicked()), this, SLOT(testmulitfreq()));

    connect(ui->actionReconstruct,SIGNAL(triggered()),this,SLOT(reconstruct()));
    connect(ui->reconstructionButton,SIGNAL(clicked()),this,SLOT(startreconstruct()));

    connect(ui->actionSet, SIGNAL(triggered()), this, SLOT(set()));
    connect(ui->actionChinese, SIGNAL(triggered()), this, SLOT(switchlanguage()));
    connect(ui->actionEnglish, SIGNAL(triggered()), this, SLOT(switchlanguage()));
    connect(ui->pSizeValue, SIGNAL(valueChanged(int)), this, SLOT(changePointSize(int)));
    connect(ui->loadTest, SIGNAL(clicked()), this, SLOT(loadTestModel()));

    connect(ui->actionExit, SIGNAL(triggered()), pj, SLOT(close()));//解决投影窗口不能关掉的问题
    connect(ui->actionExit, SIGNAL(triggered()), fa, SLOT(close()));
    connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(close()));
}

///------------------辅助功能-------------------///

void MainWindow::generatePath(int type)
{
    selectPath(type);
    QDir *addpath_1 = new QDir;
    QDir *addpath_2 = new QDir;
    if(type == 0 || type == 1){
        addpath_1->mkpath(projChildPath + "/left/");
        addpath_2->mkpath(projChildPath +"/right/");
    }
    else
        addpath_1->mkpath(projChildPath);
}

void MainWindow::selectPath(int PATH)//decide current childpath
{
    QString t;
    switch (PATH) {
        case PATHCALIB:
        t = "/calib/";
        break;
        case PATHSCAN:
        t = "/scan/";
        break;
        case PATHRECON:
        t = "/reconstruction/";
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
    if(screencount == 1)
        QMessageBox::information(this,tr("Info"),tr("Only one screen detected."));
    else{
        QRect screenRect = desktopWidget->screenGeometry(0);
        screenWidth = screenRect.width();
        screenHeight = screenRect.height();
    }
}

void MainWindow::switchlanguage()
{
    QString qmPath = ":/";//表示从资源文件夹中加载
    QString local;
    if(inEnglish){
        local = "zh.pm";
        inEnglish = false;
        ui->actionEnglish->setEnabled(true);
        ui->actionChinese->setDisabled(true);
    }
    else{
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

void MainWindow::loadTestModel()
{
    if (projChildPath != NULL){
        QString testPath = projChildPath + "0.ply";
        displayModel->LoadModel(testPath);
    }
    else{
        QMessageBox::warning(NULL,tr("File Not Found"),tr("Test file doesn't exist."));
        return;
    }
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
