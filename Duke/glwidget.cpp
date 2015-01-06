#include "glwidget.h"

#include <QMouseEvent>
#include <GLU.h>
#include <QColorDialog>
#include <qmath.h>

GLfloat scale = 1;
GLfloat percent = 0.1;
GLfloat speed = 20;
bool hasModel = false;
glm::mat4 transform_camera(1.0f); // 摄像机的位置和定向，即摄像机在世界坐标系中位置
glm::mat4 transform_model(1.0f);  // 模型变换矩阵，即物体坐标到世界坐标
glm::mat4 model_view_matrix;

bool keyPressed[2] = {false, false};

GLWidget::GLWidget(QWidget *parent) :
    QGLWidget(QGLFormat(QGL::SampleBuffers), parent, 0, Qt::FramelessWindowHint)
{
    setFormat(QGLFormat(QGL::DoubleBuffer | QGL::DepthBuffer));
    rotationX = 0.0;
    rotationY = 0.0;
    rotationZ = 0.0;
    offsetX = 0.0;
    offsetY = 0.0;
    pointSize = 1;
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
    hasModel = plyloader->LoadModel(loadpath);
    if(hasModel)
        updateGL();
}

void GLWidget::initializeGL()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(30, float(this->width())/this->height(), 1.0, 1.0e10);

    qglClearColor(backColor);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_CULL_FACE);
    SetupLights();
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_NORMALIZE);

    transform_camera = glm::affineInverse(glm::lookAt(glm::vec3(0,0,40), glm::vec3(0,0,0), glm::vec3(0,-1,0)));
    transform_model = glm::translate(glm::vec3(-10,-10,0));
}

void GLWidget::resizeGL(int width, int height)
{
    glViewport(0, 0, this->width(), this->height());
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(28.0,float(this->width())/this->height(), 1.0, 1.0e10);
}

void GLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    model_view_matrix = glm::affineInverse(transform_camera);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(&model_view_matrix[0][0]);
    model_view_matrix *= transform_model;
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(&model_view_matrix[0][0]);
    draw();
}

void GLWidget::draw()
{
    GLfloat sizes[]=
    {
        scale, scale, scale
    };
    glRotatef(rotationX, 1.0, 0.0, 0.0);
    glRotatef(rotationY, 0.0, 1.0, 0.0);
    glRotatef(rotationZ, 0.0, 0.0, 1.0);
    glScalef(sizes[0], sizes[1], sizes[2]);

    long int total = plyloader->m_totalConnectedPoints;
    for(int p = 0; p < total; ++p)
    {
        glPointSize(pointSize);
        glBegin(GL_POINTS);
        qglColor(QColor::fromCmyk(255,0,255,0));
        glVertex3f((plyloader->mp_vertexXYZ[p*3]) * percent,
                (plyloader->mp_vertexXYZ[p*3 + 1]) * percent,
                 (plyloader->mp_vertexXYZ[p*3 + 2]) * percent);
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
        rotationX += 180 * dy;
        rotationY += 180 * dx;
        updateGL();
    }
    else if(event->buttons() == Qt::RightButton){
        rotationX += 180 * dx;
        rotationZ += 180 * dy;
        updateGL();
    }
    else if(event->buttons()==Qt::MiddleButton) {
         transform_camera *= glm::translate(glm::vec3(-speed*dx,speed*dy,0));
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
    if(scale < 10 && scale > 0)
    {
        updateGL();
    }
    else
    {
        scale = 1;
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


void GLWidget::drag_ball(int x1, int y1, int x2, int y2, glm::mat4& Tmodel, glm::mat4& Tcamera)
{
    float r = (float)std::min(this->height(), this->width())/3;
    float r2 = r*0.9f;
    float ax = x1 - (float)this->width()/2;
    float ay = y1 - (float)this->height()/2;
    float bx = x2 - (float)this->width()/2;
    float by = y2 - (float)this->height()/2;
    float da = std::sqrt(ax*ax+ay*ay);
    float db = std::sqrt(bx*bx+by*by);
    if(std::max(da,db)>r2){
        float dx, dy;
        if(da>db){
            dx = (r2/da-1)*ax;
            dy = (r2/da-1)*ay;
        }else{
            dx = (r2/db-1)*bx;
            dy = (r2/db-1)*by;
        }
        ax += dx; ay +=dy; bx += dx; by += dy;
    }
    float az = std::sqrt( r*r-(ax*ax+ay*ay) );
    float bz = std::sqrt( r*r-(bx*bx+by*by) );
    glm::vec3 a = glm::vec3(ax,ay,az);
    glm::vec3 b = glm::vec3(bx,by,bz);
    float theta = std::acos(glm::dot(a,b)/(r*r));
    glm::vec3 v2 = glm::cross(a,b);
    // v2是视觉坐标系的向量，v是v2在物体坐标系中的坐标
    glm::vec3 v = glm::vec3(
        glm::affineInverse(Tmodel) * Tcamera
        * glm::vec4(v2[0],v2[1],v2[2],0) );
    Tmodel *= glm::rotate( theta*180/3.14f, v );
}
