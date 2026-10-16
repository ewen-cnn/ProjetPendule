#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qcustomplot.h"
#include "objectgl.h"
#include <cmath>

#include <QtMath>
#include <QVector>
#include <QTimer>
#include <QResizeEvent>

QVector<double> timeData;
QVector<double> penduleReal;
QVector<double> penduleTheo;
QVector<double> vitesseAngPot;
QVector<double> vitesseAngEnc;
QVector<double> posAngPot;
QVector<double> posAngEnc;
QSerialPort serial;

#define ADC_MAX         4095.0f
#define VREF            3.3
#define ZERO_G_VOLT     1.65
#define SENSITIVITY     0.330 // V/g
#define PULSES_PER_REV  1024
#define DEG_PER_COUNT   (360.0 / PULSES_PER_REV)
#define POT_ANGLE_MAX   300.0f

// Valeurs de capteurs

float ax, ay, az;
float posXPotar, posYPotar, posZPotar;
float vitXPotar, vitYPotar, vitZPotar;
float posXEnco, posYEnco, posZEnco;
float vitXEnco, vitYEnco, vitZEnco;
float dt;

// Sommes
float sumVitX = 0, sumVitY = 0, sumVitZ = 0;
float sumPosXPotar = 0, sumPosYPotar = 0, sumPosZPotar = 0;
float sumVitXPotar = 0, sumVitYPotar = 0, sumVitZPotar = 0;
float sumPosXEnco = 0, sumPosYEnco = 0, sumPosZEnco = 0;
float sumVitXEnco = 0, sumVitYEnco = 0, sumVitZEnco = 0;
float diffPosX = 0, diffPosY = 0, diffPosZ = 0;

// Moyennes
float moyVitX = 0, moyVitY = 0, moyVitZ = 0;
float moyPosXPotar = 0, moyPosYPotar = 0, moyPosZPotar = 0;
float moyVitXPotar = 0, moyVitYPotar = 0, moyVitZPotar = 0;
float moyPosXEnco = 0, moyPosYEnco = 0, moyPosZEnco = 0;
float moyVitXEnco = 0, moyVitYEnco = 0, moyVitZEnco = 0;
float diffMoyPosX = 0, diffMoyPosY = 0, diffMoyPosZ = 0;

float adc_to_voltage(uint16_t adc_val) {
    return (adc_val * VREF) / ADC_MAX;
}

float voltage_to_g(float voltage) {
    return (voltage - ZERO_G_VOLT) / SENSITIVITY;
}

/////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    timer2.start();
    ui->setupUi(this);
    // Initialisation des graphes (1 seule fois)
    ui->plot->addGraph();  // Graphe 0
    ui->plot->graph(0)->setPen(QPen(Qt::blue));
    ui->plot->graph(0)->setName("Mesure réelle");

    ui->plot2->addGraph(); // Graphe 0
    ui->plot2->graph(0)->setPen(QPen(Qt::blue));
    ui->plot2->graph(0)->setName("Potentiomètre");

    ui->plot2->addGraph(); // Graphe 1
    ui->plot2->graph(1)->setPen(QPen(Qt::red));
    ui->plot2->graph(1)->setName("Encodeur");

    ui->plot3->addGraph(); // Graphe 0
    ui->plot3->graph(0)->setPen(QPen(Qt::blue));
    ui->plot3->graph(0)->setName("Angle Potentiomètre");

    ui->plot3->addGraph(); // Graphe 1
    ui->plot3->graph(1)->setPen(QPen(Qt::red));
    ui->plot3->graph(1)->setName("Angle Encodeur");

    connect(ui->paramButton, &QPushButton::clicked, this, &MainWindow::paramButton);
    connect(ui->page1_2, &QPushButton::clicked, this, &MainWindow::afficherPage1);
    connect(ui->page3_2, &QPushButton::clicked, this, &MainWindow::afficherPage3);
    connect(ui->page2_3, &QPushButton::clicked, this, &MainWindow::afficherPage2);
    connect(ui->page1_3, &QPushButton::clicked, this, &MainWindow::afficherPage1);
    connect(ui->page2_1, &QPushButton::clicked, this, &MainWindow::afficherPage2);
    connect(ui->page3_1, &QPushButton::clicked, this, &MainWindow::afficherPage3);
    connect(ui->startSim, &QPushButton::clicked, this, &MainWindow::startSim);

    portSerie();
    opengl();
    readSerialData();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::simulation(){
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, [=]() {

            double elapsedSeconds = timer2.elapsed() / 1000.0; // millisecondes -> secondes
            int index = 0;

            while (index < timeData.size() && timeData[index] < elapsedSeconds)index++;
            if (index >= timeData.size()) index = timeData.size() - 1;

            qDebug() << "Valeur index :" << index;
            QVector<double> timeSlice = timeData.mid(0, index);
            int taille = timeData.size();

            if(penduleReal.size()<taille){
                 penduleReal.resize(taille);
            }
            ui->plot->graph(0)->setData(timeSlice, penduleReal.mid(0, index));

            ui->plot2->graph(0)->setData(timeSlice, vitesseAngPot.mid(0, index));
            ui->plot2->graph(1)->setData(timeSlice, penduleReal.mid(0, index));

            ui->plot3->graph(0)->setData(timeSlice, posAngPot.mid(0, index));
            ui->plot3->graph(1)->setData(timeSlice, posAngEnc.mid(0, index));

            ui->plot->replot();
            ui->plot2->replot();
            ui->plot3->replot();
        });
        timer->start(100);
}

