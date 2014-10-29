#include "mainwindow.h"
#include "graycodes.h"
#include "set.h"
//#include "cameracalibration.h"

#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QImage>
#include <QKeySequence>
#include <QToolBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QStatusBar>
#include <QDesktopWidget>
#include <QVector>

const HV_RESOLUTION Resolution = RES_MODE0;
const HV_SNAP_MODE SnapMode = CONTINUATION;
const HV_BAYER_CONVERT_TYPE ConvertType = BAYER2RGB_NEIGHBOUR1;
const HV_SNAP_SPEED SnapSpeed = HIGH_SPEED;
const long ADCLevel           = ADC_LEVEL2;
const int XStart              = 0;//图像左上角点在相机幅面1280X1024上相对于幅面左上角点坐标
const int YStart              = 0;
const int Width               = 1280;//相机视野应大于等于投影幅面
const int Height              = 1024;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    this->resize(1200,800);
    QSizePolicy mainWinSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    mainWinSizePolicy.setHorizontalStretch(0);
    mainWinSizePolicy.setVerticalStretch(0);
    mainWinSizePolicy.setHeightForWidth(this->sizePolicy().hasHeightForWidth());
    this->setSizePolicy(mainWinSizePolicy);
    this->setMinimumSize(QSize(1200, 800));

    createActions();

    QMenu *file = menuBar()->addMenu(tr("&File"));
    file->addAction(newAction);//add newproject action into file menu
    file->addAction(openAction);
    file->addAction(saveAction);
    file->addAction(closeAction);

    QToolBar *toolBar = addToolBar(tr("Fast Access"));
    toolBar->addAction(newAction);
    toolBar->addSeparator();
    toolBar->addAction(openCameraAction);
    toolBar->addSeparator();
    toolBar->addAction(projectorAction);
    toolBar->addSeparator();
    toolBar->addAction(calibAction);
    toolBar->addSeparator();
    toolBar->addAction(scanAction);
    toolBar->addSeparator();
    toolBar->addAction(reconstructAction);
    toolBar->addSeparator();
    toolBar->addAction(setAction);

    msgLabel = new QLabel;
    msgLabel->setMinimumSize(msgLabel->sizeHint());
    msgLabel->setAlignment(Qt::AlignHCenter);
    statusBar()->addWidget(msgLabel);
    statusBar()->setStyleSheet(QString("QStatusBar::item{border: 0px}"));

    createCentralWindow(this);

    saveCon = 1;//Save calib images start with 1
    cameraOpened = false;
    isConfigured = false;
    isProjectorOpened = true;

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(readframe()));
    viewWidth = 320;//Set width that fit the view window
    cameraWidth = Width;
    cameraHeight = Height;

    sWindow = new Set(this);//Initialize the set dialog
    getSetInfo();
    getScreenGeometry();//Get mian screen and projector screen geometry
    QDesktopWidget* desktopWidget = QApplication::desktop();
    QRect projRect = desktopWidget->screenGeometry(1);//1 represent projector
    int xOffSet = (projRect.width()-projectorWidth)/2 + screenWidth;
    int yOffSet = (projRect.height()-projectorHeight)/2;

    pj = new Projector(NULL, projectorWidth, projectorHeight, xOffSet, yOffSet);//Initialize the projector
    pj->move(screenWidth,0);//make the window displayed by the projector
    pj->showFullScreen();
    connect(closeAction,SIGNAL(triggered()),pj,SLOT(close()));//解决投影窗口不能关掉的问题
    connect(closeAction,SIGNAL(triggered()),this,SLOT(close()));

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

