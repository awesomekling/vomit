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

#include "CPU.h"
#include "debugger.h"

//#define DEBUG_GDT
//#define DEBUG_LDT
//#define DEBUG_IDT

void CPU::_SGDT(Instruction& insn)
{
    if (insn.modrm().isRegister()) {
        throw InvalidOpcode("SGDT with register destination");
    }
    snoop(insn.modrm().segment(), insn.modrm().offset(), MemoryAccessType::Write);
    snoop(insn.modrm().segment(), insn.modrm().offset() + 6, MemoryAccessType::Write);
    DWORD maskedBase = o16() ? (GDTR.base & 0x00ffffff) : GDTR.base;
    writeMemory16(insn.modrm().segment(), insn.modrm().offset(), GDTR.limit);
    writeMemory32(insn.modrm().segment(), insn.modrm().offset() + 2, maskedBase);
}

void CPU::_SIDT(Instruction& insn)
{
    if (insn.modrm().isRegister()) {
        throw InvalidOpcode("SIDT with register destination");
    }
    snoop(insn.modrm().segment(), insn.modrm().offset(), MemoryAccessType::Write);
    snoop(insn.modrm().segment(), insn.modrm().offset() + 6, MemoryAccessType::Write);
    DWORD maskedBase = o16() ? (IDTR.base & 0x00ffffff) : IDTR.base;
    writeMemory16(insn.modrm().segment(), insn.modrm().offset(), IDTR.limit);
    writeMemory32(insn.modrm().segment(), insn.modrm().offset() + 2, maskedBase);
}

void CPU::_SLDT_RM16(Instruction& insn)
{
    insn.modrm().writeClearing16(LDTR.segment, o32());
}

void CPU::setLDT(WORD selector)
{
    auto descriptor = getDescriptor(selector);
    DWORD base = 0;
    DWORD limit = 0;
    if (!descriptor.isNull()) {
        if (descriptor.isLDT()) {
            auto& ldtDescriptor = descriptor.asLDTDescriptor();
            if (!descriptor.present()) {
                throw NotPresent(selector, "LDT segment not present");
            }
            base = ldtDescriptor.base();
            limit = ldtDescriptor.limit();
        } else {
            throw GeneralProtectionFault(selector, "Not an LDT descriptor");
        }
    }
    LDTR.segment = selector;
    LDTR.base = base;
    LDTR.limit = limit;

#ifdef DEBUG_LDT
    vlog(LogAlert, "setLDT { segment: %04X => base:%08X, limit:%08X }", LDTR.segment, LDTR.base, LDTR.limit);
#endif
}

void CPU::_LLDT_RM16(Instruction& insn)
{
    if (!getPE()) {
        throw InvalidOpcode("LLDT not recognized in real mode");
    }

    setLDT(insn.modrm().read16());
#ifdef DEBUG_LDT
    dumpLDT();
#endif
}

void CPU::dumpLDT()
{
    for (unsigned i = 0; i < LDTR.limit; i += 8) {
        dumpDescriptor(getDescriptor(i | 4));
    }
}

void CPU::_LGDT(Instruction& insn)
{
    if (getCPL() != 0) {
        throw GeneralProtectionFault(0, "LGDT with CPL != 0");
    }
    if (insn.modrm().isRegister()) {
        throw InvalidOpcode("LGDT with register source");
    }

    DWORD base = readMemory32(insn.modrm().segment(), insn.modrm().offset() + 2);
    WORD limit = readMemory16(insn.modrm().segment(), insn.modrm().offset());
    DWORD baseMask = o32() ? 0xffffffff : 0x00ffffff;
    GDTR.base = base & baseMask;
    GDTR.limit = limit;
#ifdef DEBUG_GDT
    vlog(LogAlert, "LGDT { base:%08X, limit:%08X }", GDTR.base, GDTR.limit);
    dumpGDT();
#endif
}

void CPU::dumpGDT()
{
    for (unsigned i = 0; i < LDTR.limit; i += 8) {
        dumpDescriptor(getDescriptor(i));
    }
}

void CPU::dumpIDT()
{
    vlog(LogDump, "IDT { base:%08X, limit:%08X }", IDTR.base, IDTR.limit);
    if (getPE()) {
        for (DWORD isr = 0; isr < (IDTR.limit / 16); ++isr) {
            dumpDescriptor(getInterruptGate(isr));
        }
    }
}

