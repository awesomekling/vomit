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

#include <stdio.h>
#include "vomit.h"
#include "vcpu.h"
#include "debug.h"
#include "disasm.h"

int VCpu::dumpDisassembled(WORD segment, DWORD offset)
{
    char disasm[64];
    char buf[512];
    char* p = buf;

    BYTE* opcode = memoryPointer(segment, offset);

    if (!opcode) {
        vlog(LogCPU, "dumpDisassembled can't dump %04X:%08X", segment, offset);
        return 0;
    }
    int width = insn_width(opcode);
    disassemble(opcode, offset, disasm, sizeof(disasm));

    p += sprintf(p, "%04X:%08X ", segment, offset);

    for (int i = 0; i < (width ? width : 7); ++i)
        p += sprintf(p, "%02X", opcode[i]);

    for (int i = 0; i < (14-((width?width:7)*2)); ++i)
        p += sprintf(p, " ");

    p += sprintf(p, " %s", disasm);

    vlog(LogDump, buf);

    /* Recurse if this is a prefix instruction. */
    if (*opcode == 0x26 || *opcode == 0x2E || *opcode == 0x36 || *opcode == 0x3E || *opcode == 0xF2 || *opcode == 0xF3)
        width += dumpDisassembled(segment, offset + width);

    return width;
}

#ifdef VOMIT_TRACE
void VCpu::dumpTrace()
{
#if 0
    fprintf(stderr,
        "%04X:%08X "
        "EAX=%08X EBX=%08X ECX=%08X EDX=%08X ESP=%08X EBP=%08X ESI=%08X EDI=%08X "
        "CR0=%08X CR1=%08X CR2=%08X CR3=%08X CR4=%08X CR5=%08X CR6=%08X CR7=%08X "
        "DR0=%08X DR1=%08X DR2=%08X DR3=%08X DR4=%08X DR5=%08X DR6=%08X DR7=%08X "
        "DS=%04X ES=%04X SS=%04X FS=%04X GS=%04X "
        "C=%u P=%u A=%u Z=%u S=%u I=%u D=%u O=%u\n",
        getCS(), getEIP(),
        getEAX(), getEBX(), getECX(), getEDX(), getESP(), getEBP(), getESI(), getEDI(),
        getCR0(), getCR1(), getCR2(), getCR3(), getCR4(), getCR5(), getCR6(), getCR7(),
        getDR0(), getDR1(), getDR2(), getDR3(), getDR4(), getDR5(), getDR6(), getDR7(),
        getDS(), getES(), getSS(), getFS(), getGS(),
        getCF(), getPF(), getAF(), getZF(),
        getSF(), getIF(), getDF(), getOF()
    );
#else
    printf(
        "%04X:%08X %02X "
        "EAX=%08X EBX=%08X ECX=%08X EDX=%08X ESP=%08X EBP=%08X ESI=%08X EDI=%08X "
        "CR0=%08X A20=%u "
        "DS=%04X ES=%04X SS=%04X FS=%04X GS=%04X "
        "C=%u P=%u A=%u Z=%u S=%u I=%u D=%u O=%u "
        "A%u O%u X%u\n",
        getCS(), getEIP(),
        codeMemory()[getEIP()],
        getEAX(), getEBX(), getECX(), getEDX(), getESP(), getEBP(), getESI(), getEDI(),
        getCR0(),
        isA20Enabled(),
        getDS(), getES(), getSS(), getFS(), getGS(),
        getCF(), getPF(), getAF(), getZF(),
        getSF(), getIF(), getDF(), getOF(),
        a16() ? 16 : 32,
        o16() ? 16 : 32,
        x16() ? 16 : 32
    );
#endif
}
#endif

void VCpu::dumpSelector(const char* segmentRegisterName, SegmentIndex segmentIndex)
{
    const SegmentSelector& selector = m_selector[segmentIndex];
    vlog(LogDump, "%s: %04X {%08X:%05X}",
        segmentRegisterName,
        getSegment(segmentIndex),
        selector.base,
        selector.limit
    );
}

const char* VCpu::registerName(VCpu::RegisterIndex16 registerIndex)
{
    switch (registerIndex) {
    case VCpu::RegisterAX:
        return "AX";
    case VCpu::RegisterBX:
        return "BX";
    case VCpu::RegisterCX:
        return "CX";
    case VCpu::RegisterDX:
        return "DX";
    case VCpu::RegisterBP:
        return "BP";
    case VCpu::RegisterSP:
        return "SP";
    case VCpu::RegisterSI:
        return "SI";
    case VCpu::RegisterDI:
        return "DI";
    }
    VM_ASSERT(false);
    return nullptr;
}

