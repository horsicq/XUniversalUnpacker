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
#include "xuniversalunpacker_emulator.h"

#include <QFile>

#include "emulatorunpacker.h"
#include "xpe.h"

XUniversalUnpackerEmulator::XUniversalUnpackerEmulator()
{
}

XUniversalUnpackerEmulator::~XUniversalUnpackerEmulator()
{
}

QString XUniversalUnpackerEmulator::getName() const
{
    return QStringLiteral("Emulator");
}

QString XUniversalUnpackerEmulator::getDescription() const
{
    return QStringLiteral("Emulation-based unpacker (runs the loader stub to the OEP and dumps the image)");
}

bool XUniversalUnpackerEmulator::isApplicable(const QString &inputPath) const
{
    // The emulation engine reconstructs PE images; treat any valid PE as unpackable here (the
    // generic heuristic still runs when no specific packer is detected).
    QFile file(inputPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    XPE pe(&file);
    const bool bResult = pe.isValid();
    file.close();

    return bResult;
}

XUniversalUnpacker::Result XUniversalUnpackerEmulator::unpack(const Options &options, XBinary::PDSTRUCT *pPdStruct)
{
    if (pPdStruct) {
        XBinary::setPdStructStatus(pPdStruct, 0, QStringLiteral("Emulation unpacking..."));
    }

    // Map the universal options onto the emulation engine's options 1:1.
    EmulatorUnpacker::Options engineOptions;
    engineOptions.inputPath = options.inputPath;
    engineOptions.resultDirectory = options.resultDirectory;
    engineOptions.packerName = options.packerName;
    engineOptions.forceOverwrite = options.forceOverwrite;
    engineOptions.maxSteps = options.maxSteps;
    engineOptions.captureApiLog = options.captureApiLog;
    engineOptions.fixImports = options.fixImports;
    engineOptions.fixRelocations = options.fixRelocations;
    engineOptions.saveOverlay = options.saveOverlay;

    // EmulatorUnpacker reports progress/diagnostics and honours cancellation through the same
    // pPdStruct we pass here; its summary lines are also returned in engineResult.messages.
    const EmulatorUnpacker::Result engineResult = EmulatorUnpacker::unpack(engineOptions, pPdStruct);

    Result result;
    result.success = engineResult.success;
    result.moduleName = getName();
    result.outputPath = engineResult.outputPath;
    result.packerName = engineResult.packerName;
    result.method = engineResult.method;
    result.oepRva = engineResult.oepRva;
    result.imageBase = engineResult.imageBase;
    result.steps = engineResult.steps;
    result.sections = engineResult.sections;
    result.inputSize = engineResult.inputSize;
    result.outputSize = engineResult.outputSize;
    result.inputSha256 = engineResult.inputSha256;
    result.reason = engineResult.reason;
    result.messages = engineResult.messages;
    result.apiLog = engineResult.apiLog;

    if (pPdStruct && !result.success && !result.reason.isEmpty()) {
        XBinary::setPdStructStatus(pPdStruct, 0, result.reason);
    }

    return result;
}
