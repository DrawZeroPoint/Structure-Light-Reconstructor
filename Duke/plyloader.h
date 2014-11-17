#ifndef PLYLOADER_H
#define PLYLOADER_H

#include <QObject>

#include <iostream>
#include <vector>

using namespace std;

struct SModelData
{
  vector <float> vecFaceTriangles; // = face * 9
  vector <float> vecFaceTriangleColors; // = face * 9
  vector <float> vecNormals; // = face * 9
  int iTotalConnectedTriangles;
};

class PlyLoader : public QObject
{
    Q_OBJECT
public:
    explicit PlyLoader(QObject *parent = 0);
    int LoadModel(char *filename);
    float* mp_vertexXYZ;
    int m_totalConnectedPoints;

private:
  SModelData m_ModelData;
};

#endif // PLYLOADER_H