void MainWindow::readSerialData() {
    static QByteArray buffer;
    static float last_angle_rad =0;
    static float vitesseAnguPot = 0;
    static float vitesseAnguEnc = 0;
    int endIndex;
    float L = 26;  //Longeur de la tige en cm

    qint64 now = timer2.elapsed();
    dt = (now - last_time_ms) / 1000.0;

    qDebug() << "Valeur now reçue :" << now;
    qDebug() << "Valeur last_time_ms reçue :" << last_time_ms;
    qDebug() << "Valeur dt reçue :" << dt;

    last_time_ms = now;

    // Lire les données disponibles
    buffer.append(serial->readAll());

    // Vérifie si on a une ligne complète (terminée par '\n')
    while ((endIndex = buffer.indexOf('\n')) != -1) {
        QByteArray line = buffer.left(endIndex);
        buffer.remove(0, endIndex + 1); // Supprime la ligne traitée du buffer

        QString cleanLine = QString::fromUtf8(line).trimmed(); // Nettoyage

        // Exemple : "AccX=0.98g;AccY=0.01g;AccZ=9.82g;Pot=1.62V"
        QStringList valeurs = cleanLine.split(", ");
        for (QStringList::const_iterator it = valeurs.constBegin(); it != valeurs.constEnd(); ++it) {
            const QString &valeur = *it;
            if (valeur.startsWith("AccX: ")) {
                float ax = voltage_to_g(adc_to_voltage(valeur.section(':', 1).remove("g").toFloat()));
                ui->acc_X->display(QString::number(ax, 'f', 1));
                qDebug() << "Valeur accX reçue :" << ax;
            } else if (valeur.startsWith("AccY: ")) {
                float ay = voltage_to_g(adc_to_voltage(valeur.section(':', 1).remove("g").toFloat()));
                ui->acc_Y->display(QString::number(ay, 'f', 1));
                qDebug() << "Valeur accY reçue :" << ay;
            } else if (valeur.startsWith("AccZ: ")) {
                float az = voltage_to_g(adc_to_voltage(valeur.section(':', 1).remove("g").toFloat()));
                ui->acc_Z->display(QString::number(az, 'f', 1));
                qDebug() << "Valeur accZ reçue :" << az;
            }
            else if (valeur.startsWith("Pot: ")) {
                int potarValue = valeur.section(':', 1).remove("g").toFloat();
                int angle_deg = (potarValue / ADC_MAX) * POT_ANGLE_MAX - 159;
                int angle_rad = angle_deg * M_PI / 180.0f;

                float posXPotar = 0 ;
                float posYPotar = L*sin(angle_rad);
                float posZPotar = -L*cos(angle_rad) ;

                vitXPotar = 0;
                vitYPotar = (posYPotar - (L*sin(last_angle_rad))) /dt;
                vitZPotar = (posZPotar - (-L*cos(last_angle_rad)))  /dt;

                vitesseAnguPot = (angle_rad - last_angle_rad) /dt;
                last_angle_rad = angle_rad;

                ui->vitesseAngle->display(angle_deg);
                /*posAngPot.append(angle_rad);
                vitesseAngPot.append(vitesseAnguPot);*/

                qDebug() << "Valeur potar :" << angle_deg;
                qDebug() << "Valeur vitessePotY :" << vitYPotar;
                qDebug() << "Valeur vitessePotZ :" << vitZPotar;

            }
            else if (valeur.startsWith("Encoder: ")) {
                int encoderValue = valeur.section(':', 1).remove("g").toFloat();

                const int max_count = 4096;
                int angle_deg = ((encoderValue % max_count)*360)/max_count;
                if (angle_deg >180) angle_deg = angle_deg - 360;
                float angle_rad = angle_deg * (M_PI / 180.0f);

                vitesseAnguEnc = (angle_rad - last_angle_rad)/ dt;
                last_angle_rad = angle_rad;

                float posXEnco = 0 ;
                ui->posX->display(posXEnco);
                float posYEnco = L*sin(angle_rad);
                ui->posY->display(posYEnco);
                float posZEnco = -L*cos(angle_rad);
                ui->posZ->display(posZEnco);

                vitXEnco = 0;
                vitYEnco = (posYEnco - (L*sin(last_angle_rad))) /dt ;
                vitZEnco = (posZEnco - (-L*cos(last_angle_rad))) /dt ;

                qDebug() << "Valeur angle_rad :" << angle_rad << ", valeur last_angle_rad" << last_angle_rad ;

                /*posAngEnc.append(angle_rad);
                penduleReal.append(vitesseAnguEnc);
                timeData.append(now / 1000.0);*/
                ui->angle->display(angle_deg);

                qDebug() << "Valeur vitesseEncoY :" << vitYEnco;
                qDebug() << "Valeur vitesseEncoZ :" << vitZEnco;
                qDebug() << "Valeur vitesseAngle :" << vitesseAnguEnc;
            }
            posAngEnc.append(angle_rad);
            penduleReal.append(vitesseAnguEnc);
            vitesseAngPot.append(vitesseAnguPot);
            vitesseAngEnc.append(vitesseAnguEnc);
            posAngPot.append(angle_rad);
            posAngEnc.append(angle_rad);
            timeData.append(now / 1000.0);
        }
        valeursMoyennes();
    }
}

