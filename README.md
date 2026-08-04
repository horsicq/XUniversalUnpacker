# XUniversalUnpacker

A front-end-agnostic **unpacker engine** (Qt/C++) that drives the emulation-based
[XEmulUnpacker](https://github.com/horsicq/XEmulUnpacker) with automatic packer
detection. Give it a packed executable and it loads the file, single-steps the
loader stub in the CPU emulator until the transfer to the original entry point
(OEP), reconstructs the in-memory image (imports, relocations, optional overlay)
and writes a runnable dump out — no manual packer selection required.

It is the shared core behind the [XVolkolak](https://github.com/horsicq/XVolkolak)
GUI and console front ends.

## Architecture

`XUniversalUnpacker` (`xuniversalunpacker.{h,cpp}`) is the **abstract base**: it declares one
common interface — `getName()`, `getDescription()`, `isApplicable(inputPath)`,
`unpack(Options, XBinary::PDSTRUCT *)` (progress + cancellation flow through the `PDSTRUCT`) —
plus the shared output-path layout and a small module registry
(`moduleNames()`, `createModule()`, `createAllModules()`, `createApplicableModule()`). Each
concrete unpacking strategy is a subclass in the **`modules/`** folder:

| Module | Class | Strategy |
| --- | --- | --- |
| `modules/xuniversalunpacker_static.{h,cpp}` | `XUniversalUnpackerStatic` | Static depacking — reconstructs the original directly from the packed stream with no code execution (covers UPX via `XUPX`). Exact and fast; tried first. |
| `modules/xuniversalunpacker_emulator.{h,cpp}` | `XUniversalUnpackerEmulator` | Emulation — runs the loader stub in the CPU emulator to the OEP and dumps the image (generic + the 21 packer-specific unpackers). The broad catch-all. |

`createApplicableModule(inputPath)` returns the first module whose `isApplicable()` is true,
in that preference order (precise static first, generic emulation last). Add a new unpacker by
dropping another `XUniversalUnpacker` subclass into `modules/` and registering it in
`createAllModules()`.

### Underlying pieces

| File | What it does |
| --- | --- |
| `emulatorunpacker.{h,cpp}` | `EmulatorUnpacker` — the emulation driver the emulator module wraps. Takes an `Options` struct (input path, result dir, packer name, step budget, fix-imports/relocations/overlay flags, cancel flag), runs the emulator and returns a `Result` (output path, OEP, image base, sections, API log, diagnostics). Emits a live log via a callback. |
| `packerdetect.{h,cpp}` | `PackerDetect` — auto-detection. Scans the file with the Nauz File Detector engine (SpecAbstract / XScanEngine, the same engine NFD uses) and maps the detected protector/packer to an `XEmulUnpackerFactory` name, so the correct per-packer method is chosen automatically instead of the generic heuristic. |

## Using it

`XUniversalUnpacker` is included as a `.cmake` (like the other `_mylibs` libraries).
`include(xuniversalpacker.cmake)` defines the `xuniversalunpacker` static library
target; link it. The `_mylibs` dependency providers must be added first:

```cmake
# Dependency providers (capstone / compression / xsimd) must be added first.
add_subdirectory(dep/XCapstone   XCapstone)
add_subdirectory(dep/XArchive    XArchive)
add_subdirectory(dep/Formats/xsimd XSIMD)

# Include BEFORE any widget cmakes (nfd_widget.cmake, ...): it defines and then blanks the
# engine source variables so those cmakes add only their widget layer, not the whole engine.
include(dep/XUniversalUnpacker/xuniversalpacker.cmake)

target_link_libraries(MyApp PRIVATE xuniversalunpacker)
```

`xuniversalpacker.cmake` pulls the full scan engine
([SpecAbstract](https://github.com/horsicq/SpecAbstract) →
[XScanEngine](https://github.com/horsicq/XScanEngine) + the Formats set) and the
emulation-unpacker engine
([XEmulUnpacker](https://github.com/horsicq/XEmulUnpacker) →
[XEmulator](https://github.com/horsicq/XEmulator)) as sibling dependencies, then
compiles everything into one `xuniversalunpacker` static library that exposes just
the two headers above as its public API. (`xuniversalpacker.pri` is the qmake
counterpart.)

## Public API sketch

```cpp
#include "emulatorunpacker.h"

EmulatorUnpacker::Options options;
options.inputPath       = "packed.exe";
options.resultDirectory = "out";
// options.packerName left empty -> PackerDetect picks the method automatically.

EmulatorUnpacker::Result result = EmulatorUnpacker::unpack(options);
if (result.success) {
    // result.outputPath, result.oepRva, result.packerName, result.method, ...
}
```
