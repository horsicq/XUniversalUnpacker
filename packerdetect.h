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
#ifndef PACKERDETECT_H
#define PACKERDETECT_H

#include <QString>

// Auto-detect the packer of a file with the Nauz File Detector scan engine
// (SpecAbstract / XScanEngine, the same engine NFD uses) and map the detected
// protector/packer name to an XEmulUnpackerFactory packer name, so the unpacker
// can always pick the correct per-packer method instead of relying on the manual
// -p selection or the generic heuristic.
namespace PackerDetect {

struct RESULT {
    QString sFactoryName;   // XEmulUnpackerFactory name to drive the unpack ("" -> use generic)
    QString sDetectedName;  // raw engine detection, e.g. "UPX"
    QString sDetectedInfo;  // full engine string, e.g. "UPX(3.94)[NRV,best]"
    bool bDetected;         // engine reported a protector/packer record

    RESULT() : bDetected(false) {}
};

// Scan sFileName and return the mapping. Never throws; returns an empty
// sFactoryName when no supported packer is recognised.
RESULT detect(const QString &sFileName);

// Map a raw engine packer name (e.g. "NsPack", "Upack") to a factory name
// (e.g. "NSPack", "(Win)Upack"). Returns "" when nothing matches. Exposed for tests.
QString mapToFactoryName(const QString &sEngineName);

}  // namespace PackerDetect

#endif  // PACKERDETECT_H
