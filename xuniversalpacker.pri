# XUniversalUnpacker - front-end-agnostic unpacker engine (qmake).
#
# Lists only this module's own files (the EmulatorUnpacker driver + PackerDetect auto-detector),
# matching the sibling XEmulUnpacker/xemulunpacker.pri convention. A consuming .pro must also
# pull the engine this module is built on:
#   include(../SpecAbstract/specabstract.pri)   # NFD scan engine (auto packer detection)
#   include(../XEmulUnpacker/xemulunpacker.pri)  # emulation-unpacker engine
#
# The CMake build (xuniversalpacker.cmake) is the primary build and wires these automatically.

INCLUDEPATH += $$PWD
INCLUDEPATH += $$PWD/modules
DEPENDPATH += $$PWD
DEPENDPATH += $$PWD/modules

HEADERS += \
    $$PWD/emulatorunpacker.h \
    $$PWD/packerdetect.h \
    $$PWD/xuniversalunpacker.h \
    $$PWD/modules/xuniversalunpacker_emulator.h \
    $$PWD/modules/xuniversalunpacker_static.h

SOURCES += \
    $$PWD/emulatorunpacker.cpp \
    $$PWD/packerdetect.cpp \
    $$PWD/xuniversalunpacker.cpp \
    $$PWD/modules/xuniversalunpacker_emulator.cpp \
    $$PWD/modules/xuniversalunpacker_static.cpp

DISTFILES += \
    $$PWD/xuniversalpacker.cmake
