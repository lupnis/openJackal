QT -= gui
QT += network
QT += sql

CONFIG += c++11 console
CONFIG -= app_bundle

DEFINES -= QT_DEPRECATED_WARNINGS

SOURCES += \
    Logging.cpp \
    Database.cpp \
    Configurations.cpp \
    RequestMeta.cpp \
    HeaderFetcher.cpp \
    FileFetcher.cpp \
    TaskRunner.cpp \
    NodeController.cpp \
    main.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    Logging.h \
    Database.h \
    Configurations.h \
    RequestMeta.h \
    HeaderFetcher.h \
    TaskRunner.h \
    NodeController.h \
    FileFetcher.h


unix|win32: LIBS += -L../3rdparty/hiredis/build -lhiredis
INCLUDEPATH += ../3rdparty/hiredis
DEPENDPATH += ../3rdparty/hiredis
