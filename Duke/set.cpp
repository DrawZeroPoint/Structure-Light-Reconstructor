#include "set.h"
#include <QtGui>
#include <QDialog>
#include <QMessageBox>
#include <QLayout>
#include <QPushButton>
#include <QApplication>
#include <QChar>

Set::Set(QMainWindow *parent) : QDialog(parent,Qt::Dialog|Qt::CustomizeWindowHint)
{
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(this->sizePolicy().hasHeightForWidth());
    this->setSizePolicy(sizePolicy);

    settingTab = new QTabWidget(this);//这里已经生成了settingTab
    settingTab->setObjectName(QStringLiteral("settingTab"));
    settingTab->setAutoFillBackground(true);

    tab = new QWidget();
    formLayout = new QFormLayout(tab);
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    boardSizeBox = new QGroupBox(tab);
    formLayout_4 = new QFormLayout(boardSizeBox);
    formLayout_3 = new QFormLayout();
    label_2 = new QLabel(boardSizeBox);
    label_2->setText(tr("Grid Width(mm);"));
    formLayout_3->setWidget(1, QFormLayout::LabelRole, label_2);
    label_3 = new QLabel(boardSizeBox);
    label_3->setText(tr("Grid Height(mm):"));
    formLayout_3->setWidget(2, QFormLayout::LabelRole, label_3);

    boardWidth = new QSpinBox(boardSizeBox);
    boardWidth->setValue(15);
    formLayout_3->setWidget(1, QFormLayout::FieldRole, boardWidth);
    boardHeight = new QSpinBox(boardSizeBox);
    boardHeight->setValue(15);
    formLayout_3->setWidget(2, QFormLayout::FieldRole, boardHeight);
    formLayout_4->setLayout(0, QFormLayout::LabelRole, formLayout_3);
    formLayout->setWidget(0, QFormLayout::SpanningRole, boardSizeBox);
    settingTab->addTab(tab, QString(tr("Calibration Board")));

    tab_2 = new QWidget();
    formLayout_8 = new QFormLayout(tab_2);
    gridSizeBox = new QGroupBox(tab_2);
    gridSizeBox->setTitle("Grid width and height");
    formLayout_6 = new QFormLayout(gridSizeBox);
    gridLayout_3 = new QGridLayout();

    projGridWidth = new QSpinBox(gridSizeBox);
    projGridWidth->setValue(15);
    gridLayout_3->addWidget(projGridWidth, 1, 1, 1, 1);
    label_4 = new QLabel(gridSizeBox);
    label_4->setText(tr("Grid Width(mm):"));
    gridLayout_3->addWidget(label_4, 1, 0, 1, 1);
    label_5 = new QLabel(gridSizeBox);
    label_5->setText(tr("Grid Height(mm):"));
    gridLayout_3->addWidget(label_5, 2, 0, 1, 1);

    projGridHeight = new QSpinBox(gridSizeBox);
    projGridHeight->setValue(15);
    gridLayout_3->addWidget(projGridHeight, 2, 1, 1, 1);
    formLayout_6->setLayout(0, QFormLayout::LabelRole, gridLayout_3);
    horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    formLayout_6->setItem(0, QFormLayout::FieldRole, horizontalSpacer_2);
    formLayout_8->setWidget(0, QFormLayout::SpanningRole, gridSizeBox);

    pattenNumBox = new QGroupBox(tab_2);
    pattenNumBox->setTitle("Grid number in X and Y direction");
    formLayout_2 = new QFormLayout(pattenNumBox);
    gridLayout_2 = new QGridLayout();

    label_6 = new QLabel(pattenNumBox);
    label_6->setText("X Grid Number:");
    gridLayout_2->addWidget(label_6, 0, 0, 1, 1);
    xGridNum = new QSpinBox(pattenNumBox);
    xGridNum->setMinimum(3);
    xGridNum->setValue(10);
    gridLayout_2->addWidget(xGridNum, 0, 1, 1, 1);

    label_7 = new QLabel(pattenNumBox);
    label_7->setText("Y Grid Number:");
    gridLayout_2->addWidget(label_7, 1, 0, 1, 1);
    yGridNum = new QSpinBox(pattenNumBox);
    yGridNum->setMinimum(3);
    yGridNum->setValue(9);
    gridLayout_2->addWidget(yGridNum, 1, 1, 1, 1);

    formLayout_2->setLayout(0, QFormLayout::LabelRole, gridLayout_2);
    horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    formLayout_2->setItem(0, QFormLayout::FieldRole, horizontalSpacer_3);
    formLayout_8->setWidget(1, QFormLayout::SpanningRole, pattenNumBox);

    projGeomBox = new QGroupBox(tab_2);
    projGeomBox->setTitle("Width and height of projected patten");
    geomFormLayout = new QFormLayout(projGeomBox);
    geomGridLayout = new QGridLayout();

    widthLabel = new QLabel(projGeomBox);
    widthLabel->setText("Width of project window(pixel):");
    geomGridLayout->addWidget(widthLabel, 0, 0, 1, 1);
    widthSpinBox = new QSpinBox(projGeomBox);
    widthSpinBox->setMinimum(0);
    widthSpinBox->setMaximum(1024);
    widthSpinBox->setValue(800);
    geomGridLayout->addWidget(widthSpinBox, 0, 1, 1, 1);

    heightLabel = new QLabel(projGeomBox);
    heightLabel->setText("Height of project window(pixel):");
    geomGridLayout->addWidget(heightLabel, 1, 0, 1, 1);
    heightSpinBox = new QSpinBox(projGeomBox);
    heightSpinBox->setMinimum(0);
    heightSpinBox->setMaximum(800);
    heightSpinBox->setValue(600);
    geomGridLayout->addWidget(heightSpinBox, 1, 1, 1, 1);

    geomFormLayout->setLayout(0, QFormLayout::LabelRole, geomGridLayout);
    geomSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    geomFormLayout->setItem(0, QFormLayout::FieldRole, geomSpacer);
    formLayout_8->setWidget(2, QFormLayout::SpanningRole, projGeomBox);

    settingTab->addTab(tab_2, QString(tr("Projector")));

    tab_3 = new QWidget();
    gridLayout_5 = new QGridLayout(tab_3);
    groupBox_2 = new QGroupBox(tab_3);
    gridLayout_6 = new QGridLayout(groupBox_2);
    formLayout_5 = new QFormLayout();
    gridLayout_4 = new QGridLayout();
    label_8 = new QLabel(groupBox_2);
    label_8->setText(tr("Black Threshold"));
    gridLayout_4->addWidget(label_8, 0, 0, 1, 1);
    blackThresholdEdit = new QSpinBox(groupBox_2);
    blackThresholdEdit->setObjectName(QStringLiteral("blackThresholdEdit"));
    blackThresholdEdit->setMaximum(256);
    blackThresholdEdit->setValue(40);
    gridLayout_4->addWidget(blackThresholdEdit, 0, 1, 1, 1);
    label_9 = new QLabel(groupBox_2);
    label_9->setText(tr("White Threshold"));
    gridLayout_4->addWidget(label_9, 1, 0, 1, 1);
    whiteThresholdEdit = new QSpinBox(groupBox_2);
    whiteThresholdEdit->setObjectName(QStringLiteral("whiteThresholdEdit"));
    whiteThresholdEdit->setMaximum(256);
    gridLayout_4->addWidget(whiteThresholdEdit, 1, 1, 1, 1);
    formLayout_5->setLayout(0, QFormLayout::LabelRole, gridLayout_4);
    horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    formLayout_5->setItem(0, QFormLayout::FieldRole, horizontalSpacer_4);
    gridLayout_6->addLayout(formLayout_5, 0, 0, 1, 1);
    gridLayout_5->addWidget(groupBox_2, 0, 0, 1, 1);
    gridLayout = new QGridLayout();
    autoContrastCheck = new QCheckBox(tab_3);
    autoContrastCheck->setText(tr("Auto Contrast"));
    autoContrastCheck->setChecked(false);
    gridLayout->addWidget(autoContrastCheck, 0, 0, 1, 1);
    saveAutoContrastImagesCheck = new QCheckBox(tab_3);
    saveAutoContrastImagesCheck->setText(tr("Save Auto Contrast Images"));
    gridLayout->addWidget(saveAutoContrastImagesCheck, 1, 0, 1, 1);
    raySamplingCheck = new QCheckBox(tab_3);
    raySamplingCheck->setText(tr("Ray Sampling"));
    raySamplingCheck->setChecked(true);
    gridLayout->addWidget(raySamplingCheck, 2, 0, 1, 1);
    gridLayout_5->addLayout(gridLayout, 1, 0, 1, 1);
    settingTab->addTab(tab_3, QString(tr("Reconstruction")));

    tab_4 = new QWidget();
    formLayout_12 = new QFormLayout(tab_4);
    formLayout_12->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    groupBox = new QGroupBox(tab_4);
    formLayout_11 = new QFormLayout(groupBox);
    exportObjCheck = new QCheckBox(groupBox);
    exportObjCheck->setText(tr("Export Obj"));
    exportObjCheck->setChecked(true);
    formLayout_11->setWidget(0, QFormLayout::LabelRole, exportObjCheck);
    horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    formLayout_11->setItem(0, QFormLayout::FieldRole, horizontalSpacer);
    exportPlyCheck = new QCheckBox(groupBox);
    exportPlyCheck->setText(tr("Export Ply"));
    exportPlyCheck->setChecked(true);
    formLayout_11->setWidget(1, QFormLayout::LabelRole, exportPlyCheck);
    formLayout_12->setWidget(0, QFormLayout::LabelRole, groupBox);
    horizontalSpacer_6 = new QSpacerItem(176, 74, QSizePolicy::Expanding, QSizePolicy::Minimum);
    formLayout_12->setItem(0, QFormLayout::FieldRole, horizontalSpacer_6);
    settingTab->addTab(tab_4, QString(tr("Export")));

    okButton = new QPushButton(tr("OK"));
    okButton->setDefault(true);

    cancelButton = new QPushButton(tr("Cancel"));
    QHBoxLayout *button = new QHBoxLayout;
    button->addWidget(okButton);
    button->addWidget(cancelButton);

    QVBoxLayout *lay = new QVBoxLayout;
    lay->addWidget(settingTab);
    lay->addLayout(button);
    setLayout(lay);

    this->setGeometry(300,300,500,300);//对set窗口的大小进行设置

    connect(okButton, SIGNAL(clicked()), this, SLOT(createConfigurationFile()));
    connect(okButton,SIGNAL(clicked()),this,SLOT(hide()));
    connect(cancelButton,SIGNAL(clicked()),this,SLOT(hide()));
}
void Set::test(bool flag)
{
    if(flag == true)
        QMessageBox::information(NULL, tr("Test"), tr("Successed!"));
    else
        QMessageBox::warning(NULL, tr("Test"), tr("Failed!"));
}

