TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += /Users/kevin/Downloads/media/opensdk/include
LIBS += -L /Users/kevin/Downloads/media/opensdk/lib

SOURCES += main.c \
    ff_transmux.c \
    ff_transcache.c \
    ff_muxer.c \
    ff_common.cpp

HEADERS += \
    ff_transmux.h \
    ff_transcache.h \
    ff_muxer.h \
    ff_common.h

