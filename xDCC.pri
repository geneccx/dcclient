HEADERS += ./xmlstructs.h \
    ./dcapifetcher.h \
    ./xdcc.h \
    ./loginform.h \
    ./resource.h \
    ./channelhandler.h \
    ./irchandler.h \
    ./commandpacket.h \
    ./gameprotocol.h \
    ./gpsprotocol.h \
    ./qgproxy.h \
    ./xdcc_version.h \
    ./settingsform.h \
    ./friendshandler.h \
    ./updateform.h

SOURCES += ./dcapifetcher.cpp \
    ./main.cpp \
    ./loginform.cpp \
    ./xdcc.cpp \
    ./channelhandler.cpp \
    ./irchandler.cpp \
    ./commandpacket.cpp \
    ./gameprotocol.cpp \
    ./gpsprotocol.cpp \
    ./qgproxy.cpp \
    ./settingsform.cpp \
    ./friendshandler.cpp \
    ./updateform.cpp

FORMS += ./xdcc_main.ui \
    ./xdcc_login.ui \
    ./xdcc_update.ui \
    ./xdcc_options.ui \

RESOURCES += xdcc.qrc
