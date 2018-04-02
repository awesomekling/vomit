// Computron x86 PC Emulator
// Copyright (C) 2003-2018 Andreas Kling <awesomekling@gmail.com>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY ANDREAS KLING ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANDREAS KLING OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "Common.h"
#include "CPU.h"
#include "debug.h"
#include "debugger.h"
#include "Tasking.h"

void CPU::_INT_imm8(Instruction& insn)
{
    interrupt(insn.imm8(), InterruptSource::Internal);
}

void CPU::_INT3(Instruction&)
{
    interrupt(3, InterruptSource::Internal);
}

void CPU::_INTO(Instruction&)
{
    /* XXX: I've never seen this used, so it's probably good to log it. */
    vlog(LogAlert, "INTO used, can you believe it?");

    if (getOF())
        interrupt(4, InterruptSource::Internal);
}

void CPU::_IRET(Instruction&)
{
    if (getPE()) {
        if (getNT()) {
            ASSERT(!getVM());
            auto tss = currentTSS();
#ifdef DEBUG_TASK_SWITCH
            vlog(LogCPU, "IRET with NT=1 switching tasks. Inner TSS @ %08X -> Outer TSS sel %04X...", TR.base, tss.getBacklink());
#endif
            taskSwitch(tss.getBacklink(), JumpType::IRET);
            return;
        }
    }

    farReturn(JumpType::IRET);
}

static WORD makeErrorCode(WORD num, bool idt, CPU::InterruptSource source)
{
    if (idt)
        return (num << 3) | 2 | (WORD)source;
    return num & 0xfc;
}

void CPU::interruptToTaskGate(BYTE, InterruptSource source, std::optional<WORD> errorCode, Gate& gate)
{
    auto descriptor = getDescriptor(gate.selector());
    if (options.trapint) {
        dumpDescriptor(descriptor);
    }
    if (!descriptor.isGlobal()) {
        throw GeneralProtectionFault(makeErrorCode(gate.selector(), 0, source), "Interrupt to task gate referencing local descriptor");
    }
    if (!descriptor.isTSS()) {
        throw GeneralProtectionFault(makeErrorCode(gate.selector(), 0, source), "Interrupt to task gate referencing non-TSS descriptor");
    }
    auto& tssDescriptor = descriptor.asTSSDescriptor();
    if (tssDescriptor.isBusy()) {
        throw GeneralProtectionFault(makeErrorCode(gate.selector(), 0, source), "Interrupt to task gate referencing busy TSS descriptor");
    }
    if (!tssDescriptor.present()) {
        throw GeneralProtectionFault(makeErrorCode(gate.selector(), 0, source), "Interrupt to task gate referencing non-present TSS descriptor");
    }
    taskSwitch(tssDescriptor, JumpType::INT);
    if (errorCode.has_value()) {
        if (tssDescriptor.is32Bit())
            push32(errorCode.value());
        else
            push16(errorCode.value());
    }
}

void CPU::realModeInterrupt(BYTE isr, InterruptSource source)
{
    ASSERT(!getPE());
    WORD originalCS = getCS();
    WORD originalIP = getIP();
    WORD flags = getFlags();
    WORD selector = readPhysicalMemory<WORD>(PhysicalAddress(isr * 4 + 2));
    WORD offset = readPhysicalMemory<WORD>(PhysicalAddress(isr * 4));

    if (options.trapint)
        vlog(LogCPU, "PE=0 interrupt %02x,%04x%s -> %04x:%04x", isr, getAX(), source == InterruptSource::External ? " (external)" : "", selector, offset);

#ifdef LOG_FAR_JUMPS
    vlog(LogCPU, "[PE=0] INT from %04x:%08x to %04x:%08x", getBaseCS(), getBaseEIP(), entry.selector, entry.offset);
#endif

    setCS(selector);
    setEIP(offset);

    push16(flags);
    push16(originalCS);
    push16(originalIP);

    setIF(0);
    setTF(0);
}

