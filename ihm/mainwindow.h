#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QGridLayout>
#include <QtGui>
#include <iostream>
#include <QMainWindow>
#include <QTimer>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMenuBar>
#include <QMessageBox>
#include <QElapsedTimer>
#include <QSerialPort>
#include <QtMath>
#include "qcustomplot.h"
#include "objectgl.h"


#include <QFile>
#include <QTextStream>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


private slots:
    void onTimer_UpdateDisplay();
    void paramButton();
    void startSim();
    void simulation();
    void afficherPage1();
    void afficherPage2();
    void afficherPage3();

protected:
    void resizeEvent(QResizeEvent *) override;

private:
    Ui::MainWindow *ui;
    QTimer *timer;
    QSerialPort *serial;
    QElapsedTimer timer2;
    QVector<double> timeData;

    bool bouton1Appuye = false;
    bool bouton2Appuye = false;

    float vitesseAnguEnc = 0.0f;
    float vitesseAnguPot = 0.0f;
    float posAnguEnc = 0.0f;
    float posAnguPot = 0.0f;

    double theta, phi, psi;
    float dt, now;
    float angle_rad;
    int   angle_deg;
    float accelX, accelY, accelZ;
    float ax, ay, az;
    float posXEnco, posYEnco, posZEnco;
    float posXPotar, posYPotar, posZPotar;
    float vitXEnco, vitYEnco, vitZEnco;
    float vitXPotar, vitYPotar, vitZPotar;

    float sumVitX, sumVitY, sumVitZ;
    float sumPosXPotar, sumPosYPotar, sumPosZPotar ;
    float sumPosXEnco, sumPosYEnco, sumPosZEnco ;
    float sumVitXPotar, sumVitYPotar, sumVitZPotar;
    float sumVitXEnco, sumVitYEnco, sumVitZEnco;

    float moyPosXPotar, moyPosYPotar, moyPosZPotar ;
    float moyPosXEnco, moyPosYEnco, moyPosZEnco ;

    float moyVitX, moyVitY, moyVitZ;
    float moyVitXPotar, moyVitYPotar, moyVitZPotar;
    float moyVitXEnco, moyVitYEnco, moyVitZEnco;

    float diffPosX, diffPosY, diffPosZ;
    float diffMoyPosX, diffMoyPosY, diffMoyPosZ;


    float velocityX = 0.0f, velocityY = 0.0f, velocityZ = 0.0f;
    float last_time_ms;

    void portSerie();
    void readSerialData();
    void valeursMoyennes();
    void setupSimulated();
    void opengl();

    objectgl *Object_GL;
};

#endif // MAINWINDOW_H
