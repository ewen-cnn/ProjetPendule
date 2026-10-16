HEADERS += \
    mainwindow.h \
    objectgl.h \
    qcustomplot.h

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    objectgl.cpp \
    qcustomplot.cpp

QT += \
    widgets \
    openglwidgets\
    opengl \
    serialport\
    core gui widgets printsupport\
    core gui opengl widgets

FORMS += \
    mainwindow.ui

LIBS+= \
    -lopengl32
