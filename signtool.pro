#QT       += core gui
QT       += webkitwidgets network
QT       += script
QT       -= opengl multimedia multimediawidgets printsupport qml quick sensors sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SignTool_Kamigami

RC_ICONS = image/my_icon.ico

#DEFINES += APPLICATION_FOR_FORUM_PUBLIC

HEADERS +=  \
    mainwindow.h \
    signtool.h

SOURCES +=  \
    main.cpp \
    mainwindow.cpp \
    signtool.cpp

contains( DEFINES, APPLICATION_FOR_FORUM_PUBLIC ) {
    message("APPLICATION_FOR_FORUM_PUBLIC Defined!!!")
    HEADERS +=  \
        singleapp/singleapplication.h

    SOURCES +=  \
        singleapp/singleapplication.cpp
}

FORMS   +=  \
    signtool.ui

RESOURCES += \
    image.qrc

