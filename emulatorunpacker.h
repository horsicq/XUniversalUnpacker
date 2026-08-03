/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef EMULATORUNPACKER_H
#define EMULATORUNPACKER_H

#include <QString>
#include <QStringList>

#include <atomic>
#include <functional>

// Front-end-agnostic driver around the XEmulUnpacker engine. Loads a packed
// executable, single-steps its loader stub in the emulator until the transfer to
// the original entry point, and writes the reconstructed image out. The packer can
// be auto (generic heuristic) or one of the packer-specific unpackers.
class EmulatorUnpacker
{
public:
    struct Options
    {
        QString inputPath;
        QString resultDirectory;
        QString packerName;          // empty or genericPackerName() -> generic auto
        bool forceOverwrite = false;
        qint64 maxSteps = 0;         // 0 -> the selected unpacker's default budget
        bool captureApiLog = true;
        bool fixImports = true;      // reconstruct the import table so the dump is runnable
        bool fixRelocations = true;  // reconstruct base relocations (two-base diff) for relocatable images
        bool saveOverlay = false;    // append the original packed file's overlay to the output
        const std::atomic_bool *cancelFlag = nullptr;  // caller-owned; set true to abort the run
    };

    struct Result
    {
        bool success = false;
        QString outputPath;
        QString packerName;          // unpacker actually used (its getPackerName())
        QString method;              // OEP heuristic that fired ("write-then-execute", "<packer> signature", ...)
        quint64 oepRva = 0;          // recovered original entry point (RVA)
        quint64 imageBase = 0;
        qint64 steps = 0;            // instructions executed
        int sections = 0;            // sections in the rebuilt image
        qint64 inputSize = 0;
        qint64 outputSize = 0;
        QString inputSha256;
        QString reason;              // engine stop reason / diagnostics
        QStringList messages;        // human-readable summary lines
        QStringList apiLog;          // emulated OS/API calls the stub made
    };

    // Live log sink; receives one line per engine message as it is produced.
    using LogCallback = std::function<void(const QString &)>;

    // Names selectable in Options::packerName (generic first, then each packer family).
    static QStringList availablePackers();
    static QString genericPackerName();

    static QString defaultResultDirectory(const QString &inputPath);
    static QString resultOutputPath(const QString &inputPath, const QString &resultDirectory, const QString &methodName);
    static Result unpack(const Options &options, const LogCallback &onLog = LogCallback());
};

#endif  // EMULATORUNPACKER_H