const char* VCpu::registerName(VCpu::RegisterIndex32 registerIndex)
{
    switch (registerIndex) {
    case VCpu::RegisterEAX:
        return "EAX";
    case VCpu::RegisterEBX:
        return "EBX";
    case VCpu::RegisterECX:
        return "ECX";
    case VCpu::RegisterEDX:
        return "EDX";
    case VCpu::RegisterEBP:
        return "EBP";
    case VCpu::RegisterESP:
        return "ESP";
    case VCpu::RegisterESI:
        return "ESI";
    case VCpu::RegisterEDI:
        return "EDI";
    }
    VM_ASSERT(false);
    return nullptr;
}

void VCpu::dumpWatches()
{
    for (WatchedAddress& watch : m_watches) {
        if (watch.size == ByteSize) {
            BYTE data = readUnmappedMemory8(watch.address);
            if (data != watch.lastSeenValue) {
                vlog(LogDump, "\033[32;1m%08X\033[0m [%-16s] %02X", watch.address, qPrintable(watch.name), data);
                watch.lastSeenValue = data;
            }
        } else if (watch.size == WordSize) {
            WORD data = readUnmappedMemory16(watch.address);
            if (data != watch.lastSeenValue) {
                vlog(LogDump, "\033[32;1m%08X\033[0m [%-16s] %04X", watch.address, qPrintable(watch.name), data);
                watch.lastSeenValue = data;
            }
        } else if (watch.size == DWordSize) {
            DWORD data = readUnmappedMemory32(watch.address);
            if (data != watch.lastSeenValue) {
                vlog(LogDump, "\033[32;1m%08X\033[0m [%-16s] %08X", watch.address, qPrintable(watch.name), data);
                watch.lastSeenValue = data;
            }
        }
    }
}

void VCpu::dumpAll()
{
    BYTE* csip = codeMemory();

    auto dumpRegister = [this](VCpu::RegisterIndex16 registerIndex)
    {
        if (getPE())
            vlog(LogDump, "E%s: %08X", VCpu::registerName(registerIndex), getRegister32(static_cast<VCpu::RegisterIndex32>(registerIndex)));
        else
            vlog(LogDump, "%s: %04X", VCpu::registerName(registerIndex), getRegister16(registerIndex));
    };

    dumpRegister(RegisterAX);
    dumpRegister(RegisterBX);
    dumpRegister(RegisterCX);
    dumpRegister(RegisterDX);
    dumpRegister(RegisterBP);
    dumpRegister(RegisterSP);
    dumpRegister(RegisterSI);
    dumpRegister(RegisterDI);

    if (!getPE()) {
        vlog(LogDump, "CS: %04X", getCS());
        vlog(LogDump, "DS: %04X", getDS());
        vlog(LogDump, "ES: %04X", getES());
        vlog(LogDump, "SS: %04X", getSS());
        vlog(LogDump, "FS: %04X", getFS());
        vlog(LogDump, "GS: %04X", getGS());
    } else {
        dumpSelector("CS", RegisterCS);
        dumpSelector("DS", RegisterDS);
        dumpSelector("ES", RegisterES);
        dumpSelector("SS", RegisterSS);
        dumpSelector("FS", RegisterFS);
        dumpSelector("GS", RegisterGS);
    }
    vlog(LogDump, "EIP: %08X", getEIP());

    vlog(LogDump, "CR0: %08X", getCR0());

    vlog(LogDump, "A20: %u", isA20Enabled());

    vlog(LogDump, "GDTR: {base=%08X, limit=%04X}", this->GDTR.base, this->GDTR.limit);
    vlog(LogDump, "LDTR: {base=%08X, limit=%04X}", this->LDTR.base, this->LDTR.limit);
    vlog(LogDump, "IDTR: {base=%08X, limit=%04X}", this->IDTR.base, this->IDTR.limit);

    vlog(LogDump, "C=%u P=%u A=%u Z=%u S=%u I=%u D=%u O=%u", getCF(), getPF(), getAF(), getZF(), getSF(), getIF(), getDF(), getOF());

    vlog(LogDump, "  -  (%02X %02X%02X%02X%02X%02X)", csip[0], csip[1], csip[2], csip[3], csip[4], csip[5]);

    dumpDisassembled(getBaseCS(), getBaseEIP());

    dumpMemory(getBaseCS(), getBaseEIP(), 4);

}

