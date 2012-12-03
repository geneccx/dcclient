TEMPLATE = app
TARGET = xdcc
DESTDIR = ../
QT += core gui network webkit

CONFIG += release communi
CONFIG += x86 x86_64

DEFINES += QT_LARGEFILE_SUPPORT QT_NETWORK_LIB QT_WEBKIT_LIB COMMUNI_STATIC

INCLUDEPATH += ./GeneratedFiles \
    ./GeneratedFiles/Release \
    ./include \
    .

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

macx: {
#    ICON = xdcc.icns
}
