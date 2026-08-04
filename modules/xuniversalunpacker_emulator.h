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
#ifndef XUNIVERSALUNPACKER_EMULATOR_H
#define XUNIVERSALUNPACKER_EMULATOR_H

#include "xuniversalunpacker.h"

// Emulation-based module: loads the executable into the XEmulUnpacker CPU emulator, single-steps
// the loader stub to the original entry point (OEP) and dumps the reconstructed image. Handles
// the generic heuristic plus the packer-specific unpackers (UPX, ASPack, FSG, ...) via the NFD
// scan engine's auto-detection. This is the broad catch-all module for packed PE executables.
class XUniversalUnpackerEmulator : public XUniversalUnpacker {
public:
    XUniversalUnpackerEmulator();
    virtual ~XUniversalUnpackerEmulator() override;

    virtual QString getName() const override;
    virtual QString getDescription() const override;
    virtual bool isApplicable(const QString &inputPath) const override;
    virtual Result unpack(const Options &options, XBinary::PDSTRUCT *pPdStruct) override;
};

#endif  // XUNIVERSALUNPACKER_EMULATOR_H
