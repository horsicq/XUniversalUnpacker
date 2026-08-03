set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    add_definitions(-DNOMINMAX)
endif()

if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS _SCL_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)
    add_compile_options(/FS /bigobj)
    add_compile_options($<$<CONFIG:Debug>:/Z7> $<$<CONFIG:RelWithDebInfo>:/Z7>)
endif()

# Qt major: reuse whatever the enclosing project already found; only probe if built standalone.
if(TARGET Qt6::Core)
    set(XUNIVERSALUNPACKER_QT_MAJOR 6)
elseif(TARGET Qt5::Core)
    set(XUNIVERSALUNPACKER_QT_MAJOR 5)
else()
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
    set(XUNIVERSALUNPACKER_QT_MAJOR ${QT_VERSION_MAJOR})
endif()

# Feature switches the Nauz File Detector scan engine (SpecAbstract) needs. The engine gives
# the unpacker automatic packer detection ("always the correct method") -- the same engine NFD uses.
add_definitions(-DUSE_DEX -DUSE_PDF -DUSE_ARCHIVE -DUSE_XSIMD)

set(XUNIVERSALUNPACKER_MYLIBS "${CMAKE_CURRENT_LIST_DIR}/..")
set(XUNIVERSALUNPACKER_XEMULUNPACKER_DIR "${XUNIVERSALUNPACKER_MYLIBS}/XEmulUnpacker")

# The scan engine (SpecAbstract -> XScanEngine -> full Formats + XDEX/XPDF/XArchive +
# XStaticUnpacker + XDisasmCore + XOptions + the NFD detection modules). Include it FIRST so
# XFORMATS_SOURCES et al. are defined with the FULL format set; the emulator's smaller
# hand-picked Formats subset (pulled by xemulunpacker.cmake below) then overlaps it and is
# de-duplicated by canonical path.
include("${XUNIVERSALUNPACKER_MYLIBS}/SpecAbstract/specabstract.cmake")

# XEmulUnpacker = the emulation-unpacker engine (XEmulator core + Formats subset + the
# packer-specific unpackers + factory).
include("${XUNIVERSALUNPACKER_XEMULUNPACKER_DIR}/xemulunpacker.cmake")

# Combine both source sets and de-duplicate by CANONICAL absolute path: the scan engine and
# the emulator each list the shared Formats files (xbinary.cpp, xpe.cpp, ...) but via
# different relative spellings ("XEmulator/../Formats/x" vs "Formats/x"), so a plain string
# REMOVE_DUPLICATES would miss them and compile each twice (duplicate symbols).
set(XUNIVERSALUNPACKER_SOURCES_RAW ${SPECABSTRACT_SOURCES} ${XEMULUNPACKER_SOURCES})
set(XUNIVERSALUNPACKER_SOURCES "")
foreach(src ${XUNIVERSALUNPACKER_SOURCES_RAW})
    get_filename_component(src_abs "${src}" ABSOLUTE)
    list(APPEND XUNIVERSALUNPACKER_SOURCES "${src_abs}")
endforeach()
list(REMOVE_DUPLICATES XUNIVERSALUNPACKER_SOURCES)

add_library(xuniversalunpacker STATIC
    ${XUNIVERSALUNPACKER_SOURCES}
    emulatorunpacker.cpp
    emulatorunpacker.h
    packerdetect.cpp
    packerdetect.h
)

# Public API: the core wrapper (emulatorunpacker.h) + the auto-detector (packerdetect.h).
target_include_directories(xuniversalunpacker PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(xuniversalunpacker PRIVATE
    "${XUNIVERSALUNPACKER_XEMULUNPACKER_DIR}"
    "${XUNIVERSALUNPACKER_XEMULUNPACKER_DIR}/packers"
    "${XUNIVERSALUNPACKER_MYLIBS}/SpecAbstract"
    "${XUNIVERSALUNPACKER_MYLIBS}/SpecAbstract/modules"
    "${XUNIVERSALUNPACKER_MYLIBS}/XScanEngine"
    "${XUNIVERSALUNPACKER_MYLIBS}/XArchive/3rdparty/zlib/src"
)

target_link_libraries(xuniversalunpacker PUBLIC
    Qt${XUNIVERSALUNPACKER_QT_MAJOR}::Core
    Qt${XUNIVERSALUNPACKER_QT_MAJOR}::Concurrent
    # Widgets pulls the QT_GUI_LIB/QT_WIDGETS_LIB defines so XOptions/XFormats compile their
    # widget helper methods (setComboBox, adjustWidget, setFileTypeComboBox, ...) that the GUI's
    # NFD scan widget links against. Console consumers link this too but never create a widget.
    Qt${XUNIVERSALUNPACKER_QT_MAJOR}::Widgets
)

# DIE detection-engine dependencies. The enclosing project must add_subdirectory() the
# providers first: _mylibs/XCapstone (capstone), _mylibs/XArchive (bzip2/lzma/zlib/ppmd),
# _mylibs/Formats/xsimd (xsimd).
target_link_libraries(xuniversalunpacker PUBLIC
    capstone
    bzip2
    lzma
    zlib
    ppmd
    xsimd
)

if(WIN32)
    # XPE Authenticode parsing pulls in Wintrust / Crypt32.
    target_link_libraries(xuniversalunpacker PUBLIC Crypt32 Wintrust)
endif()
