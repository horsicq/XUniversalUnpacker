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
#include "emulatorunpacker.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QScopedPointer>

#include "packerdetect.h"
#include "xemulunpacker.h"
#include "xemulunpackerfactory.h"
#include "xbinary.h"
#include "xpe.h"

namespace {

QString safeMethodName(const QString &methodName)
{
    QString result = XBinary::convertFileNameSymbols(methodName.trimmed(), QStringLiteral("unknown"));

    for(int i = 0; i < result.size(); i++) {
        if(result.at(i).unicode() < 0x20) {
            result[i] = QLatin1Char('_');
        }
    }

    while(result.endsWith(QLatin1Char('.')) || result.endsWith(QLatin1Char(' '))) {
        result.chop(1);
    }

    return result.isEmpty() ? QStringLiteral("unknown") : result;
}

}  // namespace

QStringList EmulatorUnpacker::availablePackers()
{
    return XEmulUnpackerFactory::packerNames();
}

QString EmulatorUnpacker::genericPackerName()
{
    return XEmulUnpackerFactory::genericName();
}

QString EmulatorUnpacker::defaultResultDirectory(const QString &inputPath)
{
    if(inputPath.trimmed().isEmpty()) {
        return QString();
    }

    return QFileInfo(inputPath).absoluteDir().absolutePath();
}

QString EmulatorUnpacker::resultOutputPath(const QString &inputPath, const QString &resultDirectory, const QString &methodName)
{
    const QFileInfo inputInfo(inputPath);
    const QString originalFileName = inputInfo.fileName();
    const QString rootDirectory = resultDirectory.trimmed().isEmpty() ? defaultResultDirectory(inputPath) : resultDirectory.trimmed();

    if(originalFileName.isEmpty() || rootDirectory.isEmpty()) {
        return QString();
    }

    QString originalBaseName = inputInfo.baseName();
    QString allExtensions = inputInfo.completeSuffix();
    if(originalBaseName.isEmpty()) {
        // Treat a dotfile as a basename without an extension.
        originalBaseName = originalFileName;
        allExtensions.clear();
    }

    const QString outputDirectoryName = originalFileName + QStringLiteral(".unpacked");
    QString outputFileName = QStringLiteral("%1.unpacked[%2]").arg(originalBaseName, safeMethodName(methodName));
    if(!allExtensions.isEmpty()) {
        outputFileName += QLatin1Char('.') + allExtensions;
    }

    return QFileInfo(QDir(QDir(rootDirectory).filePath(outputDirectoryName)).filePath(outputFileName)).absoluteFilePath();
}

