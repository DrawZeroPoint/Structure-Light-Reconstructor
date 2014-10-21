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
    this->resize(340, 261);//对set窗口的大小进行设置
    QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(this->sizePolicy().hasHeightForWidth());
    this->setSizePolicy(sizePolicy);
    this->setMinimumSize(QSize(340, 261));
    this->setMaximumSize(QSize(340, 261));

    settingTab = new QTabWidget(this);//这里已经生成了settingTab
    settingTab->setObjectName(QStringLiteral("settingTab"));
    settingTab->setAutoFillBackground(true);

    tab = new QWidget();
    formLayout = new QFormLayout(tab);
    formLayout->setObjectName(QStringLiteral("formLayout"));
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    cameraSizeBox = new QGroupBox(tab);
    formLayout_4 = new QFormLayout(cameraSizeBox);
    formLayout_3 = new QFormLayout();
    label_2 = new QLabel(cameraSizeBox);
    label_2->setText(tr("Width"));
    formLayout_3->setWidget(1, QFormLayout::LabelRole, label_2);
    label_3 = new QLabel(cameraSizeBox);
    label_3->setText(tr("Height"));
    formLayout_3->setWidget(2, QFormLayout::LabelRole, label_3);
    cameraWidth = new QSpinBox(cameraSizeBox);
    cameraWidth->setObjectName(QStringLiteral("cameraWidth"));
    cameraWidth->setMaximum(3000);
    cameraWidth->setValue(960);
    formLayout_3->setWidget(1, QFormLayout::FieldRole, cameraWidth);
    cameraHeight = new QSpinBox(cameraSizeBox);
    cameraHeight->setObjectName(QStringLiteral("cameraHeight"));
    cameraHeight->setMaximum(3000);
    cameraHeight->setValue(768);
    formLayout_3->setWidget(2, QFormLayout::FieldRole, cameraHeight);
    formLayout_4->setLayout(0, QFormLayout::LabelRole, formLayout_3);
    formLayout->setWidget(0, QFormLayout::SpanningRole, cameraSizeBox);
    settingTab->addTab(tab, QString(tr("Camera")));

    tab_2 = new QWidget();
    tab_2->setObjectName(QStringLiteral("tab_2"));
    formLayout_8 = new QFormLayout(tab_2);
    projSizeBox = new QGroupBox(tab_2);
    formLayout_6 = new QFormLayout(projSizeBox);
    gridLayout_3 = new QGridLayout();
    projWidth = new QSpinBox(projSizeBox);
    projWidth->setObjectName(QStringLiteral("projWidth"));
    projWidth->setMaximum(3000);
    projWidth->setValue(1024);
    gridLayout_3->addWidget(projWidth, 1, 1, 1, 1);
    label_4 = new QLabel(projSizeBox);
    label_4->setText(tr("Width"));
    gridLayout_3->addWidget(label_4, 1, 0, 1, 1);
    label_5 = new QLabel(projSizeBox);
    label_5->setText(tr("Height"));
    gridLayout_3->addWidget(label_5, 2, 0, 1, 1);
    projHeight = new QSpinBox(projSizeBox);
    projHeight->setObjectName(QStringLiteral("projHeight"));
    projHeight->setMaximum(3000);
    projHeight->setValue(768);
    gridLayout_3->addWidget(projHeight, 2, 1, 1, 1);
    formLayout_6->setLayout(0, QFormLayout::LabelRole, gridLayout_3);
    horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    formLayout_6->setItem(0, QFormLayout::FieldRole, horizontalSpacer_2);
    formLayout_8->setWidget(0, QFormLayout::SpanningRole, projSizeBox);
    projPositionBox = new QGroupBox(tab_2);
    formLayout_2 = new QFormLayout(projPositionBox);
    gridLayout_2 = new QGridLayout();
    label_6 = new QLabel(projPositionBox);
    label_6->setText("X");
    gridLayout_2->addWidget(label_6, 0, 0, 1, 1);
    projXPosition = new QSpinBox(projPositionBox);
    projXPosition->setObjectName(QStringLiteral("projXPosition"));
    projXPosition->setMinimum(-3000);
    projXPosition->setMaximum(3000);
    projXPosition->setValue(1024);
    gridLayout_2->addWidget(projXPosition, 0, 1, 1, 1);
    label_7 = new QLabel(projPositionBox);
    label_7->setText("Y");
    gridLayout_2->addWidget(label_7, 1, 0, 1, 1);
    projYPosition = new QSpinBox(projPositionBox);
    projYPosition->setObjectName(QStringLiteral("projYPosition"));
    projYPosition->setMinimum(-3000);
    projYPosition->setMaximum(3000);
    gridLayout_2->addWidget(projYPosition, 1, 1, 1, 1);
    formLayout_2->setLayout(0, QFormLayout::LabelRole, gridLayout_2);
    horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    formLayout_2->setItem(0, QFormLayout::FieldRole, horizontalSpacer_3);
    formLayout_8->setWidget(1, QFormLayout::SpanningRole, projPositionBox);
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

    connect(okButton, SIGNAL(clicked()), this, SLOT(createConfigurationFile()));
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
    cam_w = cameraWidth->value();
    cam_h = cameraHeight->value();
    proj_w = projWidth->value();
    proj_h = projHeight->value();
    projectorWinPos_x = projXPosition->value();
    projectorWinPos_y = projYPosition->value();
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
    xmlWriter.writeStartElement("Projector");
    xmlWriter.writeTextElement("Width",QString::number(proj_w, 10));
    xmlWriter.writeTextElement("Height",QString::number(proj_h, 10));
    xmlWriter.writeEndElement();
    xmlWriter.writeStartElement("Camera");
    xmlWriter.writeTextElement("Width",QString::number(cam_w, 10));
    xmlWriter.writeTextElement("Height",QString::number(cam_h, 10));
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

