QT += core gui widgets testlib sql printsupport svg
CONFIG += c++14 console
CONFIG -= app_bundle
TARGET = tst_core
TEMPLATE = app

DEFINES += SR_DATA_DIR=\\\"$$PWD/../data\\\"
INCLUDEPATH += $$PWD/../src

SOURCES += tst_core.cpp \
    ../src/model/Geometry.cpp \
    ../src/model/SceneObject.cpp \
    ../src/model/Catalog.cpp \
    ../src/model/SceneDocument.cpp \
    ../src/io/CaseStore.cpp \
    ../src/render/ScenePainter.cpp \
    ../src/forms/FormLayout.cpp \
    ../src/export/PdfExporter.cpp

HEADERS += \
    ../src/model/Geometry.h \
    ../src/model/SceneObject.h \
    ../src/model/Catalog.h \
    ../src/model/SceneDocument.h \
    ../src/io/CaseStore.h \
    ../src/render/ScenePainter.h \
    ../src/forms/FormLayout.h \
    ../src/export/PdfExporter.h
