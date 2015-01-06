#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QGLWidget>
#include "plyloader.h"
#include <glm.hpp>
#include <GL/glut.h>
#include <ext.hpp>

class GLWidget : public QGLWidget
{
    Q_OBJECT
public:
    explicit GLWidget(QWidget *parent = 0);
    ~GLWidget();

    int pointSize;
    void LoadModel(QString loadpath);
    void setPoint(int psize);

protected:
    void initializeGL();
    void resizeGL(int width = 300, int height =300);
    void paintGL();

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);

private:
    void draw();
    void SetupLights();
    void createGradient();
    void drawBackground(QPainter *painter);
    GLfloat rotationX;
    GLfloat rotationY;
    GLfloat rotationZ;
    GLfloat offsetX;
    GLfloat offsetY;
    QPoint lastPos;
    PlyLoader *plyloader;

    QRadialGradient gradient;
    QColor backColor;

    void drag_ball(int x1, int y1, int x2, int y2, glm::mat4& Tmodel, glm::mat4& Tcamera);
};

#endif // GLWIDGET_H
