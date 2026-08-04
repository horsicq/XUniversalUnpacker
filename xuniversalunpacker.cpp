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
#include "xuniversalunpacker.h"

#include <QDir>
#include <QFileInfo>

#include "xbinary.h"

#include "modules/xuniversalunpacker_emulator.h"
#include "modules/xuniversalunpacker_static.h"

namespace {

// Sanitise a method/packer name into a filesystem-safe token for the output filename.
QString safeMethodName(const QString &methodName)
{
    QString result = XBinary::convertFileNameSymbols(methodName.trimmed(), QStringLiteral("unknown"));

    for (int i = 0; i < result.size(); i++) {
        if (result.at(i).unicode() < 0x20) {
            result[i] = QLatin1Char('_');
        }
    }

    while (result.endsWith(QLatin1Char('.')) || result.endsWith(QLatin1Char(' '))) {
        result.chop(1);
    }

    return result.isEmpty() ? QStringLiteral("unknown") : result;
}

}  // namespace

XUniversalUnpacker::XUniversalUnpacker()
{
}

XUniversalUnpacker::~XUniversalUnpacker()
{
}

QString XUniversalUnpacker::defaultResultDirectory(const QString &inputPath)
{
    if (inputPath.trimmed().isEmpty()) {
        return QString();
    }

    return QFileInfo(inputPath).absoluteDir().absolutePath();
}

QString XUniversalUnpacker::resultOutputPath(const QString &inputPath, const QString &resultDirectory, const QString &methodName)
{
    const QFileInfo inputInfo(inputPath);
    const QString originalFileName = inputInfo.fileName();
    const QString rootDirectory = resultDirectory.trimmed().isEmpty() ? defaultResultDirectory(inputPath) : resultDirectory.trimmed();

    if (originalFileName.isEmpty() || rootDirectory.isEmpty()) {
        return QString();
    }

    QString originalBaseName = inputInfo.baseName();
    QString allExtensions = inputInfo.completeSuffix();
    if (originalBaseName.isEmpty()) {
        // Treat a dotfile as a basename without an extension.
        originalBaseName = originalFileName;
        allExtensions.clear();
    }

    const QString outputDirectoryName = originalFileName + QStringLiteral(".unpacked");
    QString outputFileName = QStringLiteral("%1.unpacked[%2]").arg(originalBaseName, safeMethodName(methodName));
    if (!allExtensions.isEmpty()) {
        outputFileName += QLatin1Char('.') + allExtensions;
    }

    return QFileInfo(QDir(QDir(rootDirectory).filePath(outputDirectoryName)).filePath(outputFileName)).absoluteFilePath();
}

QStringList XUniversalUnpacker::moduleNames()
{
    QStringList listNames;

    const QList<XUniversalUnpacker *> listModules = createAllModules();
    for (int i = 0; i < listModules.size(); i++) {
        listNames.append(listModules.at(i)->getName());
        delete listModules.at(i);
    }

    return listNames;
}

XUniversalUnpacker *XUniversalUnpacker::createModule(const QString &sName)
{
    const QList<XUniversalUnpacker *> listModules = createAllModules();

    XUniversalUnpacker *pResult = nullptr;
    for (int i = 0; i < listModules.size(); i++) {
        XUniversalUnpacker *pModule = listModules.at(i);
        if ((pResult == nullptr) && (pModule->getName() == sName)) {
            pResult = pModule;  // keep the match; delete the rest below
        } else {
            delete pModule;
        }
    }

    return pResult;
}

QList<XUniversalUnpacker *> XUniversalUnpacker::createAllModules()
{
    QList<XUniversalUnpacker *> listResult;

    // Order = the preference order used by createApplicableModule(): the precise static module
    // first, the generic emulation module as the catch-all fallback.
    listResult.append(new XUniversalUnpackerStatic());
    listResult.append(new XUniversalUnpackerEmulator());

    return listResult;
}

XUniversalUnpacker *XUniversalUnpacker::createApplicableModule(const QString &inputPath)
{
    const QList<XUniversalUnpacker *> listModules = createAllModules();

    XUniversalUnpacker *pResult = nullptr;
    for (int i = 0; i < listModules.size(); i++) {
        XUniversalUnpacker *pModule = listModules.at(i);
        if ((pResult == nullptr) && pModule->isApplicable(inputPath)) {
            pResult = pModule;
        } else {
            delete pModule;
        }
    }

    return pResult;
}