EmulatorUnpacker::Result EmulatorUnpacker::unpack(const Options &options, const LogCallback &onLog)
{
    Result result;
    const QString inputPath = options.inputPath.trimmed();
    const QString resultDirectory = options.resultDirectory.trimmed().isEmpty() ? defaultResultDirectory(inputPath) : options.resultDirectory.trimmed();

    auto log = [&onLog, &result](const QString &message) {
        result.messages << message;
        if(onLog) {
            onLog(message);
        }
    };

    auto fail = [&result, &log](const QString &message) {
        log(message);
        return result;
    };

    if(inputPath.isEmpty()) {
        return fail(QStringLiteral("Input file is not set."));
    }

    const QFileInfo inputInfo(inputPath);
    if(!inputInfo.exists() || !inputInfo.isFile()) {
        return fail(QStringLiteral("Input file does not exist: %1").arg(inputPath));
    }

    if(resultDirectory.isEmpty()) {
        return fail(QStringLiteral("Result directory is not set."));
    }

    const QFileInfo resultDirectoryInfo(resultDirectory);
    if(resultDirectoryInfo.exists() && !resultDirectoryInfo.isDir()) {
        return fail(QStringLiteral("Result directory is not a directory: %1").arg(resultDirectoryInfo.absoluteFilePath()));
    }

    // Hash + size of the input for the report.
    {
        QFile inputFile(inputInfo.absoluteFilePath());
        if(!inputFile.open(QIODevice::ReadOnly)) {
            return fail(QStringLiteral("Cannot open input file: %1").arg(inputFile.errorString()));
        }
        QCryptographicHash hash(QCryptographicHash::Sha256);
        if(!hash.addData(&inputFile)) {
            return fail(QStringLiteral("Cannot read input file: %1").arg(inputFile.errorString()));
        }
        result.inputSha256 = QString::fromLatin1(hash.result().toHex());
        result.inputSize = inputInfo.size();
    }

    // Select the unpacker. When the caller asked for generic auto-detection, run the NFD scan
    // engine (SpecAbstract) first and switch to the matching packer-specific unpacker -- so the
    // correct per-packer OEP method is always used. Fall back to the generic heuristic when the
    // engine recognises nothing (or an unsupported packer).
    QString sEffectivePacker = options.packerName;
    if(options.packerName.isEmpty() || (options.packerName == XEmulUnpackerFactory::genericName())) {
        const PackerDetect::RESULT detection = PackerDetect::detect(inputInfo.absoluteFilePath());
        if(detection.bDetected) {
            log(QStringLiteral("Detected: %1").arg(detection.sDetectedInfo));
        }
        if(!detection.sFactoryName.isEmpty()) {
            sEffectivePacker = detection.sFactoryName;
            log(QStringLiteral("Auto-selected unpacker: %1").arg(sEffectivePacker));
        }
    }

    QScopedPointer<XEmulUnpacker> unpacker(XEmulUnpackerFactory::create(sEffectivePacker));
    unpacker->setStopFlag(options.cancelFlag);
    result.packerName = unpacker->getPackerName();

    if(onLog) {
        QObject::connect(unpacker.data(), &XEmulUnpacker::infoMessage, unpacker.data(),
                         [&onLog](const QString &text) { onLog(text); });
        QObject::connect(unpacker.data(), &XEmulUnpacker::oepDetected, unpacker.data(),
                         [&onLog](quint64 oepRva, const QString &method) {
                             onLog(QStringLiteral("OEP detected: RVA 0x%1 (%2)").arg(oepRva, 0, 16).arg(method));
                         });
    }

    log(QStringLiteral("Input:  %1").arg(inputInfo.absoluteFilePath()));
    log(QStringLiteral("Size:   %1 bytes").arg(result.inputSize));
    log(QStringLiteral("SHA256: %1").arg(result.inputSha256));
    log(QStringLiteral("Packer: %1").arg(result.packerName));
    log(QStringLiteral("Running emulator..."));

    XEmulUnpacker::OPTIONS engineOptions = unpacker->getDefaultOptions();
    if(options.maxSteps > 0) {
        engineOptions.nMaxSteps = options.maxSteps;
    }
    engineOptions.bCaptureApiLog = options.captureApiLog;
    engineOptions.bReconstructImports = options.fixImports;
    engineOptions.bReconstructRelocs = options.fixRelocations;

    const XEmulUnpacker::RESULT engineResult = unpacker->unpack(inputInfo.absoluteFilePath(), engineOptions);

    result.method = engineResult.sMethod;
    result.oepRva = engineResult.nOEP;
    result.imageBase = engineResult.nImageBase;
    result.steps = engineResult.nSteps;
    result.sections = engineResult.nSections;
    result.reason = engineResult.sReason;
    result.apiLog = engineResult.listApiLog;

    if(!engineResult.bSuccess) {
        return fail(QStringLiteral("Unpack failed: %1").arg(engineResult.sReason));
    }

    if(engineResult.baPE.isEmpty()) {
        return fail(QStringLiteral("Unpack produced no image."));
    }

    const QString methodName = result.method.trimmed().isEmpty() ? result.packerName : result.method;
    result.outputPath = resultOutputPath(inputInfo.absoluteFilePath(), resultDirectory, methodName);
    if(result.outputPath.isEmpty()) {
        return fail(QStringLiteral("Cannot build the output path."));
    }

    const QFileInfo outputInfo(result.outputPath);
    // Prefer canonical paths (resolving symlinks and on-disk case) when both
    // sides exist, and fall back to absolute paths for a new output.
    bool sameFile = false;
    const QString inputCanonical = inputInfo.canonicalFilePath();
    const QString outputCanonical = outputInfo.canonicalFilePath();
    if(!inputCanonical.isEmpty() && !outputCanonical.isEmpty()) {
        sameFile = (inputCanonical == outputCanonical);
    } else {
        sameFile = (inputInfo.absoluteFilePath() == outputInfo.absoluteFilePath());
    }
    if(sameFile) {
        return fail(QStringLiteral("Output file must be different from the input file."));
    }

    QDir outputDir(outputInfo.absolutePath());
    if(!outputDir.exists() && !outputDir.mkpath(QStringLiteral("."))) {
        return fail(QStringLiteral("Cannot create output directory: %1").arg(outputInfo.absolutePath()));
    }

    if(outputInfo.exists() && !options.forceOverwrite) {
        return fail(QStringLiteral("Output file already exists (enable overwrite / --force): %1").arg(outputInfo.absoluteFilePath()));
    }

    // The reconstructed image, plus (optionally) the original packed file's overlay -- the
    // trailing data after the last PE section (installer payloads, appended resources, ...)
    // that the emulator never maps and so cannot rebuild. "Save overlay" carries it across.
    QByteArray outputData = engineResult.baPE;
    if(options.saveOverlay) {
        QFile overlaySource(inputInfo.absoluteFilePath());
        if(overlaySource.open(QIODevice::ReadOnly)) {
            XPE pe(&overlaySource);
            if(pe.isValid()) {
                const qint64 overlayOffset = pe.getOverlayOffset();
                const qint64 overlaySize = pe.getOverlaySize();
                if((overlaySize > 0) && (overlayOffset > 0) && overlaySource.seek(overlayOffset)) {
                    const QByteArray overlay = overlaySource.read(overlaySize);
                    outputData.append(overlay);
                    log(QStringLiteral("Overlay:  appended %1 bytes from the original file").arg(overlay.size()));
                } else {
                    log(QStringLiteral("Overlay:  none present in the original file"));
                }
            }
            overlaySource.close();
        }
    }

    // Write the reconstructed image. Re-stat the output here (it may have appeared
    // during the long emulation) and honour forceOverwrite at write time, not just
    // in the up-front check.
    const QFileInfo finalOutputInfo(result.outputPath);
    if(finalOutputInfo.exists()) {
        if(!options.forceOverwrite) {
            return fail(QStringLiteral("Output file already exists (enable overwrite / --force): %1").arg(finalOutputInfo.absoluteFilePath()));
        }
        if(!QFile::remove(finalOutputInfo.absoluteFilePath())) {
            return fail(QStringLiteral("Cannot remove existing output file: %1").arg(finalOutputInfo.absoluteFilePath()));
        }
    }

    QFile outputFile(finalOutputInfo.absoluteFilePath());
    if(!outputFile.open(QIODevice::WriteOnly)) {
        return fail(QStringLiteral("Cannot create output file: %1").arg(outputFile.errorString()));
    }
    if(outputFile.write(outputData) != outputData.size() || !outputFile.flush()) {
        const QString error = outputFile.errorString();
        outputFile.close();
        outputFile.remove();
        return fail(QStringLiteral("Write failed: %1").arg(error));
    }
    outputFile.close();
    result.outputSize = outputData.size();

    result.success = true;
    log(QStringLiteral("OEP:      RVA 0x%1").arg(result.oepRva, 0, 16));
    log(QStringLiteral("Method:   %1").arg(result.method));
    log(QStringLiteral("Sections: %1").arg(result.sections));
    log(QStringLiteral("Steps:    %1").arg(result.steps));
    log(QStringLiteral("Output:   %1 (%2 bytes)").arg(finalOutputInfo.absoluteFilePath()).arg(result.outputSize));
    log(QStringLiteral("Unpack completed."));

    return result;
}
