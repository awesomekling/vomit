/*
 * Copyright (C) 2003-2018 Andreas Kling <awesomekling@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY ANDREAS KLING ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANDREAS KLING OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef VGA_MEMORY_H
#define VGA_MEMORY_H

#include "types.h"
#include <QtCore/QRect>
#include <QtGui/QBrush>
#include <QtGui/QColor>

class Machine;

class VGAMemory
{
public:
    explicit VGAMemory(Machine&);
    ~VGAMemory();

    template<typename T> void write(DWORD address, T value);
    template<typename T> T read(DWORD address);

    void write8(DWORD address, BYTE value);
    BYTE read8(DWORD address);

    /*!
        Returns the specified pixel plane.
	Valid indices are 0 through 3.
     */
    BYTE *plane(int index) const;

private:
    void synchronizeColors();

    Machine& machine() const { return m_machine; }
    Machine& m_machine;

    QColor m_color[16];
    QBrush m_brush[16];

    BYTE* m_plane[4];
    BYTE m_latch[4];
};

inline bool addressIsInVGAMemory(DWORD address)
{
    return address >= 0xA0000 && address < 0xB0000;
}

#endif
