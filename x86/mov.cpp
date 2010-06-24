// x86/mov.cpp
// MOV instructions

#include "vcpu.h"
#include "debug.h"

void _MOV_RM8_imm8(VCpu* cpu)
{
    (void) cpu->resolveModRM8(cpu->fetchOpcodeByte());
    cpu->updateModRM8(cpu->fetchOpcodeByte());
}

void _MOV_RM16_imm16(VCpu* cpu)
{
    (void) cpu->resolveModRM16(cpu->fetchOpcodeByte());
    cpu->updateModRM16(cpu->fetchOpcodeWord());
}

void _MOV_RM16_seg(VCpu* cpu)
{
    BYTE rm = cpu->fetchOpcodeByte();

    VM_ASSERT(vomit_modRMRegisterPart(rm) >= 0 && vomit_modRMRegisterPart(rm) <= 5);

    cpu->writeModRM16(rm, *cpu->tseg[vomit_modRMRegisterPart(rm)]);

#ifdef VOMIT_DEBUG
    if (vomit_modRMRegisterPart(rm) == VCpu::RegisterFS || vomit_modRMRegisterPart(rm) == VCpu::RegisterGS) {
        vlog(VM_CPUMSG, "%04X:%04X: Read from 80386 segment register");
    }
#endif
}

void _MOV_seg_RM16(VCpu* cpu)
{
    BYTE rm = cpu->fetchOpcodeByte();

    VM_ASSERT(vomit_modRMRegisterPart(rm) >= 0 && vomit_modRMRegisterPart(rm) <= 5);

    *cpu->tseg[vomit_modRMRegisterPart(rm)] = cpu->readModRM16(rm);

#ifdef VOMIT_DEBUG
    if (vomit_modRMRegisterPart(rm) == VCpu::RegisterFS || vomit_modRMRegisterPart(rm) == VCpu::RegisterGS) {
        vlog(VM_CPUMSG, "%04X:%04X: Write to 80386 segment register");
    }
#endif
}

void _MOV_seg_RM32(VCpu* cpu)
{
    BYTE rm = cpu->fetchOpcodeByte();

    VM_ASSERT(vomit_modRMRegisterPart(rm) >= 0 && vomit_modRMRegisterPart(rm) <= 5);

    *cpu->tseg[vomit_modRMRegisterPart(rm)] = cpu->readModRM32(rm);

#ifdef VOMIT_DEBUG
    if (vomit_modRMRegisterPart(rm) == VCpu::RegisterFS || vomit_modRMRegisterPart(rm) == VCpu::RegisterGS) {
        vlog(VM_CPUMSG, "%04X:%04X: Write to 80386 segment register");
    }
#endif
}

void _MOV_RM8_reg8(VCpu* cpu)
{
    BYTE rm = cpu->fetchOpcodeByte();
    cpu->writeModRM8(rm, *cpu->treg8[vomit_modRMRegisterPart(rm)]);
}

void _MOV_reg8_RM8(VCpu* cpu)
{
    BYTE rm = cpu->fetchOpcodeByte();
    *cpu->treg8[vomit_modRMRegisterPart(rm)] = cpu->readModRM8(rm);
}

void _MOV_RM16_reg16(VCpu* cpu)
{
    BYTE rm = cpu->fetchOpcodeByte();
    cpu->writeModRM16(rm, *cpu->treg16[vomit_modRMRegisterPart(rm)]);
}

void _MOV_RM32_reg32(VCpu* cpu)
{
    BYTE rm = cpu->fetchOpcodeByte();
    cpu->writeModRM32(rm, *cpu->treg32[vomit_modRMRegisterPart(rm)]);
}

void _MOV_reg16_RM16(VCpu* cpu)
{
    BYTE rm = cpu->fetchOpcodeByte();
    *cpu->treg16[vomit_modRMRegisterPart(rm)] = cpu->readModRM16(rm);
}

void _MOV_reg32_RM32(VCpu* cpu)
{
    BYTE rm = cpu->fetchOpcodeByte();
    *cpu->treg32[vomit_modRMRegisterPart(rm)] = cpu->readModRM32(rm);
}

void _MOV_AL_imm8(VCpu* cpu)
{
    cpu->regs.B.AL = cpu->fetchOpcodeByte();
}

void _MOV_BL_imm8(VCpu* cpu)
{
    cpu->regs.B.BL = cpu->fetchOpcodeByte();
}

void _MOV_CL_imm8(VCpu* cpu)
{
    cpu->regs.B.CL = cpu->fetchOpcodeByte();
}

void _MOV_DL_imm8(VCpu* cpu)
{
    cpu->regs.B.DL = cpu->fetchOpcodeByte();
}

void _MOV_AH_imm8(VCpu* cpu)
{
    cpu->regs.B.AH = cpu->fetchOpcodeByte();
}

void _MOV_BH_imm8(VCpu* cpu)
{
    cpu->regs.B.BH = cpu->fetchOpcodeByte();
}