void CPU::_LIDT(Instruction& insn)
{
    if (getCPL() != 0) {
        throw GeneralProtectionFault(0, "LIDT with CPL != 0");
    }
    if (insn.modrm().isRegister()) {
        throw InvalidOpcode("LIDT with register source");
    }

    DWORD base = readMemory32(insn.modrm().segment(), insn.modrm().offset() + 2);
    WORD limit = readMemory16(insn.modrm().segment(), insn.modrm().offset());
    DWORD baseMask = o32() ? 0xffffffff : 0x00ffffff;
    IDTR.base = base & baseMask;
    IDTR.limit = limit;
#if DEBUG_IDT
    dumpIDT();
#endif
}

void CPU::_CLTS(Instruction&)
{
    if (getPE()) {
        if (getCPL() != 0) {
            throw GeneralProtectionFault(0, QString("CLTS with CPL!=0(%1)").arg(getCPL()));
        }
    }
    m_CR0 &= ~(1 << 3);
}

void CPU::_LMSW_RM16(Instruction& insn)
{
    if (getPE()) {
        if (getCPL() != 0) {
            throw GeneralProtectionFault(0, QString("LMSW with CPL!=0(%1)").arg(getCPL()));
        }
    }

    WORD msw = insn.modrm().read16();
    m_CR0 = (m_CR0 & 0xFFFFFFF0) | (msw & 0x0F);
#ifdef PMODE_DEBUG
    vlog(LogCPU, "LMSW set CR0=%08X, PE=%u", getCR0(), getPE());
#endif
}

void CPU::_SMSW_RM16(Instruction& insn)
{
    auto& modrm = insn.modrm();
#ifdef PMODE_DEBUG
    vlog(LogCPU, "SMSW get LSW(CR0)=%04X, PE=%u", getCR0() & 0xFFFF, getPE());
#endif
    if (o32() && modrm.isRegister())
        modrm.write32(getCR0());
    else
        modrm.write16(getCR0() & 0xFFFF);
}

void CPU::_LAR_reg16_RM16(Instruction& insn)
{
    // FIXME: This has various ways it can fail, implement them.
    WORD selector = insn.modrm().read16() & 0xffff;
    auto descriptor = getDescriptor(selector);
    if (descriptor.isNull() || descriptor.isError()) {
        setZF(0);
        return;
    }
    insn.reg16() = descriptor.m_high & 0x00ffff00;
    setZF(1);
}

void CPU::_LAR_reg32_RM32(Instruction& insn)
{
    // FIXME: This has various ways it can fail, implement them.
    WORD selector = insn.modrm().read32() & 0xffff;
    auto descriptor = getDescriptor(selector);
    if (descriptor.isNull() || descriptor.isError()) {
        setZF(0);
        return;
    }
    insn.reg32() = descriptor.m_high & 0x00ffff00;
    setZF(1);
}

static bool isValidDescriptorForLSL(const Descriptor& descriptor)
{
    if (descriptor.isNull())
        return true;
    if (descriptor.isError())
        return true;
    if (descriptor.isSegmentDescriptor())
        return true;

    switch (descriptor.asSystemDescriptor().type()) {
    case SystemDescriptor::AvailableTSS_16bit:
    case SystemDescriptor::LDT:
    case SystemDescriptor::BusyTSS_16bit:
    case SystemDescriptor::AvailableTSS_32bit:
    case SystemDescriptor::BusyTSS_32bit:
        return true;
    default:
        return false;
    }
}

void CPU::_LSL_reg16_RM16(Instruction& insn)
{
    WORD selector = insn.modrm().read16() & 0xffff;
    auto descriptor = getDescriptor(selector);
    // FIXME: This should also fail for conforming code segments somehow.
    if (!isValidDescriptorForLSL(descriptor)) {
        setZF(0);
        return;
    }

    DWORD effectiveLimit;
    if (descriptor.isLDT())
        effectiveLimit = descriptor.asLDTDescriptor().effectiveLimit();
    else if (descriptor.isTSS())
        effectiveLimit = descriptor.asTSSDescriptor().effectiveLimit();
    else
        effectiveLimit = descriptor.asSegmentDescriptor().effectiveLimit();
    insn.reg16() = effectiveLimit;
    setZF(1);
}

