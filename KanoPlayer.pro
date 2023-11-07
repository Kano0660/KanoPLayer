QT       += core gui
QT += svg
QT  += multimedia
QT       += openglwidgets
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


INCLUDEPATH +=include \
          SDL2-2.28.4/include \
          soundtouch/include \
          KanoPalyer


VPATH += src

SOURCES += \
    KanoPalyer/av_decoder.cpp \
    KanoPalyer/av_player.cpp \
    KanoPalyer/threadpool.cpp \
    KanoWidget/KanoSlider.cpp \
    KanoWidget/kanoplaylistview.cpp \
    kanolistviewwidget.cpp \
    kanoopenfile.cpp \
    kanoopenglwidget.cpp \
    kanoplayctrlbar.cpp \
    kanoplaywidget.cpp \
    kanotitlebar.cpp \
    kanowidget.cpp \
    main.cpp \
    widget.cpp

HEADERS += \
    KanoPalyer/KanoPlayer.h \
    KanoPalyer/av_clock.h \
    KanoPalyer/av_decoder.h \
    KanoPalyer/av_player.h \
    KanoPalyer/threadpool.h \
    KanoWidget/KanoSlider.h \
    KanoWidget/kanoplaylistview.h \
    kanolistviewwidget.h \
    kanoopenfile.h \
    kanoopenglwidget.h \
    kanoplayctrlbar.h \
    kanoplaylistview.h \
    kanoplaywidget.h \
    kanotitlebar.h \
    kanowidget.h \
    widget.h


LIBS += -L$$PWD/lib\
        -L$$PWD/SDL2-2.28.4/lib/x64\
        -L$$PWD/soundtouch/lib/ -lSoundTouchD_x64\
        -lavcodec\
        -lavdevice\
        -lavfilter\
        -lavformat\
        -lavutil\
        -lpostproc\
        -lswresample\
        -lswscale\
        -lSDL2\
        -lSDL2main\
        -lSDL2test\



# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    res.qrc

FORMS +=
