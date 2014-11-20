#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QGLWidget>
#include "plyloader.h"

class GLWidget : public QGLWidget
{
    Q_OBJECT
public:
    explicit GLWidget(QWidget *parent = 0);
    ~GLWidget();

    int pointSize;
    void setPoint(int psize);

protected:
    void initializeGL();
    void resizeGL(int width = 300, int height =300);
    void paintGL();

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

private:
    void draw();
    void SetupLights();
    void createGradient();
    void drawBackground(QPainter *painter);
    GLfloat rotationX;
    GLfloat rotationY;
    GLfloat rotationZ;
    QPoint lastPos;
    PlyLoader *plyloader;

    QRadialGradient gradient;
    QColor backColor;


};

#endif // GLWIDGET_H
