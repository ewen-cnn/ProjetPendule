#include "objectgl.h"
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QPainter>
//=============================================================================================
objectgl::objectgl(QWidget *parent) :
        QOpenGLWidget(parent)



{
    // Initialize each color
    BackGround_Color    =QColor::fromRgb(0 ,0 ,0);
    Axis_X_Color        =QColor::fromRgb(255,64  ,64  ,128);   // Color of the X axis : red
    Axis_Y_Color        =QColor::fromRgb(64  ,255,64  ,128);   // Color of the Y axis : green
    Axis_Z_Color        =QColor::fromRgb(64  , 64 ,255,128);  // Color of the Z axis : blue

    // Set initial value of angles
    angle_x=angle_y=angle_z=0;

    // Start display in the isometric view
    IsometricView();
    //TopView();
}
//=============================================================================================
objectgl::~objectgl()
{
    makeCurrent();
}
//=============================================================================================
void objectgl::initializeGL()
{
    initializeOpenGLFunctions(); // Nécessaire pour activer les fonctions OpenGL
    glClearColor(
        BackGround_Color.redF(),
        BackGround_Color.greenF(),
        BackGround_Color.blueF(),
        1.0f
        );
    glShadeModel(GL_FLAT);
    glEnable(GL_DEPTH_TEST);                           // Depth buffer enabled (Hide invisible items)
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_BLEND);
    glEnable(GL_COLOR_MATERIAL);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // For transparency
    glColorMaterial(GL_FRONT,GL_DIFFUSE);
    glEnable(GL_NORMALIZE);
}
//=============================================================================================
// DRAW THE SCENE
//=============================================================================================
void objectgl::paintGL(  )
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Set the lights
    GLfloat LightAmbient[]={0.4f,0.4f,0.4f,1.0f};
    GLfloat LightDiffuse[]={0.8f,0.8f,0.8f,1.0f};
    glLightfv(GL_LIGHT0,GL_AMBIENT,LightAmbient);
    glLightfv(GL_LIGHT0,GL_DIFFUSE,LightDiffuse);
    int LightPos[4]={0,0,10,1};
    glLightiv(GL_LIGHT0,GL_POSITION,LightPos);

    // Move the display according to the current orientation
    glRotated(xRot / 16.0, 1.0, 0.0, 0.0);
    glRotated(yRot / 16.0, 0.0, 1.0, 0.0);
    glRotated(zRot / 16.0, 0.0, 0.0, 1.0);

    glDisable(GL_LIGHTING);

    // Invert the Y-axis for an orthonormal frame
    glScalef(1,-1,1);

    // Draw Frame
    Draw_Frame();

    // Start display of the items
    glPushMatrix();                // The following properties are only for the object

    // Zoom according to the view's parameters
    glScalef(Zoom,Zoom,Zoom);      // ReZoom the object

    // Light independant (color is constant)
    //glLineWidth(5.0);

    Draw_Box();
    glEnable(GL_LIGHTING);   // Re enable the light
    // End of the object
    glPopMatrix();

    // Update the view
    glViewport(0, 0,WindowSize.width(), WindowSize.height());
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    GLfloat Ratio=(GLfloat)WindowSize.width()/(GLfloat)WindowSize.height();
    glOrtho((-0.5+dx)*Ratio,
            ( 0.5+dx)*Ratio ,
            +0.5+dy,
            -0.5+dy,
            -1500.0, 1500.0);
    glMatrixMode(GL_MODELVIEW);
}

//=============================================================================================
// Draw Object
//=============================================================================================
#define L1  0.9
#define L2  0.4
#define L3  0.2

