TEMPLATE = app
TARGET = xdcc
DESTDIR = ../
QT += core gui network xml webkit
CONFIG += release
DEFINES += QT_LARGEFILE_SUPPORT QT_XML_LIB QT_NETWORK_LIB QT_WEBKIT_LIB
INCLUDEPATH += ./GeneratedFiles \
    ./GeneratedFiles/Release \
    ./include \
	.
LIBS += -lircclient-qt

DEPENDPATH += .
MOC_DIR += ./GeneratedFiles/release
OBJECTS_DIR += release
UI_DIR += ./GeneratedFiles
RCC_DIR += ./GeneratedFiles
include(xDCC.pri)
win32: {
    RC_FILE = xDCC.rc
	LIBS += -lwinmm -llib/WinSparkle
}
