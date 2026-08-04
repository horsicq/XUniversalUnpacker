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
#ifndef XUNIVERSALUNPACKER_H
#define XUNIVERSALUNPACKER_H

#include "xbinary.h"

// Abstract base for a "universal unpacker" module. Each concrete module (see the modules/
// folder) implements one unpacking strategy -- emulation-based, static, ... -- behind this
// common interface, so a front end can enumerate the available modules, ask which one applies
// to a given file, and drive it uniformly. The output-path layout and the module registry are
// shared here; the actual unpacking is the subclass's job.
class XUniversalUnpacker {
public:
    struct Options {
        QString inputPath;
        QString resultDirectory;
        QString packerName;          // empty -> module default / auto-detect
        bool forceOverwrite = false;
        qint64 maxSteps = 0;         // emulation step budget (0 -> module default); static modules ignore it
        bool captureApiLog = true;
        bool fixImports = true;      // reconstruct the import table so the dump is runnable
        bool fixRelocations = true;  // reconstruct base relocations for relocatable images
        bool saveOverlay = false;    // append the original file's overlay to the output
    };

    struct Result {
        bool success = false;
        QString moduleName;          // which module produced this result (getName())
        QString outputPath;
        QString packerName;          // packer/protector actually handled
        QString method;              // method that fired (OEP heuristic, "static", ...)
        quint64 oepRva = 0;          // recovered original entry point (RVA); 0 for static modules
        quint64 imageBase = 0;
        qint64 steps = 0;            // instructions executed (emulation)
        int sections = 0;
        qint64 inputSize = 0;
        qint64 outputSize = 0;
        QString inputSha256;
        QString reason;              // stop reason / diagnostics
        QStringList messages;        // human-readable summary lines
        QStringList apiLog;          // emulated OS/API calls (emulation modules)
    };

    XUniversalUnpacker();
    virtual ~XUniversalUnpacker();

    // --- the interface every module implements ---
    virtual QString getName() const = 0;                            // stable module id, e.g. "Emulator"
    virtual QString getDescription() const = 0;                     // one-line human description
    virtual bool isApplicable(const QString &inputPath) const = 0;  // can this module unpack the file?
    // Progress + cancellation flow through pPdStruct (XBinary idiom): status lines via
    // setPdStructStatus(), cancellation via isPdStructStopped(). Summary lines land in Result.messages.
    virtual Result unpack(const Options &options, XBinary::PDSTRUCT *pPdStruct) = 0;

    // --- shared output-path helpers (the layout is common to all modules) ---
    static QString defaultResultDirectory(const QString &inputPath);
    static QString resultOutputPath(const QString &inputPath, const QString &resultDirectory, const QString &methodName);

    // --- module registry ---
    static QStringList moduleNames();                                        // getName() of every module
    static XUniversalUnpacker *createModule(const QString &sName);           // caller owns; nullptr if unknown
    static QList<XUniversalUnpacker *> createAllModules();                   // caller owns each
    static XUniversalUnpacker *createApplicableModule(const QString &inputPath);  // first module whose isApplicable() is true; caller owns; nullptr if none
};

#endif  // XUNIVERSALUNPACKER_H
