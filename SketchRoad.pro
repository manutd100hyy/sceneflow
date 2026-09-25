QT += core gui widgets qml quick quickcontrols2 sql svg
CONFIG += c++14
TARGET = SketchRoad
TEMPLATE = app

DEFINES += SR_DATA_DIR=\\\"$$PWD/data\\\"
INCLUDEPATH += $$PWD/src

SOURCES += \
    src/main.cpp \
    src/app/AppController.cpp \
    src/model/Geometry.cpp \
    src/model/SceneObject.cpp \
    src/model/Catalog.cpp \
    src/model/SceneDocument.cpp \
    src/io/CaseStore.cpp \
    src/render/ScenePainter.cpp \
    src/render/SceneCanvas.cpp \
    src/forms/FormLayout.cpp \
    src/export/PdfExporter.cpp

HEADERS += \
    src/app/AppController.h \
    src/model/Geometry.h \
    src/model/SceneObject.h \
    src/model/Catalog.h \
    src/model/SceneDocument.h \
    src/io/CaseStore.h \
    src/render/ScenePainter.h \
    src/render/SceneCanvas.h \
    src/forms/FormLayout.h \
    src/export/PdfExporter.h

RESOURCES += qml/qml.qrc

android {
    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
    DISTFILES += android/AndroidManifest.xml
}
