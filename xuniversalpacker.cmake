# XUniversalUnpacker - front-end-agnostic unpacker engine.
#
# Defines the `xuniversalunpacker` STATIC library: the NFD scan engine (SpecAbstract ->
# XScanEngine + the full Formats set) + the emulation-unpacker engine (XEmulUnpacker ->
# XEmulator) + the EmulatorUnpacker driver and PackerDetect auto-detector. Give it a packed
# executable and it emulates the loader stub to the OEP and dumps the reconstructed image.
#
# Usage (include from a consumer's CMakeLists, then link the target):
#   include(${CMAKE_CURRENT_LIST_DIR}/../../_mylibs/XUniversalUnpacker/xuniversalpacker.cmake)
#   target_link_libraries(MyApp PRIVATE xuniversalunpacker)
#
# Include it BEFORE a consumer's widget cmakes (nfd_widget.cmake, ...): it defines
# SPECABSTRACT_SOURCES et al. and then blanks them in the caller scope, so those widget cmakes
# add only their widget layer to the consumer target instead of recompiling the whole engine.
#
# The enclosing project must add_subdirectory() the dependency providers FIRST:
#   _mylibs/XCapstone (capstone), _mylibs/XArchive (bzip2/lzma/zlib/ppmd), _mylibs/Formats/xsimd (xsimd).
#
# Idempotent: guarded so multiple includes are harmless. Uses CMAKE_CURRENT_LIST_DIR (not
# CMAKE_CURRENT_SOURCE_DIR) so it resolves the module's own paths when included from a consumer.

if(NOT TARGET xuniversalunpacker)

# Capture the module directory up front: nested include()s below re-point CMAKE_CURRENT_LIST_DIR.
set(XUNIVERSALUNPACKER_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(XUNIVERSALUNPACKER_MYLIBS "${XUNIVERSALUNPACKER_DIR}/..")
set(XUNIVERSALUNPACKER_XEMULUNPACKER_DIR "${XUNIVERSALUNPACKER_MYLIBS}/XEmulUnpacker")

# Qt major: reuse whatever the enclosing project already found; only probe if built standalone.
if(TARGET Qt6::Core)
    set(XUNIVERSALUNPACKER_QT_MAJOR 6)
elseif(TARGET Qt5::Core)
    set(XUNIVERSALUNPACKER_QT_MAJOR 5)
else()
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
    set(XUNIVERSALUNPACKER_QT_MAJOR ${QT_VERSION_MAJOR})
endif()

# The scan engine (SpecAbstract -> XScanEngine -> full Formats + XDEX/XPDF/XArchive +
# XStaticUnpacker + XDisasmCore + XOptions + the NFD detection modules). Include it FIRST so
# XFORMATS_SOURCES et al. are defined with the FULL format set; the emulator's smaller
# hand-picked Formats subset (pulled by xemulunpacker.cmake below) then overlaps it and is
# de-duplicated by canonical path. Both includes are self-guarded (if(NOT DEFINED ..._SOURCES)).
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
    "${XUNIVERSALUNPACKER_DIR}/emulatorunpacker.cpp"
    "${XUNIVERSALUNPACKER_DIR}/emulatorunpacker.h"
    "${XUNIVERSALUNPACKER_DIR}/packerdetect.cpp"
    "${XUNIVERSALUNPACKER_DIR}/packerdetect.h"
    "${XUNIVERSALUNPACKER_DIR}/xuniversalunpacker.cpp"
    "${XUNIVERSALUNPACKER_DIR}/xuniversalunpacker.h"
    "${XUNIVERSALUNPACKER_DIR}/modules/xuniversalunpacker_emulator.cpp"
    "${XUNIVERSALUNPACKER_DIR}/modules/xuniversalunpacker_emulator.h"
    "${XUNIVERSALUNPACKER_DIR}/modules/xuniversalunpacker_static.cpp"
    "${XUNIVERSALUNPACKER_DIR}/modules/xuniversalunpacker_static.h"
)

