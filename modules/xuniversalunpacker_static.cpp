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
#include "xuniversalunpacker_static.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include "xupx.h"

namespace {

// Append one summary line to the result and surface it as the current PDSTRUCT status (if any).
// A named helper rather than an inline lambda, per the project's C++ style.
void addMessage(XUniversalUnpacker::Result *pResult, XBinary::PDSTRUCT *pPdStruct, const QString &sMessage)
{
    pResult->messages << sMessage;
    if (pPdStruct) {
        XBinary::setPdStructStatus(pPdStruct, 0, sMessage);
    }
}

}  // namespace

XUniversalUnpackerStatic::XUniversalUnpackerStatic()
{
}

XUniversalUnpackerStatic::~XUniversalUnpackerStatic()
{
}

QString XUniversalUnpackerStatic::getName() const
{
    return QStringLiteral("Static (UPX)");
}

QString XUniversalUnpackerStatic::getDescription() const
{
    return QStringLiteral("Static UPX depacker (reconstructs the original file without running any code)");
}

bool XUniversalUnpackerStatic::isApplicable(const QString &inputPath) const
{
    QFile file(inputPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const bool bResult = XUPX::isValid(&file);
    file.close();

    return bResult;
}

XUniversalUnpacker::Result XUniversalUnpackerStatic::unpack(const Options &options, XBinary::PDSTRUCT *pPdStruct)
{
    Result result;
    result.moduleName = getName();
    result.packerName = QStringLiteral("UPX");
    result.method = QStringLiteral("static");

    const QString inputPath = options.inputPath.trimmed();
    const QString resultDirectory = options.resultDirectory.trimmed().isEmpty() ? defaultResultDirectory(inputPath) : options.resultDirectory.trimmed();

    if (inputPath.isEmpty()) {
        result.reason = QStringLiteral("Input file is not set.");
        addMessage(&result, pPdStruct, result.reason);
        return result;
    }

    const QFileInfo inputInfo(inputPath);
    if (!inputInfo.exists() || !inputInfo.isFile()) {
        result.reason = QStringLiteral("Input file does not exist: %1").arg(inputPath);
        addMessage(&result, pPdStruct, result.reason);
        return result;
    }

    if (resultDirectory.isEmpty()) {
        result.reason = QStringLiteral("Result directory is not set.");
        addMessage(&result, pPdStruct, result.reason);
        return result;
    }

    QFile file(inputInfo.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        result.reason = QStringLiteral("Cannot open input file: %1").arg(file.errorString());
        addMessage(&result, pPdStruct, result.reason);
        return result;
    }

    // Hash + size of the input for the report.
    {
        QCryptographicHash hash(QCryptographicHash::Sha256);
        if (hash.addData(&file)) {
            result.inputSha256 = QString::fromLatin1(hash.result().toHex());
        }
        result.inputSize = inputInfo.size();
        file.seek(0);
    }

    XUPX upx(&file);
    if (!upx.isValid(pPdStruct)) {
        result.reason = QStringLiteral("Not a UPX-packed file.");
        addMessage(&result, pPdStruct, result.reason);
        file.close();
        return result;
    }

    result.packerName = upx.compressionMethod().isEmpty() ? QStringLiteral("UPX") : QStringLiteral("UPX (%1)").arg(upx.compressionMethod());

    addMessage(&result, pPdStruct, QStringLiteral("Input:  %1").arg(inputInfo.absoluteFilePath()));
    addMessage(&result, pPdStruct, QStringLiteral("Size:   %1 bytes").arg(result.inputSize));
    addMessage(&result, pPdStruct, QStringLiteral("SHA256: %1").arg(result.inputSha256));
    addMessage(&result, pPdStruct, QStringLiteral("Packer: %1 %2").arg(result.packerName, upx.packerVersion()));
    addMessage(&result, pPdStruct, QStringLiteral("Static depacking..."));

    // Drive XStaticUnpacker's record state machine: init -> info -> unpack current record -> finish.
    // pPdStruct flows into every step, so it carries progress and honours cancellation.
    QMap<XBinary::UNPACK_PROP, QVariant> mapProperties = upx.getDefaultUnpackProperties();
    XBinary::UNPACK_STATE state = {};

    if (!upx.initUnpack(&state, mapProperties, pPdStruct)) {
        result.reason = QStringLiteral("initUnpack() failed.");
        addMessage(&result, pPdStruct, result.reason);
        file.close();
        return result;
    }

    if (state.nNumberOfRecords <= 0) {
        upx.finishUnpack(&state, pPdStruct);
        result.reason = QStringLiteral("UPX stream reported zero records.");
        addMessage(&result, pPdStruct, result.reason);
        file.close();
        return result;
    }

    const XBinary::ARCHIVERECORD record = upx.infoCurrent(&state, pPdStruct);
    const QString sOriginalName = record.mapProperties.value(XBinary::FPART_PROP_ORIGINALNAME).toString();
    if (!sOriginalName.isEmpty()) {
        addMessage(&result, pPdStruct, QStringLiteral("Record: %1").arg(sOriginalName));
    }

    result.outputPath = resultOutputPath(inputInfo.absoluteFilePath(), resultDirectory, QStringLiteral("UPX"));
    if (result.outputPath.isEmpty()) {
        upx.finishUnpack(&state, pPdStruct);
        result.reason = QStringLiteral("Cannot build the output path.");
        addMessage(&result, pPdStruct, result.reason);
        file.close();
        return result;
    }

    const QFileInfo outputInfo(result.outputPath);
    if (outputInfo.exists() && !options.forceOverwrite) {
        upx.finishUnpack(&state, pPdStruct);
        result.reason = QStringLiteral("Output file already exists (enable overwrite / --force): %1").arg(outputInfo.absoluteFilePath());
        addMessage(&result, pPdStruct, result.reason);
        file.close();
        return result;
    }

    QDir outputDir(outputInfo.absolutePath());
    if (!outputDir.exists() && !outputDir.mkpath(QStringLiteral("."))) {
        upx.finishUnpack(&state, pPdStruct);
        result.reason = QStringLiteral("Cannot create output directory: %1").arg(outputInfo.absolutePath());
        addMessage(&result, pPdStruct, result.reason);
        file.close();
        return result;
    }

    if (outputInfo.exists() && !QFile::remove(outputInfo.absoluteFilePath())) {
        upx.finishUnpack(&state, pPdStruct);
        result.reason = QStringLiteral("Cannot remove existing output file: %1").arg(outputInfo.absoluteFilePath());
        addMessage(&result, pPdStruct, result.reason);
        file.close();
        return result;
    }

    QFile outputFile(outputInfo.absoluteFilePath());
    if (!outputFile.open(QIODevice::WriteOnly)) {
        upx.finishUnpack(&state, pPdStruct);
        result.reason = QStringLiteral("Cannot create output file: %1").arg(outputFile.errorString());
        addMessage(&result, pPdStruct, result.reason);
        file.close();
        return result;
    }

    const bool bUnpacked = upx.unpackCurrent(&state, &outputFile, pPdStruct);
    outputFile.close();

    upx.moveToNext(&state, pPdStruct);
    upx.finishUnpack(&state, pPdStruct);
    file.close();

    if (!bUnpacked) {
        outputFile.remove();
        result.reason = QStringLiteral("Static depacking failed.");
        addMessage(&result, pPdStruct, result.reason);
        return result;
    }

    result.outputSize = QFileInfo(outputInfo.absoluteFilePath()).size();
    result.success = true;

    addMessage(&result, pPdStruct, QStringLiteral("Output:   %1 (%2 bytes)").arg(outputInfo.absoluteFilePath()).arg(result.outputSize));
    addMessage(&result, pPdStruct, QStringLiteral("Unpack completed."));

    return result;
}
