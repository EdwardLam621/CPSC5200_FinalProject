HEADERS       = server.h
SOURCES       = server.cpp \
                main.cpp
QT           += network widgets

# install
target.path = ./build
INSTALLS += target
