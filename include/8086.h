#ifndef __8086_H__
#define __8086_H__

	#define CPU_DEAD			0
	#define CPU_ALIVE			1
	#define CPU_HALTED			2

	#define CPU_STATIC_FLAGS	0x7002

	#ifdef VM_DEBUG
		extern byte opuse[256];
	#endif

	#define rmreg(b) (b>>3&7)	/* Extracts RegID from RM byte. */

	extern byte CPU_PFQ_SIZE;

	extern byte cpu_state;
	extern byte cpu_pfq_current;
	extern byte cpu_type;
	extern dword cpu_ips;

	extern byte cpu_opcode;
	extern byte cpu_rmbyte;

	extern byte *mem_space;
	extern word mem_avail;

	extern word CS, DS, ES, SS, SegmentPrefix, *CurrentSegment;
	extern word AX, BX, CX, DX, BasePointer, StackPointer, SI, DI, IP;
	extern byte CF, DF, TF, PF, AF, ZF, SF, IF, OF;

	extern byte *treg8[];
	extern word *treg16[];
	extern word *tseg[];

	void cpu_init();
	void cpu_genmap();
    void cpu_kill();
	void cpu_main();
	void cpu_flags_init();
	byte cpu_pfq_getbyte();
	word cpu_pfq_getword();
	void cpu_pfq_flush();
	void cpu_modrm_init();
	void cpu_jump(word,word);
	byte cpu_parity(byte);
	void cpu_setflags(word);
	void cpu_updflags(word, byte);
	word cpu_getflags();
	void cpu_addint(byte,word,word);
	void cpu_addinstruction(int,int,void (*) (),char*,byte);
	void *cpu_rmptr(byte, byte);
	void cpu_outbyte(word, byte);
	void cpu_outword(word, word);
	void cpu_out(word, word, byte);
	byte cpu_inbyte(word);
	word cpu_inword(word);
	word cpu_in(word, byte);
	byte cpu_evaluate(byte);

	void cpu_setZF(dword);
	void cpu_setSF(dword, byte);
	void cpu_setPF(dword);
	void cpu_setCF(dword, byte);
	void cpu_setAddOF(dword, word, word, byte);
	void cpu_setSubOF(dword, word, word, byte);
	void cpu_setAF(dword, word, word);

	inline void cpu_mathflags(dword, word, word, byte);
	inline void cpu_cmpflags(dword, word, word, byte);

	dword cpu_add(word, word, byte);
	dword cpu_sub(word, word, byte);
	dword cpu_mul(word, word, byte);
	dword cpu_div(word, word, byte);
	dword cpu_imul(word, word, byte);

	dword cpu_or(word, word, byte);
	dword cpu_xor(word, word, byte);
	dword cpu_and(word, word, byte);
	dword cpu_shl(word, byte, byte);
	dword cpu_shr(word, byte, byte);
	dword cpu_sar(word, byte, byte);
	dword cpu_rcl(word, byte, byte);
	dword cpu_rcr(word, byte, byte);
	dword cpu_rol(word, byte, byte);
	dword cpu_ror(word, byte, byte);

	extern byte *cpu_pfq;

	void mem_init();
    void mem_kill();
    void mem_push(word);
    word mem_pop();
    inline byte mem_getbyte(word, word);
    inline word mem_getword(word, word);
    inline void mem_setbyte(word, word, byte);
    inline void mem_setword(word, word, word);

	void int_init();
	void int_kill();
	void int_call(byte);
	void int_return();

	word signext(byte);
	dword signext32(word);

    #define REG_AL  0
    #define REG_CL  1
    #define REG_DL  2
    #define REG_BL  3
    #define REG_AH  4
    #define REG_CH  5
    #define REG_DH  6
    #define REG_BH  7

    #define REG_AX  0
    #define REG_CX  1
    #define REG_DX  2
    #define REG_BX  3
    #define REG_StackPointer  4
    #define REG_BasePointer  5
    #define REG_SI  6
    #define REG_DI  7

    #define REG_ES  0
    #define REG_CS  1
    #define REG_SS  2
    #define REG_DS  3

	void	_UNSUPP				();

	void	_WAIT				();
	void	_NOP				();
	void	_HLT				();
	void	_INT_imm8			();
	void	_INT3				();
	void	_INTO				();
	void	_IRET				();

    void	_AAA				();
	void	_AAM				();
	void	_AAD				();
	void	_AAS				();

	void	_DAA				();
	void	_DAS				();

	void	_STC				();
    void	_STD				();
    void	_STI				();

    void	_CLC				();
    void	_CLD				();
    void	_CLI				();

    void	_CMC				();

	void	_CBW				();
	void	_CWD				();

    void	_XLAT				();

    void	_CS					();
    void	_DS					();
    void	_ES					();
    void	_SS					();

	void	_SALC				();

	void	_JMP_imm16			();
	void	_JMP_imm16_imm16	();
	void	_JMP_short_imm8		();
	void	_Jcc_imm8			();
	void	_JCXZ_imm8			();

	void	_CALL_imm16			();
	void	_RET				();
	void	_RET_imm16			();
	void	_RETF				();
	void	_RETF_imm16			();

	void	_LOOP_imm8			();
	void	_LOOPE_imm8			();
	void	_LOOPNE_imm8		();

	void	_REP				();
	void	_REPNE				();

	void	_XCHG_AX_reg16		();
	void 	_XCHG_reg8_RM8		();
	void	_XCHG_reg16_RM16	();

	void	_CMPSB				();
	void	_CMPSW				();
	void	_LODSB				();
	void	_LODSW				();
	void	_SCASB				();
	void	_SCASW				();
	void	_STOSB				();
	void	_STOSW				();
	void	_MOVSB				();
	void	_MOVSW				();

	void	_LEA_reg16_mem16	();

	void	_LDS_reg16_mem16	();
	void	_LES_reg16_mem16	();

	void	_MOV_reg8_imm8		();
	void	_MOV_reg16_imm16	();
	void	_MOV_seg_RM16		();
	void	_MOV_RM16_seg		();
	void	_MOV_AL_moff8		();
	void	_MOV_AX_moff16		();
	void	_MOV_moff8_AL		();
	void	_MOV_moff16_AX		();
	void	_MOV_reg8_RM8		();
	void	_MOV_reg16_RM16		();
	void	_MOV_RM8_reg8		();
	void	_MOV_RM16_reg16		();
	void	_MOV_RM8_imm8		();
	void	_MOV_RM16_imm16		();

	void	_XOR_RM8_reg8		();
	void	_XOR_RM16_reg16		();
	void	_XOR_reg8_RM8		();
	void	_XOR_reg16_RM16		();
	void	_XOR_RM8_imm8		();
	void	_XOR_RM16_imm16		();
	void	_XOR_RM16_imm8		();
	void	_XOR_AX_imm16		();
	void	_XOR_AL_imm8		();

	void	_OR_RM8_reg8		();
	void	_OR_RM16_reg16		();
	void	_OR_reg8_RM8		();
	void	_OR_reg16_RM16		();
	void	_OR_RM8_imm8		();
	void	_OR_RM16_imm16		();
	void	_OR_RM16_imm8		();
	void	_OR_AX_imm16		();
	void	_OR_AL_imm8			();

    void    _AND_RM8_reg8       ();
    void    _AND_RM16_reg16     ();
    void    _AND_reg8_RM8       ();
    void    _AND_reg16_RM16     ();
    void    _AND_RM8_imm8       ();
    void    _AND_RM16_imm16     ();
    void    _AND_RM16_imm8      ();
    void    _AND_AX_imm16       ();
    void    _AND_AL_imm8        ();

	void	_TEST_RM8_reg8		();
	void	_TEST_RM16_reg16	();
	void	_TEST_AX_imm16		();
	void	_TEST_AL_imm8		();

	void	_PUSH_reg16			();
	void	_PUSH_CS			();
	void	_PUSH_DS			();
	void	_PUSH_ES			();
	void	_PUSH_SS			();
	void	_PUSHF				();

	void	_POP_reg16			();
	void	_POP_CS				();
	void	_POP_DS				();
	void	_POP_ES				();
	void	_POP_SS				();
	void	_POPF				();

    void	_LAHF				();
    void	_SAHF				();

	void 	_OUT_imm8_AL		();
	void	_OUT_imm8_AX		();
	void 	_OUT_DX_AL			();
	void 	_OUT_DX_AX			();

	void	_IN_AL_imm8			();
	void	_IN_AX_imm8			();
	void	_IN_AL_DX			();
	void	_IN_AX_DX			();

	void	_ADD_RM8_reg8		();
	void	_ADD_RM16_reg16		();
	void	_ADD_reg8_RM8		();
	void	_ADD_reg16_RM16		();
	void	_ADD_AL_imm8		();
	void	_ADD_AX_imm16		();
	void	_ADD_RM8_imm8		();
	void	_ADD_RM16_imm16		();
	void	_ADD_RM16_imm8		();

    void    _SUB_RM8_reg8       ();
    void    _SUB_RM16_reg16     ();
    void    _SUB_reg8_RM8       ();
    void    _SUB_reg16_RM16     ();
    void    _SUB_AL_imm8        ();
    void    _SUB_AX_imm16       ();
    void    _SUB_RM8_imm8       ();
    void    _SUB_RM16_imm16     ();
	void	_SUB_RM16_imm8		();

    void    _ADC_RM8_reg8       ();
    void    _ADC_RM16_reg16     ();
    void    _ADC_reg8_RM8       ();
    void    _ADC_reg16_RM16     ();
    void    _ADC_AL_imm8        ();
    void    _ADC_AX_imm16       ();
    void    _ADC_RM8_imm8       ();
    void    _ADC_RM16_imm16     ();
    void    _ADC_RM16_imm8      ();

    void    _SBB_RM8_reg8       ();
    void    _SBB_RM16_reg16     ();
    void    _SBB_reg8_RM8       ();
    void    _SBB_reg16_RM16     ();
    void    _SBB_AL_imm8        ();
    void    _SBB_AX_imm16       ();
    void    _SBB_RM8_imm8       ();
    void    _SBB_RM16_imm16     ();
    void    _SBB_RM16_imm8      ();

    void    _CMP_RM8_reg8       ();
    void    _CMP_RM16_reg16     ();
    void    _CMP_reg8_RM8       ();
    void    _CMP_reg16_RM16     ();
    void    _CMP_AL_imm8        ();
    void    _CMP_AX_imm16       ();
    void    _CMP_RM8_imm8       ();
    void    _CMP_RM16_imm16     ();
	void	_CMP_RM16_imm8		();

	void	_MUL_RM8			();
	void	_MUL_RM16			();
	void	_DIV_RM8			();
	void	_DIV_RM16			();
	void	_IMUL_RM8			();
	void	_IMUL_RM16			();
	void	_IDIV_RM8			();
	void	_IDIV_RM16			();

	void	_TEST_RM8_imm8		();
	void	_TEST_RM16_imm16	();
	void	_NOT_RM8			();
	void	_NOT_RM16			();
	void	_NEG_RM8			();
	void	_NEG_RM16			();

	void	_INC_RM16			();
	void	_INC_reg16			();
	void	_DEC_RM16			();
	void	_DEC_reg16			();

	void	_CALL_RM16			();
	void	_CALL_FAR_mem16		();
	void	_CALL_imm16_imm16	();

	void	_JMP_RM16			();
	void	_JMP_FAR_mem16		();

	void	_PUSH_RM16			();
	void	_POP_RM16			();

	void	_wrap_0x80			();
	void	_wrap_0x81			();
	void	_wrap_0x83			();
	void	_wrap_0x8F			();
	void	_wrap_0xC0			();
	void	_wrap_0xC1			();
	void	_wrap_0xD0			();
	void	_wrap_0xD1			();
	void	_wrap_0xD2			();
	void	_wrap_0xD3			();
	void	_wrap_0xF6			();
	void	_wrap_0xF7			();
	void	_wrap_0xFE			();
	void	_wrap_0xFF			();

#endif

