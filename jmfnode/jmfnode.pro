QT -= gui
QT += network
QT += sql

CONFIG += c++11 console
CONFIG -= app_bundle

DEFINES -= QT_DEPRECATED_WARNINGS

SOURCES += \
    Logging.cpp \
    Database.cpp \
    RequestMeta.cpp \
    HeaderFetcher.cpp \
    FileFetcher.cpp \
    main.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    Logging.h \
    Database.h \
    RequestMeta.h \
    HeaderFetcher.h \
    FileFetcher.h


unix|win32: LIBS += -LE:/backends/hiredis/build -lhiredis
INCLUDEPATH += E:/backends/hiredis
DEPENDPATH += E:/backends/hiredis