# AUTOMOC yes (the engine has Q_OBJECT classes), AUTOUIC NO: the engine carries no .ui files.
# Leaving AUTOUIC on would make this a second AUTOUIC target in the consumer's directory scope;
# for .ui files that live outside the consumer's source tree (the widget layer under _mylibs),
# CMake generates ui_*.h to a single shared path, so a second AUTOUIC target there collides with
# the consumer's own GUI target ("ninja: multiple rules generate ui_dialogshortcuts.h").
set_target_properties(xuniversalunpacker PROPERTIES
    AUTOMOC ON
    AUTOUIC OFF
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Feature switches the NFD scan engine needs; PUBLIC so consumers that compile engine headers
# (e.g. the GUI's scan widget) inherit them. NOMINMAX likewise on Windows.
target_compile_definitions(xuniversalunpacker PUBLIC USE_DEX USE_PDF USE_ARCHIVE USE_XSIMD)
if(WIN32)
    target_compile_definitions(xuniversalunpacker PUBLIC NOMINMAX)
endif()

if(MSVC)
    target_compile_definitions(xuniversalunpacker PRIVATE _CRT_SECURE_NO_WARNINGS _SCL_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)
    target_compile_options(xuniversalunpacker PRIVATE /FS /bigobj $<$<CONFIG:Debug>:/Z7> $<$<CONFIG:RelWithDebInfo>:/Z7>)
endif()

# Public API: the module dir (emulatorunpacker.h + packerdetect.h + xuniversalunpacker.h and the
# modules/ subclasses). The public headers expose XBinary::PDSTRUCT, so Formats (xbinary.h and
# the *_def.h / subdevice.h / xsimd headers it pulls) must be PUBLIC too -- consumers that only
# link the target (e.g. the console front end) need it on their include path.
target_include_directories(xuniversalunpacker PUBLIC
    "${XUNIVERSALUNPACKER_DIR}"
    "${XUNIVERSALUNPACKER_MYLIBS}/Formats"
    "${XUNIVERSALUNPACKER_MYLIBS}/Formats/exec"
)

target_include_directories(xuniversalunpacker PRIVATE
    "${XUNIVERSALUNPACKER_DIR}/modules"
    "${XUNIVERSALUNPACKER_XEMULUNPACKER_DIR}"
    "${XUNIVERSALUNPACKER_XEMULUNPACKER_DIR}/packers"
    "${XUNIVERSALUNPACKER_MYLIBS}/SpecAbstract"
    "${XUNIVERSALUNPACKER_MYLIBS}/SpecAbstract/modules"
    "${XUNIVERSALUNPACKER_MYLIBS}/XScanEngine"
    "${XUNIVERSALUNPACKER_MYLIBS}/XStaticUnpacker"
    "${XUNIVERSALUNPACKER_MYLIBS}/XArchive/Algos/include"
)

target_link_libraries(xuniversalunpacker PUBLIC
    Qt${XUNIVERSALUNPACKER_QT_MAJOR}::Core
    Qt${XUNIVERSALUNPACKER_QT_MAJOR}::Concurrent
    # Widgets pulls the QT_GUI_LIB/QT_WIDGETS_LIB defines so XOptions/XFormats compile their
    # widget helper methods (setComboBox, adjustWidget, setFileTypeComboBox, ...) that the GUI's
    # NFD scan widget links against. Console consumers link this too but never create a widget.
    Qt${XUNIVERSALUNPACKER_QT_MAJOR}::Widgets
)

# xarchive_7zip: this library compiles XARCHIVE_SOURCES, in which
# xarchives_ip7z.cpp is unconditional (XArchive/xarchive.cmake). A static
# library does not resolve its own externals, so the failure lands on every
# consumer's executable link (~12 unresolved: CreateArchiver, GetIsArc,
# GetNumberOfFormats, GetHandlerProperty2, UString::UString,
# NWindows::NCOM::CPropVariant::*, CLimitedInStream::*). Linking it PUBLIC here
# means each consumer gets the 7-Zip objects without repeating the line.
# Consumers normally add_subdirectory(XArchive), which defines the target; a
# consumer that does not gets it from algos_7zip.cmake (self-guarded, so this
# is a no-op when the target already exists).
if(NOT TARGET xarchive_7zip)
    include("${XUNIVERSALUNPACKER_MYLIBS}/XArchive/algos_7zip.cmake")
endif()

# DIE detection-engine dependencies (targets from the provider add_subdirectory() calls).
target_link_libraries(xuniversalunpacker PUBLIC
    capstone
    bzip2
    lzma
    zlib
    ppmd
    xsimd
    xarchive_7zip
)

if(WIN32)
    # XPE Authenticode parsing pulls in Wintrust / Crypt32.
    target_link_libraries(xuniversalunpacker PUBLIC Crypt32 Wintrust)
endif()

# The engine now lives in the library. Blank the engine-level source variables in the INCLUDING
# scope (include() runs in the caller's scope) so a consumer's widget cmakes -- nfd_widget.cmake
# and friends, whose `if(NOT DEFINED <X>_SOURCES)` guards would otherwise recompile the whole
# engine into the consumer target -- add ONLY their widget layer. The consumer links
# xuniversalunpacker for the engine symbols.
set(SPECABSTRACT_SOURCES "")
set(XSCANENGINE_SOURCES "")
set(SCANITEM_SOURCES "")
set(XFORMATS_SOURCES "")
set(XDEX_SOURCES "")
set(XPDF_SOURCES "")
set(XARCHIVES_SOURCES "")
set(XSTATICUNPACKER_SOURCES "")
set(XOPTIONS_SOURCES "")
set(XDISASMCORE_SOURCES "")

endif()
