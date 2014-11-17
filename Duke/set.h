#ifndef SET_H
#define SET_H
#include <QtGui>
#include <QMainWindow>
#include <QDialog>
#include <opencv/cv.h>

#include <direct.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

namespace Ui {
class SetDialog;
}

class Set : public QDialog
{
    Q_OBJECT
public:
    Set(QMainWindow *parent = 0);

    QString saveSetPath;
    int proj_h;
    int proj_w;
    int scan_w;
    int scan_h;
    int cam_w;
    int cam_h;

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

    void switchLang();

private:
    Ui::SetDialog *set;
    int boolToInt(bool input);

signals:
    void outputSet();

private slots:
    void test(bool flag);
    void createConfigurationFile();
    void createSetFile();
};

#endif // SET_H