void Set::createConfigurationFile()//如果是槽函数，那么void声明不可少
{
    board_w = boardWidth->value();
    board_h = boardHeight->value();
    proj_w = widthSpinBox->value();
    proj_h = heightSpinBox->value();
    projGrid_w = projGridWidth->value();
    projGrid_h = projGridHeight->value();
    projectorWinPos_x = xGridNum->value();
    projectorWinPos_y = yGridNum->value();
    black_threshold = blackThresholdEdit->value();
    white_threshold = whiteThresholdEdit->value();
    if(autoContrastCheck->isChecked() == true)
        autoContrast = true;
    else
        autoContrast = false;
    if(saveAutoContrastImagesCheck->isChecked() == true)
        saveAutoContrast = true;
    else
        saveAutoContrast = false;
    if(raySamplingCheck->isChecked() == true)
        raySampling = true;
    else
        raySampling = false;
    if(exportObjCheck->isChecked() == true)
        exportObj = 1;
    else
        exportObj = 0;
    if(exportPlyCheck->isChecked() == true)
        exportPly = 1;
    else
        exportPly = 0;
    createSetFile();
}

void Set::createSetFile()
{
    int autoc, autocs, ray;
    autoc = boolToInt(autoContrast);
    autocs = boolToInt(saveAutoContrast);
    ray = boolToInt(raySampling);
    const QString &fileName = saveSetPath +"/set.xml";
    QFile file(fileName);
    file.open(QIODevice::WriteOnly);
    QXmlStreamWriter xmlWriter(&file);
    xmlWriter.setAutoFormatting(true);
    xmlWriter.writeStartDocument();//写入<?xml version="1.0" encoding="UTF-8"?>
    xmlWriter.writeStartElement("Settings");
    xmlWriter.writeStartElement("ProjectedPatten");
    xmlWriter.writeTextElement("GridWidth",QString::number(proj_w, 10));
    xmlWriter.writeTextElement("GridHeight",QString::number(proj_h, 10));
    xmlWriter.writeEndElement();
    xmlWriter.writeStartElement("CalibrationBoard");
    xmlWriter.writeTextElement("GridWidth",QString::number(board_w, 10));
    xmlWriter.writeTextElement("GridHeight",QString::number(board_h, 10));
    xmlWriter.writeEndElement();
    xmlWriter.writeStartElement("ProjectorWindow");
    xmlWriter.writeStartElement("Position");
    xmlWriter.writeTextElement("X",QString::number(projectorWinPos_x, 10));
    xmlWriter.writeTextElement("Y",QString::number(projectorWinPos_y, 10));
    xmlWriter.writeEndElement();//由于start两次所以end两次
    xmlWriter.writeEndElement();
    xmlWriter.writeStartElement("Reconstruction");
    xmlWriter.writeTextElement("AutoContrast",QString::number(autoc, 10));
    xmlWriter.writeTextElement("SaveAutoContrastImages",QString::number(autocs, 10));
    xmlWriter.writeTextElement("RaySampling",QString::number(ray, 10));
    xmlWriter.writeTextElement("BlackThreshold",QString::number(black_threshold, 10));
    xmlWriter.writeTextElement("WhiteThreshold",QString::number(white_threshold, 10));
    xmlWriter.writeEndElement();
    xmlWriter.writeStartElement("Export");
    xmlWriter.writeTextElement("Obj",QString::number(exportObj, 10));
    xmlWriter.writeTextElement("Ply",QString::number(exportPly, 10));
    xmlWriter.writeEndElement();
    xmlWriter.writeEndDocument();//写入</Settings>
    file.close();
    if(file.error()){
        test(false);
    }
}

int Set::boolToInt(bool input)
{
    if(input)
        return 1;
    else
        return 0;
}

