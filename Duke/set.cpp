#include "set.h"
#include <QMessageBox>
#include <QApplication>
#include <QPushButton>//对于connect函数是必要的，否则出现C2664类型转换错误
#include <QChar>

#include "ui_Set.h"

int projGrid_w;
int projGrid_h;
int boardCellNum_h;
int boardCellNum_v;

Set::Set(QMainWindow *parent) : QDialog(parent),
    set(new Ui::SetDialog)
{
    set->setupUi(this);
    createConfigurationFile();
    connect(set->buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(createConfigurationFile()));
    connect(set->buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), SIGNAL(outputSet()));
    connect(set->buttonBox->button(QDialogButtonBox::Ok),SIGNAL(clicked()), this, SLOT(hide()));
    connect(set->buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), this, SLOT(createConfigurationFile()));
    connect(set->buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), SIGNAL(outputSet()));
    connect(set->buttonBox->button(QDialogButtonBox::Cancel),SIGNAL(clicked()),this,SLOT(hide()));
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
    board_w = set->boardWidth->value();
    board_h = set->boardHeight->value();
    proj_w = set->projResH->value();
    proj_h = set->projResV->value();
    scan_w = set->scanResH->value();
    scan_h = set->scanResV->value();
    cam_w = set->camResH->value();
    cam_h = set->camResV->value();
    black_threshold = set->blackThresholdEdit->value();
    white_threshold = set->whiteThresholdEdit->value();
    if(set->autoContrastCheck->isChecked() == true)
        autoContrast = true;
    else
        autoContrast = false;
    if(set->saveAutoContrastImagesCheck->isChecked() == true)
        saveAutoContrast = true;
    else
        saveAutoContrast = false;
    if(set->raySamplingCheck->isChecked() == true)
        raySampling = true;
    else
        raySampling = false;
    if(set->exportObjCheck->isChecked() == true)
        exportObj = 1;
    else
        exportObj = 0;
    if(set->exportPlyCheck->isChecked() == true)
        exportPly = 1;
    else
        exportPly = 0;
    //createSetFile();
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
    xmlWriter.writeStartElement("ProjectorResolution");
    xmlWriter.writeTextElement("Width",QString::number(proj_w, 10));
    xmlWriter.writeTextElement("Height",QString::number(proj_h, 10));
    xmlWriter.writeEndElement();
    xmlWriter.writeStartElement("CalibrationBoard");
    xmlWriter.writeTextElement("BoardWidth",QString::number(board_w, 10));
    xmlWriter.writeTextElement("BoardHeight",QString::number(board_h, 10));
    xmlWriter.writeEndElement();
    xmlWriter.writeStartElement("ProjectorWindow");
    xmlWriter.writeStartElement("ScanResolution");
    xmlWriter.writeTextElement("Width",QString::number(scan_w, 10));
    xmlWriter.writeTextElement("Height",QString::number(scan_h, 10));
    xmlWriter.writeEndElement();//由于start两次所以end两次
    xmlWriter.writeEndElement();
    xmlWriter.writeStartElement("Reconstruction");
    xmlWriter.writeTextElement("AutoContrast",0);
    xmlWriter.writeTextElement("SaveAutoContrastImages",0);
    xmlWriter.writeTextElement("RaySampling",0);
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

void Set::switchLang()
{
    set->retranslateUi(this);
}
