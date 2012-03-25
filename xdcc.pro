TEMPLATE = app
TARGET = xdcc
DESTDIR = ../
QT += core gui network webkit

CONFIG += release

CONFIG += communi

DEFINES += QT_LARGEFILE_SUPPORT QT_NETWORK_LIB QT_WEBKIT_LIB

INCLUDEPATH += ./GeneratedFiles \
    ./GeneratedFiles/Release \
    ./include \
    .

LIBS += -Llib -lircclient-qt

DEPENDPATH += .

MOC_DIR += ./GeneratedFiles/release
OBJECTS_DIR += release
UI_DIR += ./GeneratedFiles
RCC_DIR += ./GeneratedFiles

include(xdcc.pri)

win32: {
    RC_FILE = xdcc.rc
    LIBS += -lwinmm -lWinSparkle
}

QMAKE_CXXFLAGS += /J
