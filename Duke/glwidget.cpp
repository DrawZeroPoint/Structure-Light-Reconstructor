#include "glwidget.h"

#include <QMouseEvent>
#include <GLU.h>
#include <QColorDialog>
#include <qmath.h>

GLfloat scale = 1.0;
GLfloat percent = 0.1;
GLfloat offsetX = 0.0;
GLfloat offsetY = 0.0;

bool keyPressed[2] = {false, false};

GLWidget::GLWidget(QWidget *parent) :
    QGLWidget(QGLFormat(QGL::SampleBuffers), parent, 0, Qt::FramelessWindowHint)
{
    setFormat(QGLFormat(QGL::DoubleBuffer | QGL::DepthBuffer));
    rotationX = 0.0;
    rotationY = 0.0;
    rotationZ = 0.0;
    pointSize = 5;
    backColor = QColor::fromCmykF(0.5, 0.4, 0.4, 0.2);
    createGradient();
    plyloader = new PlyLoader(this);
}

GLWidget::~GLWidget()
{
    delete plyloader;
}

void GLWidget::LoadModel(QString loadpath)
{
    plyloader->LoadModel(loadpath);
    draw();
}

void GLWidget::initializeGL()
{
    qglClearColor(backColor);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_CULL_FACE);
    SetupLights();
    glEnable(GL_DEPTH_TEST);
}

void GLWidget::resizeGL(int width, int height)
{
    glViewport(0, 0, this->width(), this->height());
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    GLfloat x = GLfloat(this->width()) / this->height();
    glFrustum(-x,+x,-1.0,+1.0,4.0,15.0);
    glMatrixMode(GL_MODELVIEW);
}

void GLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    draw();
    glLoadIdentity();
}

void GLWidget::draw()
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    GLfloat sizes[]=
    {
        scale, scale, scale
    };
    glTranslatef(0, 0, -10.0);
    glRotatef(rotationX, 1.0, 0.0, 0.0);
    glRotatef(rotationY, 0.0, 1.0, 0.0);
    glRotatef(rotationZ, 0.0, 0.0, 1.0);
    glScalef(sizes[0], sizes[1], sizes[2]);
    /*
    glBegin(GL_TRIANGLES);
    qglColor(QColor::fromCmyk(255,0,0,0));
    for(int i = 3; i < 6; i++)
    {
        glVertex3f(plyloader->mp_vertexXYZ[0 + 3*i], plyloader->mp_vertexXYZ[1 + 3*i], plyloader->mp_vertexXYZ[2 + 3*i]);
    }
    glEnd();
    for(int m = 0; m < 6; ++m)
    {
        glBegin(GL_LINES);
        qglColor(QColor::fromCmyk(0,255,0,0));
        for(int k = 0; k <2; ++k)
        {
            glVertex3f(edges[m][k][0], edges[m][k][1],edges[m][k][2]);
        }
        glEnd();
    }
    */
    long int total = plyloader->m_totalConnectedPoints;
    for(int p = 0; p < total; ++p)
    {
        glPointSize(pointSize);
        glBegin(GL_POINTS);
        qglColor(QColor::fromCmyk(255,0,255,0));
        glVertex3f((plyloader->mp_vertexXYZ[p*3] - offsetX) * percent,
                (plyloader->mp_vertexXYZ[p*3 + 1] - offsetY) * percent,
                 plyloader->mp_vertexXYZ[p*3 + 2] * percent);
        glEnd();
    }
    glFlush();
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
    lastPos = event->pos();
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
    GLfloat dx = GLfloat(event->x() - lastPos.x()) / width();
    GLfloat dy = GLfloat(event->y() - lastPos.y()) / height();
    if(event->buttons() == Qt::LeftButton){
        if(keyPressed[0] == true)
        {
            if (abs(dx) > abs(dy) && dx > 0) offsetX += 0.5;
            else if (abs(dy) > abs(dx) && dy >0) offsetY += 0.5;
            else if (abs(dy) > abs(dx) && dy < 0) offsetY -= 0.5;
            else if (abs(dx) > abs(dy) && dx < 0) offsetX -= 0.5;
        }
        else
        {
            rotationX += 180 * dy;
            rotationY += 180 * dx;
        }
        updateGL();
    }
    else if(event->buttons() == Qt::RightButton){
        rotationX += 180 * dy;
        rotationZ += 180 * dx;
        updateGL();
    }
    lastPos = event->pos();
}

void GLWidget::mouseDoubleClickEvent(QMouseEvent * /*event*/)
{
    QColor color = QColorDialog::getColor(backColor, this);
    if(color.isValid())
    {
        backColor = color;
        qglClearColor(backColor);
    }
}

void GLWidget::wheelEvent(QWheelEvent *event)
{
    double numDegrees = - event->delta() / 3600.0;
    scale += numDegrees;
    if(scale < 3 && scale > 0)
    {
        updateGL();
    }
    else
    {
        scale = 1;
    }
}

void GLWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Control)
    {
        keyPressed[0] = true;
    }
    else if (event->key() == Qt::Key_Alt)
    {
        keyPressed[1] = true;
    }
}

void GLWidget::keyReleaseEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Control)
    {
        keyPressed[0] = false;
    }
    else if (event->key() == Qt::Key_Alt)
    {
        keyPressed[1] = false;
    }
}


void GLWidget::SetupLights()
{
    GLfloat ambientLight[]  = {0.6f,  0.6f,  0.6f,  1.0f};//环境光
    GLfloat diffuseLight[]  = {0.7f,  0.7f,  0.7f,  1.0f};//漫反射
    GLfloat specularLight[] = {0.9f,  0.9f,  0.9f,  1.0f};//镜面光
    GLfloat lightPos[]      = {50.0f, 80.0f, 60.0f, 1.0f};//光源位置

    glEnable(GL_LIGHTING);                              //启用光照
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);     //设置环境光源
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);     //设置漫反射光源
    glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);   //设置镜面光源
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);        //设置灯光位置
    glEnable(GL_LIGHT0);                                //打开第一个灯光

    glEnable(GL_COLOR_MATERIAL);                        //启用材质的颜色跟踪
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);  //指定材料着色的面
    glMaterialfv(GL_FRONT, GL_SPECULAR, specularLight); //指定材料对镜面光的反应
    glMateriali(GL_FRONT, GL_SHININESS, 100);           //指定反射系数
}


void GLWidget::createGradient()
{
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    gradient.setCenter(0.45, 0.50);
    gradient.setFocalPoint(0.40, 0.45);
    gradient.setColorAt(0.0, QColor(105, 146, 182));
    gradient.setColorAt(0.4, QColor(81, 113, 150));
    gradient.setColorAt(0.8, QColor(16, 56, 121));
}


void GLWidget::drawBackground(QPainter *painter)
{
    painter->setPen(Qt::NoPen);
    painter->setBrush(gradient);
    painter->drawRect(rect());
}

void GLWidget::setPoint(int psize)
{
    pointSize = psize;
    updateGL();
}
