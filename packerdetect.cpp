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
#include "packerdetect.h"

#include <QChar>

#include "specabstract.h"
#include "xemulunpackerfactory.h"
#include "xscanengine.h"

namespace {

// Lowercase + strip everything that is not a letter or digit, so cosmetic spelling
// differences between the detector's name and the factory's name don't matter
// ("NsPack" == "NSPack", "Fish PE Packer" ~ "FishPEPacker", "!EP(EXE Pack)" ~ "EPEXEPack").
QString normalize(const QString &s)
{
    QString r;
    r.reserve(s.size());
    for (const QChar &c : s) {
        if (c.isLetterOrNumber()) {
            r.append(c.toLower());
        }
    }
    return r;
}

// Explicit aliases where the detector's name and the factory's name share no clean
// normalized prefix. Checked (as a substring of the normalized engine name) before the
// generic normalized match.
struct ALIAS {
    const char *pszEngineContains;  // normalized substring to look for in the engine name
    const char *pszFactoryName;     // exact XEmulUnpackerFactory name to use
};
const ALIAS g_aliases[] = {
    {"asdpack", "ASPack"},
    {"winupack", "(Win)Upack"},
    {"upack", "(Win)Upack"},
    {"exepack", "!EP(EXE Pack)"},
    {"nspack", "NSPack"},
    {"nsanti", "NSPack"},
    {"fishpe", "Fish PE Packer"},
    {"imploder", "ASPack"},
    {"quickpack", "QuickPack NT"},
    {"beroexe", "BeRoEXEPacker"},
    {"ahpack", "AHPacker"},
    {"revprot", "REVProt"},
    {"acprotect", "ACProtect"},
};

}  // namespace

QString PackerDetect::mapToFactoryName(const QString &sEngineName)
{
    const QString sEngineNorm = normalize(sEngineName);
    if (sEngineNorm.isEmpty()) {
        return QString();
    }

    // Alias names are intentionally more permissive than the factory-name match.
    // Keep them first, because some detector signatures are noisy (for example, "ASDPack"
    // would otherwise fail to match "ASPack" under strict prefix checks).
    for (const ALIAS &a : g_aliases) {
        if (sEngineNorm.contains(QString::fromLatin1(a.pszEngineContains))) {
            return QString::fromUtf8(a.pszFactoryName);
        }
    }

    // Prefer a real factory name when there is a clean match. The alias loop above
    // handles the ambiguous / historical spellings.
    const QStringList listFactory = XEmulUnpackerFactory::packerNames();
    QString sBest;
    int nBestLen = 0;
    for (const QString &sFactory : listFactory) {
        if (sFactory == XEmulUnpackerFactory::genericName()) {
            continue;
        }
        const QString sFactoryNorm = normalize(sFactory);
        if (sFactoryNorm.isEmpty()) {
            continue;
        }
        if (sEngineNorm.startsWith(sFactoryNorm) || sFactoryNorm.startsWith(sEngineNorm)) {
            // Keep the longest matching factory name (most specific).
            if (sFactoryNorm.size() > nBestLen) {
                sBest = sFactory;
                nBestLen = sFactoryNorm.size();
            }
        }
    }
    if (!sBest.isEmpty()) {
        return sBest;
    }

    // Fall back to the explicit aliases (substring match on the engine name).
    for (const ALIAS &a : g_aliases) {
        if (sEngineNorm.contains(QString::fromLatin1(a.pszEngineContains))) {
            return QString::fromUtf8(a.pszFactoryName);
        }
    }

    return QString();
}

PackerDetect::RESULT PackerDetect::detect(const QString &sFileName)
{
    RESULT result;

    SpecAbstract specAbstract;
    XScanEngine::SCAN_OPTIONS options = XScanEngine::getDefaultOptions(0);
    options.bIsDeepScan = true;        // catch stubs the surface scan misses
    options.bIsHeuristicScan = true;   // and heuristic packer signatures
    options.bIsRecursiveScan = false;

    XScanEngine::SCAN_RESULT scanResult = specAbstract.scanFile(sFileName, &options);

    // The detector emits records of many types (format, linker, compiler, ...); pick the
    // protector/packer/crypter one. Prefer the highest-priority such record.
    const XScanEngine::SCANSTRUCT *pBest = nullptr;
    for (const XScanEngine::SCANSTRUCT &rec : scanResult.listRecords) {
        if (XScanEngine::isProtection(rec.sType)) {
            if ((pBest == nullptr) || (rec.nPrio < pBest->nPrio)) {
                pBest = &rec;
            }
        }
    }

    if (pBest != nullptr) {
        result.bDetected = true;
        result.sDetectedName = pBest->sName;
        result.sDetectedInfo = pBest->sName;
        if (!pBest->sVersion.isEmpty()) {
            result.sDetectedInfo += QStringLiteral("(") + pBest->sVersion + QStringLiteral(")");
        }
        if (!pBest->sInfo.isEmpty()) {
            result.sDetectedInfo += QStringLiteral("[") + pBest->sInfo + QStringLiteral("]");
        }
        result.sFactoryName = mapToFactoryName(pBest->sName);
    }

    return result;
}
