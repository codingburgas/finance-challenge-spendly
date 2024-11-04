QT += core gui
QT += webenginewidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ./business_logic/firebase_rest_helper.cpp \
    ./business_logic/main.cpp \
    ./business_logic/mainwindow.cpp

HEADERS += \
    ./business_logic/firebase_rest_helper.h \
    ./business_logic/mainwindow.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    ./presentation/webView.qrc

ICON = presentation/icon-mac.icns

DISTFILES +=