void CPU::protectedModeInterrupt(BYTE isr, InterruptSource source, std::optional<WORD> errorCode)
{
    ASSERT(getPE());
    auto gate = getInterruptGate(isr);

    if (source == InterruptSource::Internal) {
        if (gate.DPL() < getCPL()) {
            throw GeneralProtectionFault(makeErrorCode(isr, 1, source), "Software interrupt trying to escalate privilege");
        }
    }

    if (!gate.present()) {
        throw NotPresent(makeErrorCode(isr, 1, source), "Interrupt gate not present");
    }

    if (gate.isNull()) {
        throw GeneralProtectionFault(makeErrorCode(isr, 1, source), "Interrupt gate is null");
    }

    auto entry = gate.entry();

    if (options.trapint) {
        vlog(LogCPU, "PE=1 interrupt %02x,%04x%s, type: %s (%1x), %04x:%08x", isr, getAX(), source == InterruptSource::External ? " (from PIC)" : "", gate.typeName(), gate.type(), entry.selector(), entry.offset());
        dumpDescriptor(gate);
    }

    if (gate.isTaskGate()) {
        interruptToTaskGate(isr, source, errorCode, gate);
        return;
    }

    auto descriptor = getDescriptor(gate.selector());

    if (options.trapint) {
        dumpDescriptor(descriptor);
    }

    if (descriptor.isError()) {
        throw GeneralProtectionFault(makeErrorCode(gate.selector(), 0, source), "Interrupt gate to segment outside table limit");
    }

    if (!descriptor.isCode()) {
        throw GeneralProtectionFault(makeErrorCode(gate.selector(), 0, source), "Interrupt gate to non-code segment");
    }

    auto& codeDescriptor = descriptor.asCodeSegmentDescriptor();
    if (codeDescriptor.DPL() > getCPL()) {
        throw GeneralProtectionFault(makeErrorCode(gate.selector(), 0, source), QString("Interrupt gate to segment with DPL(%1)>CPL(%2)").arg(codeDescriptor.DPL()).arg(getCPL()));
    }

    if (!codeDescriptor.present()) {
        throw NotPresent(makeErrorCode(gate.selector(), 0, source), "Interrupt to non-present segment");
    }

    bool isTrap = false;
    switch (gate.type()) {
    case 0x7: // 80286 Trap Gate (16-bit)
    case 0xf: // 80386 Trap Gate (32-bit)
        isTrap = true;
        break;
    case 0x6: // 80286 Interrupt Gate (16-bit)
    case 0xe: // 80386 Interrupt Gate (32-bit)
        break;
    default:
        // FIXME: What should be the error code here?
        throw GeneralProtectionFault(isr, "Interrupt to bad gate type");
    }

    DWORD offset = gate.offset();
    DWORD flags = getEFlags();

    WORD originalSS = getSS();
    DWORD originalESP = getESP();
    WORD originalCPL = getCPL();
    WORD originalCS = getCS();
    DWORD originalEIP = getEIP();

    if (!gate.is32Bit() || !codeDescriptor.is32Bit()) {
        offset &= 0xffff;
    }

    // FIXME: Stack-related exceptions should come before this.
    if (offset > codeDescriptor.effectiveLimit()) {
        throw GeneralProtectionFault(0, "Offset outside segment limit");
    }

    if (!codeDescriptor.conforming() && descriptor.DPL() < originalCPL) {
#ifdef DEBUG_JUMPS
        vlog(LogCPU, "%s escalating privilege from ring%u to ring%u", toString(type), originalCPL, descriptor.DPL(), descriptor);
#endif
        auto tss = currentTSS();

        WORD newSS = tss.getRingSS(descriptor.DPL());
        DWORD newESP = tss.getRingESP(descriptor.DPL());
        auto newSSDescriptor = getDescriptor(newSS, SegmentRegisterIndex::SS);

        if (newSSDescriptor.isNull()) {
            throw InvalidTSS(source == InterruptSource::External, "New ss is null");
        }

        if (newSSDescriptor.isError()) {
            throw InvalidTSS(makeErrorCode(newSS, 0, source), "New ss outside table limits");
        }

        if (newSSDescriptor.DPL() != descriptor.DPL()) {
            throw InvalidTSS(makeErrorCode(newSS, 0, source), QString("New ss DPL(%1) != code segment DPL(%2)").arg(newSSDescriptor.DPL()).arg(descriptor.DPL()));
        }

        if (!newSSDescriptor.isData() || !newSSDescriptor.asDataSegmentDescriptor().writable()) {
            throw InvalidTSS(makeErrorCode(newSS, 0, source), "New ss not a writable data segment");
        }

        if (!newSSDescriptor.present()) {
            throw StackFault(makeErrorCode(newSS, 0, source), "New ss not present");
        }

        setSS(newSS);
        setESP(newESP);
        if (gate.is32Bit()) {
#ifdef DEBUG_JUMPS
            vlog(LogCPU, "INT to inner ring, ss:esp %04x:%08x -> %04x:%08x", originalSS, originalESP, getSS(), getESP());
            vlog(LogCPU, "Push 32-bit ss:esp %04x:%08x @stack{%04x:%08x}", originalSS, originalESP, getSS(), getESP());
#endif
            push32(originalSS);
            push32(originalESP);
        } else {
#ifdef DEBUG_JUMPS
            vlog(LogCPU, "INT to inner ring, ss:sp %04x:%04x -> %04x:%04x", originalSS, originalESP, getSS(), getSP());
            vlog(LogCPU, "Push 16-bit ss:sp %04x:%04x @stack{%04x:%08x}", originalSS, originalESP, getSS(), getESP());
#endif
            push16(originalSS);
            push16(originalESP);
        }
        setCPL(descriptor.DPL());
    } else if (codeDescriptor.conforming() || codeDescriptor.DPL() == originalCPL) {
#ifdef DEBUG_JUMPS
        vlog(LogCPU, "INT same privilege from ring%u to ring%u", originalCPL, descriptor.DPL());
#endif
        setCPL(originalCPL);
    } else {
        ASSERT_NOT_REACHED();
        throw GeneralProtectionFault(makeErrorCode(gate.selector(), 0, source), "Interrupt to non-conforming code segment with DPL > CPL");
    }

    if (gate.is32Bit()) {
#ifdef DEBUG_JUMPS
        vlog(LogCPU, "Push 32-bit flags %08x @stack{%04x:%08x}", flags, getSS(), getESP());
        vlog(LogCPU, "Push 32-bit cs:eip %04x:%08x @stack{%04x:%08x}", originalCS, originalEIP, getSS(), getESP());
#endif
        push32(flags);
        push32(originalCS);
        push32(originalEIP);
    } else {
#ifdef DEBUG_JUMPS
        vlog(LogCPU, "Push 16-bit flags %04x @stack{%04x:%08x}", flags, getSS(), getESP());
        vlog(LogCPU, "Push 16-bit cs:ip %04x:%04x @stack{%04x:%08x}", originalCS, originalEIP, getSS(), getESP());
#endif
        push16(flags);
        push16(originalCS);
        push16(originalEIP);
    }

    if (errorCode.has_value()) {
        if (gate.is32Bit())
            push32(errorCode.value());
        else
            push16(errorCode.value());
    }

    if (!isTrap)
        setIF(0);
    setTF(0);
    setRF(0);
    setNT(0);

    setCS(gate.selector());
    setEIP(offset);
}

void CPU::interrupt(BYTE isr, InterruptSource source, std::optional<WORD> errorCode)
{
    if (getPE())
        protectedModeInterrupt(isr, source, errorCode);
    else
        realModeInterrupt(isr, source);
}
