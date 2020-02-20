!include( ../config.pri ) : error( "Unable to include config.pri" )
TEMPLATE = app
QT += gui widgets
CONFIG -= release debug_and_release
CONFIG *= debug precompile_header

TARGET = viz
INCLUDEPATH += \
    ../../../ExchangeToolkit/include \
    $${EIGEN_PATH}

!include(../pri/visualize.pri) : error( "Unable to include visualize.pri" )
!include(../pri/exchange.pri) : error( "Unable to include exchange.pri" )

DEFINES += QT_DEPRECATED_WARNINGS
OBJECTS_DIR = .obj-$${TARGET}
MOC_DIR = .moc-$${TARGET}
DESTDIR = bin

base_dir = ../../../examples/viz

HEADERS += \
    $${base_dir}/ExchangeHPSBridge.h \
    $${base_dir}/HPSWidget.h

SOURCES += \
    $${base_dir}/ExchangeHPSBridge.cpp \
    $${base_dir}/HPSWidget.cpp \
    $${base_dir}/main.cpp

PRECOMPILED_HEADER = $${base_dir}/viz_pch.h
