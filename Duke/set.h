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
    int black_threshold;
    int white_threshold;
    int cam_w;
    int cam_h;
    int projectorWinPos_x;
    int projectorWinPos_y;
    bool autoContrast;
    bool saveAutoContrast;
    bool raySampling;
    int exportObj;
    int exportPly;

private:

    QFormLayout *formLayout_7;
    QTabWidget *settingTab;
    QWidget *tab;
    QFormLayout *formLayout;
    QGroupBox *cameraSizeBox;
    QFormLayout *formLayout_4;
    QFormLayout *formLayout_3;
    QLabel *label_2;
    QLabel *label_3;
    QSpinBox *cameraWidth;
    QSpinBox *cameraHeight;
    QWidget *tab_2;
    QFormLayout *formLayout_8;
    QGroupBox *projSizeBox;
    QFormLayout *formLayout_6;
    QGridLayout *gridLayout_3;
    QSpinBox *projWidth;
    QLabel *label_4;
    QLabel *label_5;
    QSpinBox *projHeight;
    QSpacerItem *horizontalSpacer_2;
    QGroupBox *projPositionBox;
    QFormLayout *formLayout_2;
    QGridLayout *gridLayout_2;
    QLabel *label_6;
    QSpinBox *projXPosition;
    QLabel *label_7;
    QSpinBox *projYPosition;
    QSpacerItem *horizontalSpacer_3;
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
    QPushButton *okButton;
    QPushButton *cancelButton;

    int boolToInt(bool input);

private slots:
    void test(bool flag);
    void createConfigurationFile();
    void createSetFile();
};

#endif // SET_H
