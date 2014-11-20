#ifndef MESHCREATOR_H
#define MESHCREATOR_H

#include "pointcloudimage.h"
#include <QString>

class MeshCreator
{
public:
    MeshCreator(PointCloudImage *in);
    ~MeshCreator(void);
    void exportObjMesh(QString path);
    void exportPlyMesh(QString path);
private:
    int *pixelNum;
    PointCloudImage *cloud;
    int MeshCreator::access(int i,int j);
    int MeshCreator::access(int i,int j, int z);

    int w;
    int h;
};

#endif // MESHCREATOR_H
