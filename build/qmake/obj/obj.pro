!include( ../config.pri ) : error( "Unable to include config.pri" )
TEMPLATE = app
QT -=  core
CONFIG -= release debug_and_release qt
CONFIG *= debug 
win32:CONFIG *= console
macx:CONFIG -= app_bundle

TARGET = obj 
INCLUDEPATH += \
    ../../../ExchangeToolkit/include \
    $${EIGEN_PATH}

!include(../pri/exchange.pri) : error( "Unable to include exchage.pri" )

DEFINES += QT_DEPRECATED_WARNINGS
OBJECTS_DIR = .obj-$${TARGET}
MOC_DIR = .moc-$${TARGET}
DESTDIR = bin

SOURCES += ../../../examples/obj/main.cpp
