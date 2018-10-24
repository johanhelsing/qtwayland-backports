QT += testlib waylandclient-private
CONFIG += testcase wayland-scanner
QMAKE_USE += wayland-server

WAYLANDSERVERSOURCES += \
    $$PWD/../../../../src/3rdparty/protocol/wayland.xml \
    $$PWD/../../../../src/3rdparty/protocol/xdg-shell.xml

INCLUDEPATH += ../shared

#todo: sort
HEADERS += \
    $$PWD/corecompositor.h \
    $$PWD/coreprotocol.h \
    $$PWD/xdgshell.h \
    $$PWD/mockcompositor.h

SOURCES += \
    $$PWD/corecompositor.cpp \
    $$PWD/coreprotocol.cpp \
    $$PWD/xdgshell.cpp \
    $$PWD/mockcompositor.cpp
