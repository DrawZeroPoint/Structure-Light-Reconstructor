#ifndef SET_H
#define SET_H
#include <QtGui>
#include <QMainWindow>
#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QWidget>
#include <opencv/cv.h>

#include <direct.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

class Set : public QDialog
{
    Q_OBJECT
public:
    Set(QMainWindow *parent = 0);
    QString saveSetPath;
    int proj_h;
    int proj_w;
    int projGrid_w;
    int projGrid_h;
    int black_threshold;
    int white_threshold;
    int board_w;
    int board_h;
    int projectorWinPos_x;
    int projectorWinPos_y;
    bool autoContrast;
    bool saveAutoContrast;
    bool raySampling;
    int exportObj;
    int exportPly;

    QPushButton *okButton;
    QPushButton *cancelButton;

private:

    QFormLayout *formLayout_7;
    QTabWidget *settingTab;
    QWidget *tab;
    QFormLayout *formLayout;
    QGroupBox *boardSizeBox;
    QFormLayout *formLayout_4;
    QFormLayout *formLayout_3;
    QLabel *label_2;
    QLabel *label_3;
    QSpinBox *boardWidth;
    QSpinBox *boardHeight;
    QWidget *tab_2;
    QFormLayout *formLayout_8;

    QGroupBox *gridSizeBox;
    QFormLayout *formLayout_6;
    QGridLayout *gridLayout_3;
    QSpinBox *projGridWidth;
    QLabel *label_4;
    QLabel *label_5;
    QSpinBox *projGridHeight;
    QSpacerItem *horizontalSpacer_2;

    QGroupBox *pattenNumBox;
    QFormLayout *formLayout_2;
    QGridLayout *gridLayout_2;
    QLabel *label_6;
    QSpinBox *xGridNum;
    QLabel *label_7;
    QSpinBox *yGridNum;
    QSpacerItem *horizontalSpacer_3;

    QGroupBox *projGeomBox;
    QFormLayout *geomFormLayout;
    QGridLayout *geomGridLayout;
    QLabel *widthLabel;
    QSpinBox *widthSpinBox;
    QLabel *heightLabel;
    QSpinBox *heightSpinBox;
    QSpacerItem *geomSpacer;

    QWidget *tab_3;
    QGridLayout *gridLayout_5;
    QGroupBox *groupBox_2;
    QGridLayout *gridLayout_6;
    QFormLayout *formLayout_5;
    QGridLayout *gridLayout_4;
    QLabel *label_8;
    QSpinBox *blackThresholdEdit;
    QLabel *label_9;
    QSpinBox *whiteThresholdEdit;
    QSpacerItem *horizontalSpacer_4;
    QGridLayout *gridLayout;
    QCheckBox *autoContrastCheck;
    QCheckBox *saveAutoContrastImagesCheck;
    QCheckBox *raySamplingCheck;
    QWidget *tab_4;
    QFormLayout *formLayout_12;
    QGroupBox *groupBox;
    QFormLayout *formLayout_11;
    QCheckBox *exportObjCheck;
    QSpacerItem *horizontalSpacer;
    QCheckBox *exportPlyCheck;
    QSpacerItem *horizontalSpacer_6;

    int boolToInt(bool input);

private slots:
    void test(bool flag);
    void createConfigurationFile();
    void createSetFile();
};

#endif // SET_H