void _MOV_CH_imm8(VCpu* cpu)
{
    cpu->regs.B.CH = cpu->fetchOpcodeByte();
}

void _MOV_DH_imm8(VCpu* cpu)
{
    cpu->regs.B.DH = cpu->fetchOpcodeByte();
}

void _MOV_EAX_imm32(VCpu* cpu)
{
    cpu->regs.D.EAX = cpu->fetchOpcodeDWord();
}

void _MOV_EBX_imm32(VCpu* cpu)
{
    cpu->regs.D.EBX = cpu->fetchOpcodeDWord();
}

void _MOV_ECX_imm32(VCpu* cpu)
{
    cpu->regs.D.ECX = cpu->fetchOpcodeDWord();
}

void _MOV_EDX_imm32(VCpu* cpu)
{
    cpu->regs.D.EDX = cpu->fetchOpcodeDWord();
}

void _MOV_EBP_imm32(VCpu* cpu)
{
    cpu->regs.D.EBP = cpu->fetchOpcodeDWord();
}

void _MOV_ESP_imm32(VCpu* cpu)
{
    cpu->regs.D.ESP = cpu->fetchOpcodeDWord();
}

void _MOV_ESI_imm32(VCpu* cpu)
{
    cpu->regs.D.ESI = cpu->fetchOpcodeDWord();
}

void _MOV_EDI_imm32(VCpu* cpu)
{
    cpu->regs.D.EDI = cpu->fetchOpcodeDWord();
}

void _MOV_AX_imm16(VCpu* cpu)
{
    cpu->regs.W.AX = cpu->fetchOpcodeWord();
}

void _MOV_BX_imm16(VCpu* cpu)
{
    cpu->regs.W.BX = cpu->fetchOpcodeWord();
}

void _MOV_CX_imm16(VCpu* cpu)
{
    cpu->regs.W.CX = cpu->fetchOpcodeWord();
}

void _MOV_DX_imm16(VCpu* cpu)
{
    cpu->regs.W.DX = cpu->fetchOpcodeWord();
}

void _MOV_BP_imm16(VCpu* cpu)
{
    cpu->regs.W.BP = cpu->fetchOpcodeWord();
}

void _MOV_SP_imm16(VCpu* cpu)
{
    cpu->regs.W.SP = cpu->fetchOpcodeWord();
}

void _MOV_SI_imm16(VCpu* cpu)
{
    cpu->regs.W.SI = cpu->fetchOpcodeWord();
}

void _MOV_DI_imm16(VCpu* cpu)
{
    cpu->regs.W.DI = cpu->fetchOpcodeWord();
}

void _MOV_AL_moff8(VCpu* cpu)
{
    if (cpu->a16())
        cpu->regs.B.AL = cpu->readMemory8(cpu->currentSegment(), cpu->fetchOpcodeWord());
    else
        cpu->regs.B.AL = cpu->readMemory8(cpu->currentSegment(), cpu->fetchOpcodeDWord());
}

void _MOV_AX_moff16(VCpu* cpu)
{
    if (cpu->a16())
        cpu->regs.W.AX = cpu->readMemory16(cpu->currentSegment(), cpu->fetchOpcodeWord());
    else
        cpu->regs.W.AX = cpu->readMemory16(cpu->currentSegment(), cpu->fetchOpcodeDWord());
}

void _MOV_moff8_AL(VCpu* cpu)
{
    if (cpu->a16())
        cpu->writeMemory8(cpu->currentSegment(), cpu->fetchOpcodeWord(), cpu->getAL());
    else
        cpu->writeMemory8(cpu->currentSegment(), cpu->fetchOpcodeDWord(), cpu->getAL());
}

void _MOV_moff16_AX(VCpu* cpu)
{
    if (cpu->a16())
        cpu->writeMemory16(cpu->currentSegment(), cpu->fetchOpcodeWord(), cpu->getAX());
    else
        cpu->writeMemory16(cpu->currentSegment(), cpu->fetchOpcodeDWord(), cpu->getAX());
}

void _MOV_moff32_EAX(VCpu* cpu)
{
    if (cpu->a16())
        cpu->writeMemory32(cpu->currentSegment(), cpu->fetchOpcodeWord(), cpu->getEAX());
    else
        cpu->writeMemory32(cpu->currentSegment(), cpu->fetchOpcodeDWord(), cpu->getEAX());
}

void _MOVZX_reg16_RM8(VCpu* cpu)
{
    BYTE rm = cpu->fetchOpcodeByte();
    *cpu->treg16[vomit_modRMRegisterPart(rm)] = cpu->readModRM8(rm);
}

void _MOVZX_reg32_RM8(VCpu* cpu)
{
    BYTE rm = cpu->fetchOpcodeByte();
    *cpu->treg32[vomit_modRMRegisterPart(rm)] = cpu->readModRM8(rm);
}

void _MOVZX_reg32_RM16(VCpu* cpu)
{
    BYTE rm = cpu->fetchOpcodeByte();
    *cpu->treg32[vomit_modRMRegisterPart(rm)] = cpu->readModRM16(rm);
}
