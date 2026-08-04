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
#ifndef XUNIVERSALUNPACKER_STATIC_H
#define XUNIVERSALUNPACKER_STATIC_H

#include "xuniversalunpacker.h"

// Static module: reconstructs the original file directly from the packed stream, without running
// any code, using XStaticUnpacker's algorithmic depackers. This implementation covers UPX (XUPX);
// it is exact and fast where it applies, and is preferred over emulation for those formats.
class XUniversalUnpackerStatic : public XUniversalUnpacker {
public:
    XUniversalUnpackerStatic();
    virtual ~XUniversalUnpackerStatic() override;

    virtual QString getName() const override;
    virtual QString getDescription() const override;
    virtual bool isApplicable(const QString &inputPath) const override;
    virtual Result unpack(const Options &options, XBinary::PDSTRUCT *pPdStruct) override;
};

#endif  // XUNIVERSALUNPACKER_STATIC_H
