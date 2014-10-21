#include "virtualcamera.h"
#include "utilities.h"
#include <QFile>
#include <QMessageBox>

VirtualCamera::VirtualCamera()
{
    distortion = NULL;
    rotationMatrix = NULL;
    translationVector = NULL;
    fc.x=0;
    fc.y=0;
    cc.x=0;
    cc.y=0;
}

VirtualCamera::~VirtualCamera()
{
}

void VirtualCamera::loadDistortion(QString path)
{
    std::string str = qstringToString(path);
    loadMatrix(distortion,5,1,str);
}

bool VirtualCamera::loadCameraMatrix(QString path)
{
    bool existPath = QFile::exists(path);
    if(!existPath){
        QMessageBox::warning(NULL,"Matrix not found","File: '"+path+"' need to be added.");
        return false;//Imply that the matrix didn't loaded successfully.
    }
    cv::Mat camMatrix;
    std::string str = qstringToString(path);
    loadMatrix(camMatrix,3,3,str);
    cc.x = Utilities::matGet2D(camMatrix,2,0);//matget2d中的2,0表示取矩阵第2列，第0行
    cc.y = Utilities::matGet2D(camMatrix,2,1);
    fc.x = Utilities::matGet2D(camMatrix,0,0);
    fc.y = Utilities::matGet2D(camMatrix,1,1);
    return true;
}

void VirtualCamera::loadRotationMatrix(QString path)
{
    std::string str = qstringToString(path);
    loadMatrix(rotationMatrix,3,3,str);
}

void VirtualCamera::loadTranslationVector(QString path)
{
    std::string str = qstringToString(path);
    loadMatrix(translationVector,3,1,str);
}

int VirtualCamera::loadMatrix(cv::Mat &matrix,int rows,int cols ,std::string file)
{
    std:: ifstream in1;
    in1.open(file);
    if(!in1)
    {
        return -1;
    }
    if(!matrix.empty())
        matrix.release();
    matrix=cv::Mat(rows, cols, CV_32F);//cv_32f表示数据精度32-bit ﬂoating-point numbers ( -FLT_MAX..FLT_MAX, INF, NAN )
    for(int i=0; i<rows; i++)
    {
        for(int j=0; j<cols; j++)
        {
            float val;
            in1>>val;
            Utilities::matSet2D(matrix, j, i, val);
        }
    }
    return 1;
}

std::string VirtualCamera::qstringToString(QString qstring)
{
    std::string cstr;
    cstr = std::string((const char *)qstring.toLocal8Bit());
    return cstr;
}