static inline BYTE n(BYTE b)
{
    if (b < 0x20 || ((b > 127) && (b < 160)))
        return '.';
    return b;
}

void VCpu::dumpFlatMemory(DWORD address)
{
    address &= 0xFFFFFFF0;

    BYTE* p = &m_memory[address];
    int rows = 16;

    for (int i = 0; i < rows; ++i) {
        vlog(LogDump,
            "%08X   %02X %02X %02X %02X %02X %02X %02X %02X - %02X %02X %02X %02X %02X %02X %02X %02X   %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
            (address+i*16),
            p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
            p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15],
            n(p[0]), n(p[1]), n(p[2]), n(p[3]), n(p[4]), n(p[5]), n(p[6]), n(p[7]),
            n(p[8]), n(p[9]), n(p[10]), n(p[11]), n(p[12]), n(p[13]), n(p[14]), n(p[15])
        );
        p+=16;
    }

    p = &m_memory[address];
    for (int i = 0; i < rows; ++i) {
        fprintf(stderr,
            "db 0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\n",
            p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
            p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]
        );
        p+=16;
    }
}

void VCpu::dumpRawMemory(BYTE* p)
{
    int rows = 16;
    vlog(LogDump, "Raw dump %p", p);
    for (int i = 0; i < rows; ++i) {
        fprintf(stderr,
            "db 0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\n",
            p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
            p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]
        );
        p+=16;
    }
}

void VCpu::dumpMemory(WORD segment, DWORD offset, int rows)
{
    offset &= 0xFFFFFFF0;

    BYTE* p = memoryPointer(segment, offset);
    if (!p) {
        vlog(LogCPU, "dumpMemory can't dump %04X:%08X", segment, offset);
        return;
    }

    for (int i = 0; i < rows; ++i) {
        vlog(LogDump,
            "%04X:%04X   %02X %02X %02X %02X %02X %02X %02X %02X - %02X %02X %02X %02X %02X %02X %02X %02X   %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
            segment, (offset+i*16),
            p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
            p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15],
            n(p[0]), n(p[1]), n(p[2]), n(p[3]), n(p[4]), n(p[5]), n(p[6]), n(p[7]),
            n(p[8]), n(p[9]), n(p[10]), n(p[11]), n(p[12]), n(p[13]), n(p[14]), n(p[15])
        );
        p+=16;
    }

    p = memoryPointer(segment, offset);
    for (int i = 0; i < rows; ++i) {
        fprintf(stderr,
            "db 0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X,0x%02X\n",
            p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
            p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]
        );
        p+=16;
    }
}

static inline WORD isrSegment(const VCpu& cpu, BYTE isr)
{
    return cpu.readUnmappedMemory16(isr * 4) + 2;
}

static inline WORD isrOffset(const VCpu& cpu, BYTE isr)
{
    return cpu.readUnmappedMemory16(isr * 4);
}

void VCpu::dumpIVT()
{
    // XXX: For alignment reasons, we're skipping INT FF
    for (int i = 0; i < 0xFF; i += 4) {
        vlog(LogDump,
            "%02X>  %04X:%04X\t%02X>  %04X:%04X\t%02X>  %04X:%04X\t%02X>  %04X:%04X",
            i, isrSegment(*this, i), isrOffset(*this, i),
            i + 1, isrSegment(*this, i + 1), isrOffset(*this, i + 1),
            i + 2, isrSegment(*this, i + 2), isrOffset(*this, i + 2),
            i + 3, isrSegment(*this, i + 3), isrOffset(*this, i + 3)
        );
    }
}

void VCpu::dumpSegment(WORD index)
{
    SegmentSelector selector = makeSegmentSelector(index);

    vlog(LogCPU, "Segment 0x%04X: { base: 0x%08X, limit: %06X, bits: %u, present: %s, granularity: %s, DPL: %u }",
        index,
        selector.base,
        selector.limit,
        selector._32bit ? 32 : 16,
        selector.present ? "yes" : "no",
        selector.granularity ? "4K" : "1b",
        selector.DPL
    );
}