void MainWindow::createCentralWindow(QWidget *parent)
{
    viewWindow = new QWidget(parent);
    setCentralWidget(viewWindow);
    viewWindow->autoFillBackground();

    viewWindow->resize(964, 715);
    QSizePolicy viewWinsizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    viewWinsizePolicy.setHorizontalStretch(0);
    viewWinsizePolicy.setVerticalStretch(0);
    viewWinsizePolicy.setHeightForWidth(viewWindow->sizePolicy().hasHeightForWidth());
    viewWindow->setSizePolicy(viewWinsizePolicy);
    viewWindow->setAutoFillBackground(true);
    gridLayout = new QGridLayout(viewWindow);
    gridLayout->setSpacing(1);
    gridLayout->setContentsMargins(1, 1, 1, 1);

    toolBox = new QToolBox(viewWindow);
    QSizePolicy toolBoxSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    toolBoxSizePolicy.setHorizontalStretch(0);
    toolBoxSizePolicy.setVerticalStretch(0);
    toolBoxSizePolicy.setHeightForWidth(toolBox->sizePolicy().hasHeightForWidth());
    toolBox->setSizePolicy(toolBoxSizePolicy);
    toolBox->setMinimumSize(QSize(240, 470));
    toolBox->setMaximumSize(QSize(240, 470));
    toolBox->setAutoFillBackground(false);
    toolBox->setStyleSheet(QStringLiteral("background-color:lavender;"));

    cameraAdjustPage = new QWidget();
    cameraAdjustPage->setObjectName(QStringLiteral("cameraAdjustPage"));
    cameraAdjustPage->setGeometry(QRect(0, 0, 240, 366));
    gridLayout_6 = new QGridLayout(cameraAdjustPage);
    groupBox = new QGroupBox(cameraAdjustPage);
    formLayout = new QFormLayout(groupBox);
    lightLabel = new QLabel(groupBox);
    lightLabel->setText(tr("Light"));
    formLayout->setWidget(0, QFormLayout::LabelRole, lightLabel);
    lightSlider1 = new QSlider(groupBox);
    lightSlider1->setValue(49);
    lightSlider1->setOrientation(Qt::Horizontal);
    formLayout->setWidget(0, QFormLayout::FieldRole, lightSlider1);
    speedLabel = new QLabel(groupBox);
    speedLabel->setText(tr("Speed"));
    formLayout->setWidget(1, QFormLayout::LabelRole, speedLabel);
    speedSlider1 = new QSlider(groupBox);
    speedSlider1->setValue(49);
    speedSlider1->setOrientation(Qt::Horizontal);
    formLayout->setWidget(1, QFormLayout::FieldRole, speedSlider1);
    gridLayout_6->addWidget(groupBox, 0, 0, 1, 1);
    groupBox_2 = new QGroupBox(cameraAdjustPage);
    formLayout_2 = new QFormLayout(groupBox_2);
    lightLabel_2 = new QLabel(groupBox_2);
    lightLabel_2->setText(tr("Light"));
    formLayout_2->setWidget(0, QFormLayout::LabelRole, lightLabel_2);
    lightSlider_2 = new QSlider(groupBox_2);
    lightSlider_2->setValue(49);
    lightSlider_2->setOrientation(Qt::Horizontal);
    formLayout_2->setWidget(0, QFormLayout::FieldRole, lightSlider_2);
    label_2 = new QLabel(groupBox_2);
    label_2->setText(tr("Speed"));
    formLayout_2->setWidget(1, QFormLayout::LabelRole, label_2);
    speedSlider_2 = new QSlider(groupBox_2);
    speedSlider_2->setValue(49);
    speedSlider_2->setOrientation(Qt::Horizontal);
    formLayout_2->setWidget(1, QFormLayout::FieldRole, speedSlider_2);
    gridLayout_6->addWidget(groupBox_2, 1, 0, 1, 1);
    toolBox->addItem(cameraAdjustPage, QStringLiteral("Camera Adjust"));

    calibrationpage = new QWidget();
    calibrationpage->setGeometry(QRect(0, 0, 241, 349));
    verticalLayout = new QVBoxLayout(calibrationpage);
    label_4 = new QLabel(calibrationpage);
    label_4->setText(tr("<html><head/><body><p>Put the calibration board as follows:</p></body></html>"));
    label_4->setFrameShape(QFrame::NoFrame);
    label_4->setFrameShadow(QFrame::Plain);
    verticalLayout->addWidget(label_4);
    verticalSpacer_9 = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding);
    verticalLayout->addItem(verticalSpacer_9);
    explainLabel = new QLabel(calibrationpage);
    explainLabel->setText(tr("Explain"));
    verticalLayout->addWidget(explainLabel);
    verticalSpacer_10 = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    verticalLayout->addItem(verticalSpacer_10);
    horizontalLayout_3 = new QHBoxLayout();
    label_8 = new QLabel(calibrationpage);
    label_8->setText(tr("<html><head/><body><p>Number of photos been taken:</p></body></html>"));
    horizontalLayout_3->addWidget(label_8);
    currentPhotoLabel = new QLabel(calibrationpage);
    currentPhotoLabel->setText(tr("0/12"));
    horizontalLayout_3->addWidget(currentPhotoLabel);
    verticalLayout->addLayout(horizontalLayout_3);
    treeWidget = new QTreeWidget(calibrationpage);
    QTreeWidgetItem *__qtreewidgetitem = new QTreeWidgetItem();
    __qtreewidgetitem->setText(0, QStringLiteral("Captured Images for Calibration"));
    treeWidget->setHeaderItem(__qtreewidgetitem);
    verticalLayout->addWidget(treeWidget);
    //////////////////////////////////////
    captureButton = new QPushButton(calibrationpage);
    captureButton->setText(tr("Capture"));
    verticalLayout->addWidget(captureButton);
    connect(captureButton,SIGNAL(clicked()),this,SLOT(capturecalib()));
    calibButton = new QPushButton(calibrationpage);
    calibButton->setText(tr("Calibration"));
    calibButton->setEnabled(false);
    verticalLayout->addWidget(calibButton);
    connect(calibButton,SIGNAL(clicked()),this,SLOT(calibration()));
    toolBox->addItem(calibrationpage, QStringLiteral("Calibration"));

    scanPage = new QWidget();
    scanPage->setObjectName(QStringLiteral("scanPage"));
    scanPage->setGeometry(QRect(0, 0, 240, 366));
    verticalLayout_2 = new QVBoxLayout(scanPage);
    treeWidget_2 = new QTreeWidget(scanPage);
    QTreeWidgetItem *__qtreewidgetitem1 = new QTreeWidgetItem();
    __qtreewidgetitem1->setText(0, QStringLiteral("Captured Images for Reconstruction"));
    treeWidget_2->setHeaderItem(__qtreewidgetitem1);
    verticalLayout_2->addWidget(treeWidget_2);
    scanButton = new QPushButton(scanPage);
    scanButton->setText(tr("Scan"));
    verticalLayout_2->addWidget(scanButton);
    toolBox->addItem(scanPage, QStringLiteral("Scan"));

    reconstructionPage = new QWidget();
    reconstructionPage->setObjectName(QStringLiteral("reconstructionPage"));
    reconstructionPage->setGeometry(QRect(0, 0, 240, 366));
    formLayout_3 = new QFormLayout(reconstructionPage);
    horizontalLayout_4 = new QHBoxLayout();
    objButton = new QPushButton(reconstructionPage);
    objButton->setText(tr("Export OBJ"));
    horizontalLayout_4->addWidget(objButton);
    plyButton = new QPushButton(reconstructionPage);
    plyButton->setText(tr("Export PLY"));
    horizontalLayout_4->addWidget(plyButton);
    formLayout_3->setLayout(0, QFormLayout::LabelRole, horizontalLayout_4);
    toolBox->addItem(reconstructionPage, QStringLiteral("Reconstruction"));

    gridLayout->addWidget(toolBox, 0, 1, 1, 1);

    horizontalLayout = new QHBoxLayout();
    horizontalLayout->setSpacing(0);

    leftView = new QWidget(viewWindow);
    QSizePolicy lVsizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    lVsizePolicy.setHeightForWidth(leftView->sizePolicy().hasHeightForWidth());
    leftView->setSizePolicy(lVsizePolicy);
    leftView->setMinimumSize(QSize(240, 240));
    leftView->setStyleSheet("QWidget {border:1px solid deepskyblue;border-style: outset;}");

    QPalette palette;
    QBrush brush(QColor(0, 0, 0, 255));
    brush.setStyle(Qt::SolidPattern);
    palette.setBrush(QPalette::Active, QPalette::WindowText, brush);
    leftView->setPalette(palette);
    leftView->setAutoFillBackground(true);
    gridLayout_2 = new QGridLayout(leftView);
    gridLayout_2->setSpacing(0);
    gridLayout_2->setContentsMargins(0, 0, 0, 0);
    verticalSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    gridLayout_2->addItem(verticalSpacer, 0, 1, 1, 1);
    horizontalSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    gridLayout_2->addItem(horizontalSpacer, 1, 0, 1, 1);

    leftViewLabel = new QLabel(leftView);
    leftViewLabel->setText(tr("Left View"));
    gridLayout_2->addWidget(leftViewLabel, 1, 1, 1, 1);
    horizontalSpacer_2 = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    gridLayout_2->addItem(horizontalSpacer_2, 1, 2, 1, 1);
    verticalSpacer_2 = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    gridLayout_2->addItem(verticalSpacer_2, 2, 1, 1, 1);
    horizontalLayout->addWidget(leftView);

    leftCapture = new QWidget(viewWindow);
    QSizePolicy lCsizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    lCsizePolicy.setHeightForWidth(leftCapture->sizePolicy().hasHeightForWidth());
    leftCapture->setSizePolicy(lCsizePolicy);
    leftCapture->setMinimumSize(QSize(240, 240));
    leftCapture->setStyleSheet("QWidget {border:1px solid deepskyblue;border-style:outset;}");
    QPalette palette1;
    palette1.setBrush(QPalette::Active, QPalette::WindowText, brush);
    leftCapture->setPalette(palette1);
    leftCapture->setAutoFillBackground(true);
    gridLayout_3 = new QGridLayout(leftCapture);
    gridLayout_3->setSpacing(0);
    gridLayout_3->setContentsMargins(0, 0, 0, 0);
    verticalSpacer_3 = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    gridLayout_3->addItem(verticalSpacer_3, 0, 1, 1, 1);
    horizontalSpacer_4 = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    gridLayout_3->addItem(horizontalSpacer_4, 1, 0, 1, 1);
    leftCaptureLabel = new QLabel(leftCapture);
    leftCaptureLabel->setText(tr("Left Capture"));
    gridLayout_3->addWidget(leftCaptureLabel, 1, 1, 1, 1);
    horizontalSpacer_3 = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    gridLayout_3->addItem(horizontalSpacer_3, 1, 2, 1, 1);
    verticalSpacer_4 = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    gridLayout_3->addItem(verticalSpacer_4, 2, 1, 1, 1);
    horizontalLayout->addWidget(leftCapture);

    rightCapture = new QWidget(viewWindow);
    rightCapture->setObjectName(QStringLiteral("rightCapture"));
    QSizePolicy rCsizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    rCsizePolicy.setHeightForWidth(rightCapture->sizePolicy().hasHeightForWidth());
    rightCapture->setSizePolicy(rCsizePolicy);
    rightCapture->setMinimumSize(QSize(240, 240));
    rightCapture->setStyleSheet("QWidget {border:1px solid deepskyblue;border-style: outset;}");
    QPalette palette2;
    palette2.setBrush(QPalette::Active, QPalette::WindowText, brush);
    rightCapture->setPalette(palette2);
    rightCapture->setAutoFillBackground(true);
    gridLayout_4 = new QGridLayout(rightCapture);
    gridLayout_4->setSpacing(0);
    gridLayout_4->setContentsMargins(0, 0, 0, 0);
    verticalSpacer_5 = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    gridLayout_4->addItem(verticalSpacer_5, 0, 1, 1, 1);
    horizontalSpacer_6 = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    gridLayout_4->addItem(horizontalSpacer_6, 1, 0, 1, 1);
    rightCaptureLabel = new QLabel(rightCapture);
    rightCaptureLabel->setText(tr("Right Capture"));
    gridLayout_4->addWidget(rightCaptureLabel, 1, 1, 1, 1);
    horizontalSpacer_5 = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    gridLayout_4->addItem(horizontalSpacer_5, 1, 2, 1, 1);
    verticalSpacer_6 = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    gridLayout_4->addItem(verticalSpacer_6, 2, 1, 1, 1);
    horizontalLayout->addWidget(rightCapture);

    rightView = new QWidget(viewWindow);
    rightView->setObjectName(QStringLiteral("rightView"));
    QSizePolicy rVsizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    rVsizePolicy.setHeightForWidth(rightView->sizePolicy().hasHeightForWidth());
    rightView->setSizePolicy(rVsizePolicy);
    rightView->setMinimumSize(QSize(240, 240));
    rightView->setStyleSheet("QWidget {border:1px solid deepskyblue}");
    QPalette palette3;
    palette3.setBrush(QPalette::Active, QPalette::WindowText, brush);
    rightView->setPalette(palette3);
    rightView->setAutoFillBackground(true);
    gridLayout_5 = new QGridLayout(rightView);
    gridLayout_5->setSpacing(0);
    gridLayout_5->setContentsMargins(0, 0, 0, 0);
    verticalSpacer_7 = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    gridLayout_5->addItem(verticalSpacer_7, 0, 1, 1, 1);
    horizontalSpacer_8 = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    gridLayout_5->addItem(horizontalSpacer_8, 1, 0, 1, 1);
    rightViewLabel = new QLabel(rightView);
    rightViewLabel->setText(tr("Right View"));
    gridLayout_5->addWidget(rightViewLabel, 1, 1, 1, 1);
    horizontalSpacer_7 = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    gridLayout_5->addItem(horizontalSpacer_7, 1, 2, 1, 1);
    verticalSpacer_8 = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    gridLayout_5->addItem(verticalSpacer_8, 2, 1, 1, 1);
    horizontalLayout->addWidget(rightView);
    gridLayout->addLayout(horizontalLayout, 1, 0, 1, 2);

    threeDView = new QWidget(viewWindow);
    threeDView->setObjectName(QStringLiteral("threeDView"));
    QSizePolicy tdPolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    tdPolicy.setHeightForWidth(threeDView->sizePolicy().hasHeightForWidth());
    threeDView->setSizePolicy(tdPolicy);
    threeDView->setMinimumSize(QSize(730, 470));
    threeDView->setStyleSheet("QWidget {border:1px solid mediumpurple;}");
    QPalette palette4;
    palette4.setBrush(QPalette::Active, QPalette::WindowText, brush);
    QBrush brush22(QColor(0, 255, 255, 255));
    brush22.setStyle(Qt::SolidPattern);
    palette4.setBrush(QPalette::Active, QPalette::Button, brush22);
    QBrush brush23(QColor(127, 255, 255, 255));
    brush23.setStyle(Qt::SolidPattern);
    palette4.setBrush(QPalette::Active, QPalette::Light, brush23);
    QBrush brush24(QColor(63, 255, 255, 255));
    brush24.setStyle(Qt::SolidPattern);
    palette4.setBrush(QPalette::Active, QPalette::Midlight, brush24);
    QBrush brush25(QColor(0, 127, 127, 255));
    brush25.setStyle(Qt::SolidPattern);
    palette4.setBrush(QPalette::Active, QPalette::Dark, brush25);
    QBrush brush26(QColor(0, 170, 170, 255));
    brush26.setStyle(Qt::SolidPattern);
    threeDView->setPalette(palette4);
    threeDView->setAutoFillBackground(true);
    toolBox->raise();
    toolBox->raise();
    gridLayout->addWidget(threeDView, 0, 0, 1, 1);
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

    openCameraAction->setDisabled(true);//暂时保证不会启动两次，防止内存溢出
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
    QPixmap pms_1 = pimage_1.scaledToWidth(viewWidth);
    QPixmap pms_2 = pimage_2.scaledToWidth(viewWidth);
    leftViewLabel->setPixmap(pms_1);//use label to show the image
    rightViewLabel->setPixmap(pms_2);
}

