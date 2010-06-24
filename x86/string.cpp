// x86/string.c
// String instructions

#include "vcpu.h"

void _LODSB(VCpu* cpu)
{
    if (cpu->a16()) {
        cpu->regs.B.AL = cpu->readMemory8(cpu->currentSegment(), cpu->getSI());
        cpu->nextSI(1);
    } else {
        cpu->regs.B.AL = cpu->readMemory8(cpu->currentSegment(), cpu->getESI());
        cpu->nextESI(1);
    }
}

void _LODSW(VCpu* cpu)
{
    if (cpu->a16()) {
        cpu->regs.W.AX = cpu->readMemory16(cpu->currentSegment(), cpu->getSI());
        cpu->nextSI(2);
    } else {
        cpu->regs.W.AX = cpu->readMemory16(cpu->currentSegment(), cpu->getESI());
        cpu->nextESI(2);
    }
}

void _LODSD(VCpu* cpu)
{
    if (cpu->a16()) {
        cpu->regs.D.EAX = cpu->readMemory32(cpu->currentSegment(), cpu->getSI());
        cpu->nextSI(4);
    } else {
        cpu->regs.D.EAX = cpu->readMemory32(cpu->currentSegment(), cpu->getESI());
        cpu->nextESI(4);
    }
}

void _STOSB(VCpu* cpu)
{
    if (cpu->a16()) {
        cpu->writeMemory8(cpu->getES(), cpu->getDI(), cpu->getAL());
        cpu->nextDI(1);
    } else {
        cpu->writeMemory8(cpu->getES(), cpu->getEDI(), cpu->getAL());
        cpu->nextEDI(1);
    }
}

void _STOSW(VCpu* cpu)
{
    if (cpu->a16()) {
        cpu->writeMemory16(cpu->getES(), cpu->getDI(), cpu->getAX());
        cpu->nextDI(2);
    } else {
        cpu->writeMemory16(cpu->getES(), cpu->getEDI(), cpu->getAX());
        cpu->nextEDI(2);
    }
}

void _STOSD(VCpu* cpu)
{
    if (cpu->a16()) {
        cpu->writeMemory32(cpu->getES(), cpu->getDI(), cpu->getEAX());
        cpu->nextDI(4);
    } else {
        cpu->writeMemory32(cpu->getES(), cpu->getEDI(), cpu->getEAX());
        cpu->nextEDI(4);
    }
}

void _CMPSB(VCpu* cpu)
{
    BYTE src;
    BYTE dest;

    if (cpu->a16()) {
        src = cpu->readMemory8(cpu->currentSegment(), cpu->getSI());
        dest = cpu->readMemory8(cpu->getES(), cpu->getDI());
        cpu->nextSI(1);
        cpu->nextDI(1);
    } else {
        src = cpu->readMemory8(cpu->currentSegment(), cpu->getESI());
        dest = cpu->readMemory8(cpu->getES(), cpu->getEDI());
        cpu->nextESI(1);
        cpu->nextEDI(1);
    }

    cpu->cmpFlags8(src - dest, src, dest);
}

void _CMPSW(VCpu* cpu)
{
    WORD src;
    WORD dest;

    if (cpu->a16()) {
        src = cpu->readMemory16(cpu->currentSegment(), cpu->getSI());
        dest = cpu->readMemory16(cpu->getES(), cpu->getDI());
        cpu->nextSI(2);
        cpu->nextDI(2);
    } else {
        src = cpu->readMemory16(cpu->currentSegment(), cpu->getESI());
        dest = cpu->readMemory16(cpu->getES(), cpu->getEDI());
        cpu->nextESI(2);
        cpu->nextEDI(2);
    }

    cpu->cmpFlags16(src - dest, src, dest);
}

void _SCASB(VCpu* cpu)
{
    BYTE dest;

    if (cpu->a16()) {
        dest = cpu->readMemory8(cpu->getES(), cpu->getDI());
        cpu->nextDI(1);
    } else {
        dest = cpu->readMemory8(cpu->getES(), cpu->getEDI());
        cpu->nextEDI(1);
    }

    cpu->cmpFlags8(cpu->getAL() - dest, dest, cpu->getAL());
}

void _SCASW(VCpu* cpu)
{
    WORD dest;

    if (cpu->a16()) {
        dest = cpu->readMemory16(cpu->getES(), cpu->getDI());
        cpu->nextDI(2);
    } else {
        dest = cpu->readMemory16(cpu->getES(), cpu->getEDI());
        cpu->nextEDI(2);
    }

    cpu->cmpFlags16(cpu->getAX() - dest, dest, cpu->getAX());
}

void _MOVSB(VCpu* cpu)
{
    if (cpu->a16()) {
        BYTE tmpb = cpu->readMemory8(cpu->currentSegment(), cpu->getSI());
        cpu->writeMemory8(cpu->ES, cpu->getDI(), tmpb);
        cpu->nextSI(1);
        cpu->nextDI(1);
    } else {
        BYTE tmpb = cpu->readMemory8(cpu->currentSegment(), cpu->getESI());
        cpu->writeMemory8(cpu->ES, cpu->getEDI(), tmpb);
        cpu->nextESI(1);
        cpu->nextEDI(1);
    }
}

void _MOVSW(VCpu* cpu)
{
    if (cpu->a16()) {
        WORD tmpw = cpu->readMemory16(cpu->currentSegment(), cpu->getSI());
        cpu->writeMemory16(cpu->ES, cpu->getDI(), tmpw);
        cpu->nextSI(2);
        cpu->nextDI(2);
    } else {
        WORD tmpw = cpu->readMemory16(cpu->currentSegment(), cpu->getESI());
        cpu->writeMemory16(cpu->ES, cpu->getEDI(), tmpw);
        cpu->nextESI(2);
        cpu->nextEDI(2);
    }
}

void _MOVSD(VCpu* cpu)
{
    if (cpu->a16()) {
        DWORD tmpw = cpu->readMemory32(cpu->currentSegment(), cpu->regs.W.SI);
        cpu->writeMemory32(cpu->ES, cpu->regs.W.DI, tmpw);
        cpu->nextSI(4);
        cpu->nextDI(4);
    } else {
        DWORD tmpw = cpu->readMemory32(cpu->currentSegment(), cpu->regs.D.ESI);
        cpu->writeMemory32(cpu->ES, cpu->regs.D.EDI, tmpw);
        cpu->nextESI(4);
        cpu->nextEDI(4);
    }
}