void CPU::_LSL_reg32_RM32(Instruction& insn)
{
    WORD selector = insn.modrm().read16() & 0xffff;
    auto descriptor = getDescriptor(selector);
    // FIXME: This should also fail for conforming code segments somehow.
    if (descriptor.isError()) {
        setZF(0);
        return;
    }
    DWORD effectiveLimit;
    if (descriptor.isLDT())
        effectiveLimit = descriptor.asLDTDescriptor().effectiveLimit();
    else if (descriptor.isTSS())
        effectiveLimit = descriptor.asTSSDescriptor().effectiveLimit();
    else
        effectiveLimit = descriptor.asSegmentDescriptor().effectiveLimit();
    insn.reg32() = effectiveLimit;
    setZF(1);
}

const char* toString(SegmentRegisterIndex segment)
{
    switch (segment) {
    case SegmentRegisterIndex::CS: return "CS";
    case SegmentRegisterIndex::DS: return "DS";
    case SegmentRegisterIndex::ES: return "ES";
    case SegmentRegisterIndex::SS: return "SS";
    case SegmentRegisterIndex::FS: return "FS";
    case SegmentRegisterIndex::GS: return "GS";
    default: break;
    }
    return nullptr;
}

void CPU::raiseException(const Exception& e)
{
    if (options.crashOnException) {
        dumpAll();
        vlog(LogAlert, "CRASH ON EXCEPTION");
        ASSERT_NOT_REACHED();
    }

    try {
        setEIP(getBaseEIP());
        bool pushSize16;
        if (getPE()) {
            auto gate = getInterruptGate(e.num());
            pushSize16 = !gate.is32Bit();
        } else {
            pushSize16 = true;
        }
        jumpToInterruptHandler(e.num());
        if (e.hasCode()) {
            if (pushSize16)
                push16(e.code());
            else
                push32(e.code());
        }
    } catch (Exception e) {
        ASSERT_NOT_REACHED();
    }
}

Exception CPU::GeneralProtectionFault(WORD code, const QString& reason)
{
    WORD selector = code & 0xfff8;
    bool TI = code & 4;
    bool I = code & 2;
    bool EX = code & 1;
    vlog(LogCPU, "Exception: #GP(%04x) selector=%04X, TI=%u, I=%u, EX=%u :: %s", code, selector, TI, I, EX, qPrintable(reason));
    if (options.crashOnGPF) {
        dumpAll();
        vlog(LogAlert, "CRASH ON GPF");
        ASSERT_NOT_REACHED();
    }
    return Exception(0xd, code, reason);
}

Exception CPU::StackFault(WORD selector, const QString& reason)
{
    vlog(LogCPU, "Exception: #SS(%04x) :: %s", selector, qPrintable(reason));
    return Exception(0xc, selector, reason);
}

Exception CPU::NotPresent(WORD selector, const QString& reason)
{
    vlog(LogCPU, "Exception: #NP(%04x) :: %s", selector, qPrintable(reason));
    return Exception(0xb, selector, reason);
}

Exception CPU::InvalidOpcode(const QString& reason)
{
    vlog(LogCPU, "Exception: #UD :: %s", qPrintable(reason));
    return Exception(0x6, reason);
}

Exception CPU::BoundRangeExceeded(const QString& reason)
{
    vlog(LogCPU, "Exception: #BR :: %s", qPrintable(reason));
    return Exception(0x5, reason);
}

Exception CPU::InvalidTSS(WORD selector, const QString& reason)
{
    vlog(LogCPU, "Exception: #TS(%04x) :: %s", selector, qPrintable(reason));
    return Exception(0xa, selector, reason);
}

Exception CPU::DivideError(const QString& reason)
{
    vlog(LogCPU, "Exception: #DE :: %s", qPrintable(reason));
    return Exception(0x0, reason);
}