void MainWindow::capturecalib()
{
    if(cameraOpened){
        selectPath(0);//0 for calibration and 1 for scan
        captureImage(saveCon,true);
        currentPhotoLabel->setText(QString::number(saveCon) + "/12");
        saveCon++;
        if(saveCon == 13){
            saveCon = 1;
            calibButton->setEnabled(true);
        }
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
        QPixmap pms_1 = pimage_1.scaledToWidth(viewWidth);
        QPixmap pms_2 = pimage_2.scaledToWidth(viewWidth);
        leftCaptureLabel->setPixmap(pms_1);
        rightCaptureLabel->setPixmap(pms_2);
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
        //QString str;
        //str = "Screen: Width " + QString::number(screenWidth) +", Height " + QString::number(screenHeight) + "; Projector: Width " + QString::number(projectorWidth) + ", Height " +  QString::number(projectorHeight);
        //QMessageBox::information(NULL,"Geometry",str);
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
        projectorWidth = 800;//保证投影区域完全被拍到,这里采用了手动设置，也可以考虑加入set对话框
        projectorHeight = 600;
    }
}

void MainWindow::calib()
{
    QMessageBox::information(NULL, tr("Calibration"), tr("Calibration Actived!"));
    toolBox->setCurrentIndex(1);//go to calibration page
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
    toolBox->setCurrentIndex(2);
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
    toolBox->setCurrentIndex(3);
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
    connect(sWindow->okButton,SIGNAL(clicked()),this,SLOT(getSetInfo()));
}

