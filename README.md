# XUniversalUnpacker

A front-end-agnostic **unpacker engine** (Qt/C++) that drives the emulation-based
[XEmulUnpacker](https://github.com/horsicq/XEmulUnpacker) with automatic packer
detection. Give it a packed executable and it loads the file, single-steps the
loader stub in the CPU emulator until the transfer to the original entry point
(OEP), reconstructs the in-memory image (imports, relocations, optional overlay)
and writes a runnable dump out — no manual packer selection required.

It is the shared core behind the [XVolkolak](https://github.com/horsicq/XVolkolak)
GUI and console front ends.

## Two pieces

| File | What it does |
| --- | --- |
| `emulatorunpacker.{h,cpp}` | `EmulatorUnpacker` — the driver. Takes an `Options` struct (input path, result dir, packer name, step budget, fix-imports/relocations/overlay flags, cancel flag), runs the emulator and returns a `Result` (output path, OEP, image base, sections, API log, diagnostics). Emits a live log via a callback. |
| `packerdetect.{h,cpp}` | `PackerDetect` — auto-detection. Scans the file with the Nauz File Detector engine (SpecAbstract / XScanEngine, the same engine NFD uses) and maps the detected protector/packer to an `XEmulUnpackerFactory` name, so the correct per-packer method is chosen automatically instead of the generic heuristic. |

## Using it

`XUniversalUnpacker` is a CMake sub-project. Add it (and the `_mylibs` dependency
providers it needs) via `add_subdirectory()` and link the `xuniversalunpacker`
target:

```cmake
# Dependency providers (capstone / compression / xsimd) must be added first.
add_subdirectory(dep/XCapstone   XCapstone)
add_subdirectory(dep/XArchive    XArchive)
add_subdirectory(dep/Formats/xsimd XSIMD)

add_subdirectory(dep/XUniversalUnpacker XUniversalUnpacker)

target_link_libraries(MyApp PRIVATE xuniversalunpacker)
```

The `CMakeLists.txt` pulls the full scan engine
([SpecAbstract](https://github.com/horsicq/SpecAbstract) →
[XScanEngine](https://github.com/horsicq/XScanEngine) + the Formats set) and the
emulation-unpacker engine
([XEmulUnpacker](https://github.com/horsicq/XEmulUnpacker) →
[XEmulator](https://github.com/horsicq/XEmulator)) as sibling dependencies, then
compiles everything into one `xuniversalunpacker` static library that exposes just
the two headers above as its public API.

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