void CPU::validateSegmentLoad(SegmentRegisterIndex reg, WORD selector, const Descriptor& descriptor)
{
    if (!getPE())
        return;

    BYTE selectorRPL = selector & 3;

    if (descriptor.isError()) {
        throw GeneralProtectionFault(selector, "Selector outside table limits");
    }

    if (reg == SegmentRegisterIndex::SS) {
        if (descriptor.isNull()) {
            throw GeneralProtectionFault(0, "ss loaded with null descriptor");
        }
        if (selectorRPL != getCPL()) {
            throw GeneralProtectionFault(selector, QString("ss selector RPL(%1) != CPL(%2)").arg(selectorRPL).arg(getCPL()));
        }
        if (!descriptor.isData() || !descriptor.asDataSegmentDescriptor().writable()) {
            throw GeneralProtectionFault(selector, "ss loaded with something other than a writable data segment");
        }
        if (descriptor.DPL() != getCPL()) {
            throw GeneralProtectionFault(selector, QString("ss selector leads to descriptor with DPL(%1) != CPL(%2)").arg(descriptor.DPL()).arg(getCPL()));
        }
        if (!descriptor.present()) {
            throw StackFault(selector, "ss loaded with non-present segment");
        }
        return;
    }

    if (descriptor.isNull())
        return;

    if (reg == SegmentRegisterIndex::DS
        || reg == SegmentRegisterIndex::ES
        || reg == SegmentRegisterIndex::FS
        || reg == SegmentRegisterIndex::GS) {
        if (!descriptor.isData() && (descriptor.isCode() && !descriptor.asCodeSegmentDescriptor().readable())) {
            throw GeneralProtectionFault(selector, QString("%1 loaded with non-data or non-readable code segment").arg(registerName(reg)));
        }
        if (descriptor.isData() || descriptor.isNonconformingCode()) {
            if (selectorRPL > descriptor.DPL()) {
                throw GeneralProtectionFault(selector, QString("%1 loaded with data or non-conforming code segment and RPL > DPL").arg(registerName(reg)));
            }
            if (getCPL() > descriptor.DPL()) {
                throw GeneralProtectionFault(selector, QString("%1 loaded with data or non-conforming code segment and RPL > DPL").arg(registerName(reg)));
            }
        }
        if (!descriptor.present()) {
            throw NotPresent(selector, QString("%1 loaded with non-present segment").arg(registerName(reg)));
        }
    }

    if (!descriptor.isNull() && !descriptor.isSegmentDescriptor()) {
        dumpDescriptor(descriptor);
        throw GeneralProtectionFault(0, QString("%1 loaded with system segment").arg(registerName(reg)));
    }
}

void CPU::writeSegmentRegister(SegmentRegisterIndex segreg, WORD selector)
{
    if ((int)segreg >= 6) {
        throw InvalidOpcode("Write to invalid segment register");
    }

    auto& descriptorCache = static_cast<Descriptor&>(m_descriptor[(int)segreg]);
    auto descriptor = getDescriptor(selector, segreg);

    validateSegmentLoad(segreg, selector, descriptor);

    *m_segmentMap[(int)segreg] = selector;

    if (descriptor.isNull()) {
        descriptorCache = descriptor;
        return;
    }

    ASSERT(descriptor.isSegmentDescriptor());
    descriptorCache = descriptor.asSegmentDescriptor();
    if (options.pedebug) {
        if (getPE()) {
            vlog(LogCPU, "%s loaded with %04x { type:%02X, base:%08X, limit:%08X }",
                toString(segreg),
                selector,
                descriptor.asSegmentDescriptor().type(),
                descriptor.asSegmentDescriptor().base(),
                descriptor.asSegmentDescriptor().limit()
            );
        }
    }

    switch (segreg) {
    case SegmentRegisterIndex::CS:
        if (getPE()) {
            setCPL(descriptor.DPL());
        }
        updateDefaultSizes();
        updateCodeSegmentCache();
        break;
    case SegmentRegisterIndex::SS:
        updateStackSize();
        break;
    default:
        break;
    }
}

void CPU::_VERR_RM16(Instruction& insn)
{
    if (!getPE()) {
        throw InvalidOpcode("VERR not recognized in real mode");
    }
    WORD selector = insn.modrm().read16();
    auto descriptor = getDescriptor(selector);
    if (descriptor.isError() || descriptor.isSystemDescriptor()) {
        setZF(0);
        return;
    }
    WORD RPL = selector & 3;

    bool isConformingCode = descriptor.isCode() && descriptor.asCodeSegmentDescriptor().conforming();
    if (!isConformingCode && (getCPL() > descriptor.DPL() || RPL > descriptor.DPL())) {
        setZF(0);
        return;
    }

    if (descriptor.isCode()) {
        setZF(descriptor.asCodeSegmentDescriptor().readable());
    } else {
        // FIXME: How do I know if a data segment is "readable"? Are they always?
        setZF(1);
    }
}

void CPU::_VERW_RM16(Instruction&)
{
    ASSERT_NOT_REACHED();
}