void MainWindow::valeursMoyennes(){
    static int i;

    i++;

    sumVitX += ax * dt;
    sumVitY += ay * dt;
    sumVitZ += az * dt;

    sumPosXPotar += posXPotar;
    sumPosYPotar += posYPotar;
    sumPosZPotar += posZPotar;

    sumVitXPotar += vitXPotar;
    sumVitYPotar += vitYPotar;
    sumVitZPotar += vitZPotar;

    sumPosXEnco += posXEnco;
    sumPosYEnco += posYEnco;
    sumPosZEnco += posZEnco;

    sumVitXEnco += vitXEnco;
    sumVitYEnco += vitYEnco;
    sumVitZEnco += vitZEnco;

    /////////////// Calcul des valeurs moyennes ///////////////////

    moyVitX = sumVitX /i;
    ui->moyVitX->display(moyVitX);
    moyVitY = sumVitY /i;
    ui->moyVitY->display(moyVitY);
    moyVitZ = sumVitZ /i;
    ui->moyVitZ->display(moyVitZ);

    moyPosXPotar = sumPosXPotar / i;
    ui->moyPosXPotar->display(moyPosXPotar);
    moyPosYPotar = sumPosYPotar / i;
    ui->moyPosYPotar->display(moyPosYPotar);
    moyPosZPotar = sumPosZPotar / i;
    ui->moyPosZPotar->display(moyPosZPotar);

    moyVitXPotar = sumVitXPotar / i;
    ui->moyVitXPotar->display(moyVitXPotar);
    moyVitYPotar = sumVitYPotar / i;
    ui->moyVitYPotar->display(moyVitYPotar);
    moyVitZPotar = sumVitZPotar / i;
    ui->moyVitZPotar->display(moyVitZPotar);

    moyPosXEnco = sumPosXEnco / i;
    ui->moyPosXEnco->display(moyPosXEnco);
    moyPosYEnco = sumPosYEnco / i;
    ui->moyPosYEnco->display(moyPosYEnco);
    moyPosZEnco = sumPosZEnco / i;
    ui->moyPosZEnco->display(moyPosZEnco);

    moyVitXEnco = sumVitXEnco / i;
    ui->moyVitXEnco->display(moyVitXEnco);
    moyVitYEnco = sumVitYEnco / i;
    ui->moyVitYEnco->display(moyVitYEnco);
    moyVitZEnco = sumVitZEnco / i;
    ui->moyVitZEnco->display(moyVitZEnco);

    //////////////// Calcul de la différence moyenne entre les valeurs du potentiomètre par rapport à celle de l'encodeur

    diffPosX += posXEnco - posXPotar;
    diffPosY += posYEnco - posYPotar;
    diffPosZ += posZEnco - posZPotar;


    diffMoyPosX = diffPosX / i;
    ui->diffMoyPosX->display(diffMoyPosX);
    diffMoyPosY = diffPosY / i;
    ui->diffMoyPosY->display(diffMoyPosY);
    diffMoyPosZ = diffPosZ / i;
    ui->diffMoyPosZ->display(diffMoyPosZ);
}