void MainWindow::getSetInfo()
{
    projectorWidth = sWindow->proj_w;
    projectorHeight = sWindow->proj_h;
    black_ = sWindow->black_threshold;
    white_ = sWindow->white_threshold;
    isAutoContrast = sWindow->autoContrast;
    isSaveAutoContrast = sWindow->saveAutoContrast;
    isRaySampling = sWindow->raySampling;
    isExportObj = sWindow->exportObj;
    isExportPly = sWindow->exportPly;
}

void MainWindow::createActions()
{
    newAction = new QAction(tr("&New Project"), this); //QAction(const QString &text, QObject* parent);
    newAction->setShortcut(QKeySequence::New);
    newAction->setStatusTip(tr("Create a project."));//Indicate the status information.
    newAction->setIcon(QIcon(":/new.png"));
    connect(newAction, SIGNAL(triggered()), this, SLOT(newproject()));

    openAction = new QAction(tr("&Open Project"), this); //QAction(const QString &text, QObject* parent);
    openAction->setShortcut(QKeySequence::Open);
    openAction->setStatusTip(tr("Open a project."));
    openAction->setIcon(QIcon(":/open.png"));
    connect(openAction, SIGNAL(triggered()), this, SLOT(openproject()));

    saveAction = new QAction(tr("&Save Project"), this); //QAction(const QString &text, QObject* parent);
    saveAction->setShortcut(QKeySequence::Save);
    saveAction->setStatusTip(tr("Save current project."));
    saveAction->setIcon(QIcon(":/save.png"));

    closeAction = new QAction(tr("&Close Project"), this); //QAction(const QString &text, QObject* parent);
    closeAction->setShortcut(QKeySequence::Close);
    closeAction->setStatusTip(tr("Close current project."));
    closeAction->setIcon(QIcon(":/close.png"));

    openCameraAction = new QAction(tr("Open Camera"),this);
    openCameraAction->setStatusTip(tr("Open cameras."));
    openCameraAction->setIcon(QIcon(":/camera.png"));
    connect(openCameraAction, SIGNAL(triggered()), this, SLOT(opencamera()));

    projectorAction = new QAction(tr("Projector Control"),this);
    projectorAction->setStatusTip("Turn on/off the projector.");
    projectorAction->setIcon(QIcon(":/projector.png"));
    connect(projectorAction,SIGNAL(triggered()),this,SLOT(projectorcontrol()));

    calibAction = new QAction(tr("Calibration"), this); //QAction(const QString &text, QObject* parent);
    calibAction->setStatusTip(tr("Calibrate the cameras and the projecter."));
    calibAction->setIcon(QIcon(":/calib.png"));
    connect(calibAction, SIGNAL(triggered()), this, SLOT(calib()));

    scanAction = new QAction(tr("Scan"),this);
    scanAction->setStatusTip(tr("Active scan process."));
    scanAction->setIcon(QIcon(":/scan.png"));
    connect(scanAction,SIGNAL(triggered()),this,SLOT(scan()));

    reconstructAction = new QAction(tr("Reconstruction"),this);
    reconstructAction->setStatusTip(tr("Active reconstruction."));
    reconstructAction->setIcon(QIcon(":/reconstruct.png"));
    connect(reconstructAction,SIGNAL(triggered()),this,SLOT(reconstruct()));

    setAction = new QAction(tr("Setting"), this); //QAction(const QString &text, QObject* parent);
    setAction->setStatusTip(tr("Set parameters of the process."));
    setAction->setIcon(QIcon(":/set.png"));
    connect(setAction, SIGNAL(triggered()), this, SLOT(set()));
}