void objectgl::Draw_Box()
{
    glPushMatrix();

    glRotated(angle_z , 0.0, 0.0, 1.0);
    glRotated(angle_y, 0.0, 1.0, 0.0);
    glRotated(-angle_x, 1.0, 0.0, 0.0);

    glPushMatrix();
    glScalef(4.,4.,4.);
    glPopMatrix();

    // Rear View
    glBegin(GL_POLYGON);
    QColor color = QColor::fromRgb(0, 0, 255, 128);
    glColor4f(color.redF(), color.greenF(), color.blueF(), color.alphaF());
    glVertex3d(-L1 ,-L2   ,-L3);
    glVertex3d(-L1 ,L2    ,-L3);
    glVertex3d(L1  ,L2    ,0.0);
    glVertex3d(L1  ,-L2   ,0.0);
    glEnd();

    // Front View
    glBegin(GL_POLYGON);
    glColor4f(51 / 255.0f, 255 / 255.0f, 0 / 255.0f, 128 / 255.0f); //Green
    glVertex3d(-L1 ,-L2   ,L3);
    glVertex3d(L1  ,-L2   ,0.0);
    glVertex3d(L1  ,L2    ,0.0);
    glVertex3d(-L1 ,L2    ,L3);
    glEnd();

    // Left View
    glBegin(GL_POLYGON);
    glColor4f(255 / 255.0f, 255 / 255.0f, 0 / 255.0f, 128 / 255.0f); // yellow
    glVertex3d(-L1  ,-L2    ,L3);
    glVertex3d(-L1  ,L2     ,L3);
    glVertex3d(-L1  ,L2     ,-L3);
    glVertex3d(-L1  ,-L2    ,-L3);
    glEnd();

    // Top View
    glBegin(GL_TRIANGLES);
    glColor4f(255 / 255.0f, 102 / 255.0f, 0 / 255.0f, 128 / 255.0f); // orange
    glVertex3d(-L1  ,L2    ,L3);
    glVertex3d(L1   ,L2     ,0.0);
    glVertex3d(-L1  ,L2    ,-L3);
    glEnd();


    // Bottom View
    glBegin(GL_TRIANGLES);
    glColor4f(0 / 255.0f, 255 / 255.0f, 255 / 255.0f, 128 / 255.0f); // light blue
    glVertex3d(-L1  ,-L2    ,L3);
    glVertex3d(-L1  ,-L2    ,-L3);
    glVertex3d(L1   ,-L2     ,0.0);
    glEnd();

    glPopMatrix();
}
//=============================================================================================
// Draw the frame (X,Y and Z axis)
//=============================================================================================
void objectgl::Draw_Frame()
{
    glLineWidth(5);
    // X-axis
    glBegin(GL_LINES);
    {
        QColor c = Axis_X_Color;
        glColor4f(c.redF(), c.greenF(), c.blueF(), c.alphaF());
    }
    glVertex3d(0,0,0);
    glVertex3d(0.5, 0, 0);
    glEnd();
    // Y-axis
    glBegin(GL_LINES);
    {
        QColor c = Axis_Y_Color;
        glColor4f(c.redF(), c.greenF(), c.blueF(), c.alphaF());
    }
    glVertex3d(0,0,0);
    glVertex3d(0, 0.5, 0);
    glEnd();
    // Z-axis
    glBegin(GL_LINES);
    {
        QColor c = Axis_Z_Color;
        glColor4f(c.redF(), c.greenF(), c.blueF(), c.alphaF());
    }
    glVertex3d(0,0,0);
    glVertex3d(0, 0, 0.5);
    glEnd();
}
//=============================================================================================
// Standard views
//=============================================================================================
void objectgl::FrontView()
{
    SetXRotation(0);
    SetYRotation(0);
    SetZRotation(0);
    Zoom=1;
    dx=dy=0;
}
//=============================================================================================
void objectgl::RearView()
{
    SetXRotation(0);
    SetYRotation(180*16);
    SetZRotation(0);
    Zoom=1;
    dx=dy=0;
}
//=============================================================================================
void objectgl::LeftView()
{
    SetXRotation(0);
    SetYRotation(90*16);
    SetZRotation(0);
    Zoom=1;
    dx=dy=0;
}
//=============================================================================================
void objectgl::RightView()
{
    SetXRotation(0);
    SetYRotation(-90*16);
    SetZRotation(0);
    Zoom=1;
    dx=dy=0;
}
//=============================================================================================
void objectgl::TopView()
{
    SetXRotation(-90*16);
    SetYRotation(0);
    SetZRotation(0);
    Zoom=0.5;
    dx=dy=0;
}
//=============================================================================================
void objectgl::BottomView()
{
    SetXRotation(90*16);
    SetYRotation(0);
    SetZRotation(0);
    Zoom=0.5;
    dx=dy=0;
}
//=============================================================================================
void objectgl::IsometricView()
{
    SetXRotation(62*16);
    SetYRotation(0);
    SetZRotation(45*16);
    Zoom=0.5;
    dx=dy=0;
}
//=============================================================================================
// Events (Resize, mouse ...)
//=============================================================================================
// Resize the openGL window according to width and height
void objectgl::resizeGL(int width, int height)
{
    WindowSize=QSize(width,height);
}
//=============================================================================================
// OpenGL angles are contained in the interval [0 : 360*16]
// Normalize the angle in this interval
void objectgl::NormalizeAngle(int *angle)
{
    while (*angle < 0)
        *angle += 360 * 16;
    while (*angle >= 360 * 16)
        *angle -= 360 * 16;
}
//=============================================================================================
// Update the rotation around X if necessary
void objectgl::SetXRotation(int angle)
{
    NormalizeAngle(&angle);
    if (angle != xRot)
    {
        xRot = angle;
        emit xRotationChanged(angle);
        update();
    }
}
//=============================================================================================
void objectgl::SetYRotation(int angle)
{
    NormalizeAngle(&angle);
    if (angle != yRot)
    {
        yRot = angle;
        emit yRotationChanged(angle);
        update();
    }
}
//=============================================================================================
void objectgl::SetZRotation(int angle)
{
    NormalizeAngle(&angle);
    if (angle != zRot)
    {
        zRot = angle;
        emit zRotationChanged(angle);
        update();
    }
}
//=============================================================================================
void objectgl::mousePressEvent(QMouseEvent *event){
    // Right button  (Rotate)
    if(event->buttons()==Qt::RightButton)
        LastPos = event->pos();
    // Left button (Move)
    if(event->buttons()==Qt::LeftButton)
        LastPos = event->pos();
}
//=============================================================================================
// Wheel event -->  Change the Zoom
void objectgl::wheelEvent(QWheelEvent *event)
{

    if (event->angleDelta().y() < 0)
        Zoom/= 1-(event->angleDelta().y()/120.0)/10.0;
    if(event->angleDelta().y()>0)
        Zoom*= 1+(event->angleDelta().y()/120.0)/10.0;
}
//=============================================================================================
// Mouse move event
// Update the view if necessary
void objectgl::mouseMoveEvent(QMouseEvent *event)
{
    // Left button : move
    if(event->buttons()==Qt::LeftButton)
    {
        // Compute the difference with the previous position and scale to [-0.5 ; 0.5]
        dx+= -(event->position().x() - LastPos.x() )/(double)WindowSize.width();
        dy+= -(event->position().y() - LastPos.y() )/(double)WindowSize.height();
        // Update the view according to the new position
        //        resizeGL(WindowSize.width(),WindowSize.height());
        LastPos = event->pos();
    }

    // Right button (Rotate)
    if(event->buttons()==Qt::RightButton)
    {
        // Get the difference with the previous position
        int dx_mouse = event->position().x() - LastPos.x();
        int dy_mouse = event->position().y() - LastPos.y();
        // Update the rotation
        SetXRotation(xRot - 4 * dy_mouse);
        SetYRotation(yRot + 4 * dx_mouse);

        // Memorize previous position
        LastPos = event->pos();
    }
}
//=============================================================================================