void MainWindow::setupSimulated() {
    double m = ui->spinBoxMasse->value() * 1e-3;  // kg
    double L = ui->doubleSpinBoxLenght->value() * 1e-2;   // m
    double g = 9.81;
    double b = ui->doubleSpinBoxCoefFrottements->value();

    double theta0 = qDegreesToRadians(ui->spinBoxAngle->value());
    double omega0 = 0.0;

    double dt = 0.01;
    double T = 30.0;
    int n = static_cast<int>(T / dt);

    timeData.resize(n);
    QVector<double> theta(n), omega(n);
    timeData[0] = 0.0;
    theta[0] = theta0;
    omega[0] = omega0;

    for (int i = 1; i < n; ++i) {
        timeData[i] = i * dt;
        double alpha = - (b / m) * omega[i - 1] - (g / L) * qSin(theta[i - 1]);
        omega[i] = omega[i - 1] + alpha * dt;
        theta[i] = theta[i - 1] + omega[i] * dt;
    }

    penduleTheo.resize(n);
    for (int i = 0; i < n; ++i){
        penduleTheo[i] = qRadiansToDegrees(theta[i]);
        double f = 1.0 / (2.0 * M_PI) * sqrt((g / L) - (b / (2 * m)) * (b / (2 * m)));
        ui->FreqTheorique->display(f);
    }
    ui->plot->addGraph();
    ui->plot->graph(0)->setPen(QPen(Qt::red));
    ui->plot->graph(0)->setName("Simulation");
    ui->plot->graph(0)->setData(timeData, penduleTheo);

    ui->plot->xAxis->setRange(0, 45);
    ui->plot->yAxis->rescale();
    ui->plot->xAxis->setLabel("Temps (s)");
    ui->plot->yAxis->setLabel("Angle (°)");

    ui->plot->replot();
}

void MainWindow::paramButton() {
    setupSimulated();
}

void MainWindow::startSim() {
    simulation();
}


void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
}

void MainWindow::afficherPage1(){
    ui->stackedWidget->setCurrentIndex(0);
}

void MainWindow::afficherPage2(){
    ui->stackedWidget->setCurrentIndex(1);
}

void MainWindow::afficherPage3(){
    ui->stackedWidget->setCurrentIndex(2);
}


void MainWindow::opengl() {
    Object_GL = ui->openGLWidget;
    QMenu *ViewMenu = menuBar()->addMenu("&View");
    if (Object_GL) {

        ViewMenu->addAction(tr("Front view"), QKeySequence(tr("Ctrl+F")), Object_GL, SLOT(FrontView()));
        ViewMenu->addAction(tr("Rear view"), QKeySequence(tr("Ctrl+E")), Object_GL, SLOT(RearView()));
        ViewMenu->addAction(tr("Left view"), QKeySequence(tr("Ctrl+L")), Object_GL, SLOT(LeftView()));
        ViewMenu->addAction(tr("Right view"), QKeySequence(tr("Ctrl+R")), Object_GL, SLOT(RightView()));
        ViewMenu->addAction(tr("Top view"), QKeySequence(tr("Ctrl+T")), Object_GL, SLOT(TopView()));
        ViewMenu->addAction(tr("Bottom view"), QKeySequence(tr("Ctrl+B")), Object_GL, SLOT(BottomView()));
        ViewMenu->addAction(tr("Isometric"), QKeySequence(tr("Ctrl+I")), Object_GL, SLOT(IsometricView()));

    } else {
        qDebug() << "Erreur : Object_GL non initialisé.";
    }

    QMenu *FileMenu = menuBar()->addMenu("&File");
    FileMenu->addSeparator();
    FileMenu->addAction(tr("Quit"), QKeySequence(tr("Ctrl+Q")), qApp, SLOT(quit()));

    QTimer *timerDisplay = new QTimer(this);
    connect(timerDisplay, &QTimer::timeout, this, &MainWindow::onTimer_UpdateDisplay);
    timerDisplay->start(50);
}
void MainWindow::onTimer_UpdateDisplay() {
    phi = atan2(ay, az);
    theta = 0 ;
    psi = 0 ;
    Object_GL->setAngles(phi, theta, psi);
    qDebug() << "angles OpenGL phi:" << phi << ", theta:" << theta << ", psi:" << psi;
    Object_GL->update();
}

void MainWindow::portSerie(){

    serial = new QSerialPort(this);
    serial->setPortName("COM5");  // Remplace par ton vrai port série
    serial->setBaudRate(QSerialPort::Baud115200);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);

    if (!serial->open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Erreur", "Impossible d'ouvrir le port série !");
    }
    connect(serial, &QSerialPort::readyRead, this, &MainWindow::readSerialData);
}


