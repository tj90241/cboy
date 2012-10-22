/*************************************************************************
 *   Cboy, a Game Boy emulator
 *   Copyright (C) 2012 jkbenaim
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "cpu.h"
#include "memory.h"
#include "assert.h"
#include "video.h"

struct state_s state;
int branched= 0;

// int stop = 0;

// The op length and timing tables are taken from Shay Green's
// instruction timing tests.

// Instruction lengths.
int op_lengths[0x100] = {
	1,3,1,1,1,1,2,1,3,1,1,1,1,1,2,1,
	0,3,1,1,1,1,2,1,2,1,1,1,1,1,2,1,
	2,3,1,1,1,1,2,1,2,1,1,1,1,1,2,1,
	2,3,1,1,1,1,2,1,2,1,1,1,1,1,2,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,3,3,3,1,2,1,1,1,3,0,3,3,2,1,
	1,1,3,0,3,1,2,1,1,1,3,0,3,0,2,1,
	2,1,1,0,0,1,2,1,2,1,3,0,0,0,2,1,
	2,1,1,1,0,1,2,1,2,1,3,1,0,0,2,1
};

// Instruction times.
int op_times[0x100] = {
	1,3,2,2,1,1,2,1,5,2,2,2,1,1,2,1,
	0,3,2,2,1,1,2,1,3,2,2,2,1,1,2,1,
	2,3,2,2,1,1,2,1,2,2,2,2,1,1,2,1,
	2,3,2,2,3,3,3,1,2,2,2,2,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	2,2,2,2,2,2,1,2,1,1,1,1,1,1,2,1,// HALT changed to 1
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	2,3,3,4,3,4,2,4,2,4,3,0,3,6,2,4,
	2,3,3,0,3,4,2,4,2,4,3,0,3,0,2,4,
	3,3,2,0,0,4,2,4,4,1,4,0,0,0,2,4,
	3,3,2,1,0,4,2,4,3,2,4,1,0,0,2,4
};

// Instruction times when conditionals are taken.
int op_times_taken[0x100] = {
	1,3,2,2,1,1,2,1,5,2,2,2,1,1,2,1,
	0,3,2,2,1,1,2,1,3,2,2,2,1,1,2,1,
	3,3,2,2,1,1,2,1,3,2,2,2,1,1,2,1,
	3,3,2,2,3,3,3,1,3,2,2,2,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	2,2,2,2,2,2,0,2,1,1,1,1,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	5,3,4,4,6,4,2,4,5,4,4,0,6,6,2,4,
	5,3,4,0,6,4,2,4,5,4,4,0,6,0,2,4,
	3,3,2,0,0,4,2,4,4,1,4,0,0,0,2,4,
	3,3,2,1,0,4,2,4,3,2,4,1,0,0,2,4
};

// Instruction times for CB-prefixed instructions.
int op_cb_times[0x100] = {
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,3,2,2,2,2,2,2,2,3,2,
	2,2,2,2,2,2,3,2,2,2,2,2,2,2,3,2,
	2,2,2,2,2,2,3,2,2,2,2,2,2,2,3,2,
	2,2,2,2,2,2,3,2,2,2,2,2,2,2,3,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2
};

#define LD(X,Y) void LD_ ## X ## _ ## Y( void ) { (state.X) = (state.Y); state.pc++; }
LD(b,b);LD(b,c);LD(b,d);LD(b,e);
LD(b,h);LD(b,l);LD(b,a);
LD(c,b);LD(c,c);LD(c,d);LD(c,e);
LD(c,h);LD(c,l);LD(c,a);
LD(d,b);LD(d,c);LD(d,d);LD(d,e);
LD(d,h);LD(d,l);LD(d,a);
LD(e,b);LD(e,c);LD(e,d);LD(e,e);
LD(e,h);LD(e,l);LD(e,a);
LD(h,b);LD(h,c);LD(h,d);LD(h,e);
LD(h,h);LD(h,l);LD(h,a);
LD(l,b);LD(l,c);LD(l,d);LD(l,e);
LD(l,h);LD(l,l);LD(l,a);
LD(a,b);LD(a,c);LD(a,d);LD(a,e);
LD(a,h);LD(a,l);LD(a,a);

void (*ops[0x100])(void) = {
  /* 00 */ NOP, 	LD_BC_WORD,	LD_BC_A,	INC_BC,
  /* 04 */ INC_R, 	DEC_R, 		LD_B_BYTE,	RLCA, 
  /* 08 */ LD_WORD_SP, 	ADD_HL_BC,	LD_A_BC,	DEC_BC,
  /* 0C */ INC_R, 	DEC_R,		LD_C_BYTE, 	RRCA,
  /* 10 */ STOP, 	LD_DE_WORD,	LD_DE_A,	INC_DE,
  /* 14 */ INC_R, 	DEC_R,		LD_D_BYTE,	RLA,
  /* 18 */ JR_INDEX, 	ADD_HL_DE,	LD_A_DE, 	DEC_DE,
  /* 1C */ INC_R, 	DEC_R,		LD_E_BYTE, 	RRA,
  /* 20 */ JR_NZ_INDEX, LD_HL_WORD,	LDI_HL_A, 	INC_HL,
  /* 24 */ INC_R, 	DEC_R,		LD_H_BYTE,	DAA,
  /* 28 */ JR_Z_INDEX, 	ADD_HL_HL,	LDI_A_HL,	DEC_HL,
  /* 2C */ INC_R, 	DEC_R,		LD_L_BYTE, 	CPL,
  /* 30 */ JR_NC_INDEX,	LD_SP_WORD,	LDD_HL_A, 	INC_SP,
  /* 34 */ INC_AT_HL,	DEC_AT_HL,	LD_HL_BYTE,	SCF,
  /* 38 */ JR_C_INDEX, 	ADD_HL_SP,	LDD_A_HL,	DEC_SP,
  /* 3C */ INC_R,	DEC_R,		LD_A_BYTE, 	CCF,
  /* 40 */ LD_b_b,      LD_b_c,         LD_b_d,         LD_b_e,
  /* 44 */ LD_b_h, 	LD_b_l,		LD_B_HL,	LD_b_a,
  /* 48 */ LD_c_b, 	LD_c_c,		LD_c_d, 	LD_c_e,
  /* 4C */ LD_c_h, 	LD_c_l,		LD_C_HL,	LD_c_a,
  /* 50 */ LD_d_b, 	LD_d_c,		LD_d_d, 	LD_d_e,
  /* 54 */ LD_d_h, 	LD_d_l,		LD_D_HL,	LD_d_a,
  /* 58 */ LD_e_b, 	LD_e_c,		LD_e_d, 	LD_e_e,
  /* 5C */ LD_e_h, 	LD_e_l,		LD_E_HL,	LD_e_a,
  /* 60 */ LD_h_b, 	LD_h_c,		LD_h_d,		LD_h_e,
  /* 64 */ LD_h_h, 	LD_h_l,		LD_H_HL,	LD_h_a,
  /* 68 */ LD_l_b, 	LD_l_c,		LD_l_d, 	LD_l_e,
  /* 6C */ LD_l_h, 	LD_l_l,		LD_L_HL,	LD_l_a,
  /* 70 */ LD_HL_R, 	LD_HL_R,	LD_HL_R, 	LD_HL_R,
  /* 74 */ LD_HL_R, 	LD_HL_R,	HALT, 		LD_HL_R,
  /* 78 */ LD_a_b, 	LD_a_c,		LD_a_d,		LD_a_e,
  /* 7C */ LD_a_h, 	LD_a_l,		LD_A_HL,	LD_a_a,
  /* 80 */ ADD_A_R, 	ADD_A_R,	ADD_A_R, 	ADD_A_R,
  /* 84 */ ADD_A_R, 	ADD_A_R,	ADD_A_HL,	ADD_A_R,
  /* 88 */ ADC_A_R, 	ADC_A_R,	ADC_A_R,	ADC_A_R,
  /* 8C */ ADC_A_R, 	ADC_A_R,	ADC_A_HL,	ADC_A_R,
  /* 90 */ SUB_R, 	SUB_R,		SUB_R, 		SUB_R,
  /* 94 */ SUB_R, 	SUB_R,		SUB_HL,		SUB_R,
  /* 98 */ SBC_A_R, 	SBC_A_R,	SBC_A_R, 	SBC_A_R,
  /* 9C */ SBC_A_R, 	SBC_A_R,	SBC_A_HL,	SBC_A_R,
  /* A0 */ AND_R, 	AND_R,		AND_R, 		AND_R,
  /* A4 */ AND_R, 	AND_R,		AND_HL,		AND_R,
  /* A8 */ XOR_R, 	XOR_R,		XOR_R, 		XOR_R,
  /* AC */ XOR_R, 	XOR_R,		XOR_HL,		XOR_R,
  /* B0 */ OR_R, 	OR_R,		OR_R, 		OR_R,
  /* B4 */ OR_R, 	OR_R,		OR_HL, 		OR_R,
  /* B8 */ CP_R, 	CP_R,		CP_R, 		CP_R,
  /* BC */ CP_R, 	CP_R,		CP_HL, 		CP_R,
  /* C0 */ RET_CC,	POP_BC,		JP_NZ_ADDR,	JP_ADDR,
  /* C4 */ CALL_NZ, 	PUSH_BC,	ADD_A_BYTE,	RST_0,
  /* C8 */ RET_CC, 	RET,		JP_Z_ADDR,	CB_PREFIX,
  /* CC */ CALL_Z, 	CALL,		ADC_A_BYTE,	RST_8,
  /* D0 */ RET_CC, 	POP_DE,		JP_NC_ADDR,	LOCKUP,
  /* D4 */ CALL_NC, 	PUSH_DE,	SUB_A_BYTE, 	RST_10,
  /* D8 */ RET_CC, 	RETI,		JP_C_ADDR,	LOCKUP,
  /* DC */ CALL_C, 	LOCKUP,		SBC_A_BYTE,	RST_18,
  /* E0 */ LD_FF_BYTE_A,POP_HL,		LD_FF_C_A, 	LOCKUP,
  /* E4 */ LOCKUP, 	PUSH_HL,	AND_BYTE,	RST_20,
  /* E8 */ ADD_SP_OFFSET,JP_HL,		LD_WORD_A, 	LOCKUP,
  /* EC */ LOCKUP, 	LOCKUP,		XOR_BYTE,	RST_28,
  /* F0 */ LD_A_FF_BYTE,POP_AF,		LD_A_FF_C,	DI,
  /* F4 */ LOCKUP, 	PUSH_AF,	OR_BYTE,	RST_30,
  /* F8 */ LDHL_SP_OFFSET, 	LD_SP_HL,LD_A_WORD,	EI,
  /* FC */ LOCKUP, 	LOCKUP,		CP_BYTE, 	RST_38
};

void (*cb_ops[0x100])(void) = {
  /* 00 */ CB_RLC_R, 	CB_RLC_R,	CB_RLC_R,	CB_RLC_R,
  /* 04 */ CB_RLC_R, 	CB_RLC_R, 	CB_RLC_HL,	CB_RLC_R, 
  /* 08 */ CB_RRC_R, 	CB_RRC_R, 	CB_RRC_R, 	CB_RRC_R,
  /* 0C */ CB_RRC_R, 	CB_RRC_R,	CB_RRC_HL, 	CB_RRC_R,
  /* 10 */ CB_RL_R, 	CB_RL_R,	CB_RL_R, 	CB_RL_R,
  /* 14 */ CB_RL_R, 	CB_RL_R,	CB_RL_HL, 	CB_RL_R,
  /* 18 */ CB_RR_R, 	CB_RR_R,	CB_RR_R, 	CB_RR_R,
  /* 1C */ CB_RR_R, 	CB_RR_R,	CB_RR_HL, 	CB_RR_R,
  /* 20 */ CB_SLA_R, 	CB_SLA_R,	CB_SLA_R, 	CB_SLA_R,
  /* 24 */ CB_SLA_R, 	CB_SLA_R,	CB_SLA_HL, 	CB_SLA_R,
  /* 28 */ CB_SRA_R, 	CB_SRA_R,	CB_SRA_R, 	CB_SRA_R,
  /* 2C */ CB_SRA_R, 	CB_SRA_R,	CB_SRA_HL, 	CB_SRA_R,
  /* 30 */ CB_SWAP_R, 	CB_SWAP_R,	CB_SWAP_R, 	CB_SWAP_R,
  /* 34 */ CB_SWAP_R, 	CB_SWAP_R,	CB_SWAP_HL, 	CB_SWAP_R,
  /* 38 */ CB_SRL_R, 	CB_SRL_R,	CB_SRL_R, 	CB_SRL_R,
  /* 3C */ CB_SRL_R, 	CB_SRL_R,	CB_SRL_HL, 	CB_SRL_R,
  /* 40 */ CB_BIT_R, 	CB_BIT_R,	CB_BIT_R, 	CB_BIT_R,
  /* 44 */ CB_BIT_R, 	CB_BIT_R,	CB_BIT_HL, 	CB_BIT_R,
  /* 48 */ CB_BIT_R, 	CB_BIT_R,	CB_BIT_R, 	CB_BIT_R,
  /* 4C */ CB_BIT_R, 	CB_BIT_R,	CB_BIT_HL, 	CB_BIT_R,
  /* 50 */ CB_BIT_R, 	CB_BIT_R,	CB_BIT_R, 	CB_BIT_R,
  /* 54 */ CB_BIT_R, 	CB_BIT_R,	CB_BIT_HL, 	CB_BIT_R,
  /* 58 */ CB_BIT_R, 	CB_BIT_R,	CB_BIT_R, 	CB_BIT_R,
  /* 5C */ CB_BIT_R, 	CB_BIT_R,	CB_BIT_HL, 	CB_BIT_R,
  /* 60 */ CB_BIT_R, 	CB_BIT_R,	CB_BIT_R, 	CB_BIT_R,
  /* 64 */ CB_BIT_R, 	CB_BIT_R,	CB_BIT_HL, 	CB_BIT_R,
  /* 68 */ CB_BIT_R, 	CB_BIT_R,	CB_BIT_R, 	CB_BIT_R,
  /* 6C */ CB_BIT_R, 	CB_BIT_R,	CB_BIT_HL, 	CB_BIT_R,
  /* 70 */ CB_BIT_R, 	CB_BIT_R,	CB_BIT_R, 	CB_BIT_R,
  /* 74 */ CB_BIT_R, 	CB_BIT_R,	CB_BIT_HL, 	CB_BIT_R,
  /* 78 */ CB_BIT_R, 	CB_BIT_R,	CB_BIT_R, 	CB_BIT_R,
  /* 7C */ CB_BIT_R, 	CB_BIT_R,	CB_BIT_HL, 	CB_BIT_R,
  /* 80 */ CB_RES_B_R, 	CB_RES_B_R,	CB_RES_B_R, 	CB_RES_B_R,
  /* 84 */ CB_RES_B_R, 	CB_RES_B_R,	CB_RES_B_HL, 	CB_RES_B_R,
  /* 88 */ CB_RES_B_R, 	CB_RES_B_R,	CB_RES_B_R, 	CB_RES_B_R,
  /* 8C */ CB_RES_B_R, 	CB_RES_B_R,	CB_RES_B_HL, 	CB_RES_B_R,
  /* 90 */ CB_RES_B_R, 	CB_RES_B_R,	CB_RES_B_R, 	CB_RES_B_R,
  /* 94 */ CB_RES_B_R, 	CB_RES_B_R,	CB_RES_B_HL, 	CB_RES_B_R,
  /* 98 */ CB_RES_B_R, 	CB_RES_B_R,	CB_RES_B_R, 	CB_RES_B_R,
  /* 9C */ CB_RES_B_R, 	CB_RES_B_R,	CB_RES_B_HL, 	CB_RES_B_R,
  /* A0 */ CB_RES_B_R, 	CB_RES_B_R,	CB_RES_B_R, 	CB_RES_B_R,
  /* A4 */ CB_RES_B_R, 	CB_RES_B_R,	CB_RES_B_HL, 	CB_RES_B_R,
  /* A8 */ CB_RES_B_R, 	CB_RES_B_R,	CB_RES_B_R, 	CB_RES_B_R,
  /* AC */ CB_RES_B_R, 	CB_RES_B_R,	CB_RES_B_HL, 	CB_RES_B_R,
  /* B0 */ CB_RES_B_R, 	CB_RES_B_R,	CB_RES_B_R, 	CB_RES_B_R,
  /* B4 */ CB_RES_B_R, 	CB_RES_B_R,	CB_RES_B_HL, 	CB_RES_B_R,
  /* B8 */ CB_RES_B_R, 	CB_RES_B_R,	CB_RES_B_R, 	CB_RES_B_R,
  /* BC */ CB_RES_B_R, 	CB_RES_B_R,	CB_RES_B_HL,	CB_RES_B_R,
  /* C0 */ CB_SET_B_R, 	CB_SET_B_R,	CB_SET_B_R, 	CB_SET_B_R,
  /* C4 */ CB_SET_B_R, 	CB_SET_B_R,	CB_SET_B_HL, 	CB_SET_B_R,
  /* C8 */ CB_SET_B_R, 	CB_SET_B_R,	CB_SET_B_R, 	CB_SET_B_R,
  /* CC */ CB_SET_B_R, 	CB_SET_B_R,	CB_SET_B_HL, 	CB_SET_B_R,
  /* D0 */ CB_SET_B_R, 	CB_SET_B_R,	CB_SET_B_R, 	CB_SET_B_R,
  /* D4 */ CB_SET_B_R, 	CB_SET_B_R,	CB_SET_B_HL, 	CB_SET_B_R,
  /* D8 */ CB_SET_B_R, 	CB_SET_B_R,	CB_SET_B_R, 	CB_SET_B_R,
  /* DC */ CB_SET_B_R, 	CB_SET_B_R,	CB_SET_B_HL, 	CB_SET_B_R,
  /* E0 */ CB_SET_B_R, 	CB_SET_B_R,	CB_SET_B_R, 	CB_SET_B_R,
  /* E4 */ CB_SET_B_R, 	CB_SET_B_R,	CB_SET_B_HL, 	CB_SET_B_R,
  /* E8 */ CB_SET_B_R, 	CB_SET_B_R,	CB_SET_B_R, 	CB_SET_B_R,
  /* EC */ CB_SET_B_R, 	CB_SET_B_R,	CB_SET_B_HL, 	CB_SET_B_R,
  /* F0 */ CB_SET_B_R, 	CB_SET_B_R,	CB_SET_B_R, 	CB_SET_B_R,
  /* F4 */ CB_SET_B_R, 	CB_SET_B_R,	CB_SET_B_HL, 	CB_SET_B_R,
  /* F8 */ CB_SET_B_R, 	CB_SET_B_R,	CB_SET_B_R, 	CB_SET_B_R,
  /* FC */ CB_SET_B_R, 	CB_SET_B_R,	CB_SET_B_HL,	CB_SET_B_R
};

void UNDEF( void )
{
  address = state.pc;
  READ_BYTE();
  printf("Undefined opcode %02X at address %04X\n", memByte, state.pc);
  exit(1);
}

void UNDEF_CB( void )
{
  address = state.pc + 1;
  READ_BYTE();
  printf("Undefined CB-prefixed opcode %02X at address %04X\n", memByte, state.pc);
  exit(1);
}

void LOCKUP( void )
{
  address = state.pc;
  READ_BYTE();
  printf("LOCKUP opcode %02X at address %04X\n", memByte, state.pc);
  exit(1);
}

void NOP( void )
{
  // opcode 00
  // does nothing
  state.pc++;
  return;
}

void LD_BC_WORD( void )
{
  // opcode 01
//   address = state.pc + 1;
//   READ_BYTE();
//   state.c = memByte;
//   
//   address = state.pc + 2;
//   READ_BYTE();
//   state.b = memByte;

  address = state.pc + 1;
  READ_WORD();
  state.bc = memWord;
  
  state.pc += 3;
}

void LD_BC_A( void )
{
  // opcode 02
  address = STATE_BC;
  memByte = state.a;
  WRITE_BYTE();
  // no flags affected
  state.pc++;
}

void INC_BC( void )
{
  // opcode 03
  state.bc++;
  state.pc++;
  // no flags affected
}

void INC_R( void )
{
  // opcodes 04, 0C, 14, 1C, 24, 2C, 3C
  // INC r
  // 00rrr100
  
  address = state.pc;
  READ_BYTE();
  int regNumber = ((int)(memByte) & 0x38) >> 3;
  u8 *reg = cpu_getReg(regNumber);
  
  int old_r = (*reg);
  (*reg)++;
  int new_r = (*reg);
  
  // flag Z
  if((*reg) == 0)
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  if( (new_r & 0xF0) != (old_r & 0xF0) )
    SET_H();
  else
    RESET_H();
  
  // flag C is not affected
  
  state.pc++;
}

void DEC_R( void )
{
  // opcodes 05, 0D, 15, 1D, 25, 2D, 3D
  // DEC r
  // 00rrr101
  
  address = state.pc;
  READ_BYTE();
  int regNumber = ((int)(memByte) & 0x38) >> 3;
  u8 *reg = cpu_getReg(regNumber);
  
  int old_r = (*reg);
  (*reg)--;
  int new_r = (*reg);
  
  // flag Z
  if(new_r == 0)
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  SET_N();
  
  // flag H
  if( (new_r & 0xF0) != (old_r & 0xF0) )
    SET_H();
  else
    RESET_H();
  
  // flag C is not affected
  
  state.pc++;
}

void LD_B_BYTE( void )
{
  // opcode 06
  address = state.pc + 1;
  READ_BYTE();
  state.b = memByte;
  state.pc += 2;
}

void RLCA( void )
{
  // opcode 07
  int temp = state.a;
  
  state.a = state.a << 1;
  
  // flag C and shift in carry bit
  if( temp > 0x7F )
  {
    state.a |= 1;
    SET_C();
  }
  else
    RESET_C();
  
  // flag Z
  RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  RESET_H();
  
  state.pc++;
}

void LD_WORD_SP( void )
{
  // opcode 08
  address = state.pc + 1;
  READ_WORD();
  address = memWord;
  memWord = state.sp;
  WRITE_WORD();
  
  // no flags affected?
  
  state.pc += 3;
} 

void ADD_HL_BC( void )
{
  // opcode 09
  int result = (int)(STATE_HL) + (int)(STATE_BC);
  
  // flag Z in not affected
  
  // flag N
  RESET_N();
  
  // flag H
  int c = (int)state.l + (int)state.c;
  if( c > 0xFF )
    c = 1;
  else
    c = 0;
  
  if( ((state.h & 0x0f) + (state.b & 0x0f) + c) & 0x10 )
    SET_H();
  else
    RESET_H();
  
  // flag C
  if( result > 0xFFFF )
    SET_C();
  else
    RESET_C();
  
  state.hl = result;
  
  state.pc++;
}

void LD_A_BC( void )
{
  // opcode 0A
  address = STATE_BC;
  READ_BYTE();
  state.a = memByte;
  state.pc++;
}

void DEC_BC( void )
{
  // opcode 0B
  state.bc--;
  state.pc++;
}

void LD_C_BYTE( void )
{
  // opcode 0E
  address = state.pc + 1;
  READ_BYTE();
  state.c = memByte;
  state.pc += 2;
}

void RRCA( void )
{
  // opcode 0F
  
  if( state.a & 0x01 )
    SET_C();
  else
    RESET_C();
  
  state.a = state.a >> 1;
  
  if( ISSET_C() )
    state.a |= 0x80;
  
  // flag Z
  RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  RESET_H();
  
  state.pc++;
}

void STOP( void )
{
  // opcode 10
  // currently implemented the same as HALT
  printf("STOP\n");
//   state.halt = 1;
  state.pc++;
}

void LD_DE_WORD( void )
{
  // opcode 11
  address = state.pc + 1;
  READ_WORD();
  state.de = memWord;
  
  state.pc+=3;
}

void LD_DE_A( void )
{
  // opcode 12
  memByte = state.a;
  address = STATE_DE;
  WRITE_BYTE();
  // no flags modified
  state.pc++;
}

void INC_DE( void )
{
  // opcode 13
  // no flags affected
  state.de++;
  state.pc++;
}

void LD_D_BYTE( void )
{
  // opcode 16
  address = state.pc + 1;
  READ_BYTE();
  state.d = memByte;
  state.pc += 2;
}

void RLA( void )
{
  // opcode 17
  int old_cy = ISSET_C();
  if( state.a & 0x80 )
    SET_C();
  else
    RESET_C();
  
  state.a = state.a << 1;
  
  if( old_cy )
    state.a |= 0x01;
  
  // flag Z
  RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  RESET_H();
  
  state.pc++;
}

void JR_INDEX( void )
{
  // opcode 18
  address = state.pc + 1;
  READ_BYTE();
  state.pc += (s8)memByte + 2;
  return;
}

void ADD_HL_DE( void )
{
  // opcode 19
  int result = (int)(STATE_HL) + (int)(STATE_DE);
  
  // flag Z in not affected
  
  // flag N
  RESET_N();
  
  // flag H
  int c = (int)state.l + (int)state.e;
  if( c > 0xFF )
    c = 1;
  else
    c = 0;
  
  if( ((state.h & 0x0f) + (state.d & 0x0f) + c) & 0x10 )
    SET_H();
  else
    RESET_H();
  
  // flag C
  if( result > 0xFFFF )
    SET_C();
  else
    RESET_C();
  
  state.hl = result;
  
  state.pc++;
}
  

void LD_A_DE( void )
{
  // opcode 1A
  address = STATE_DE;
  READ_BYTE();
  state.a = memByte;
  state.pc++;
}

u8* cpu_getReg( int regNumber )
{
  switch(regNumber)
  {
    case 0:
      return &(state.b);
      break;
    case 1:
      return &(state.c);
      break;
    case 2:
      return &(state.d);
      break;
    case 3:
      return &(state.e);
      break;
    case 4:
      return &(state.h);
      break;
    case 5:
      return &(state.l);
      break;
    case 7:
      return &(state.a);
      break;
    default:
      // danger danger
      fprintf(stderr, "Register decode error\n");
      return &(state.a);
      break;
  }
}

void DEC_DE( void )
{
  // opcode 1B
  state.de--;
  state.pc++;
}

void LD_E_BYTE( void )
{
  // opcode 1E
  address = state.pc + 1;
  READ_BYTE();
  state.e = memByte;
  state.pc += 2;
}

void RRA( void )
{
  // opcode 1F
  int old_cy = ISSET_C();
  if( state.a & 0x01 )
    SET_C();
  else
    RESET_C();
  
  state.a = state.a >> 1;
  if( old_cy )
    state.a |= 0x80;
  
  // flag Z
  RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  RESET_H();
  
  state.pc++;
}

void JR_NZ_INDEX( void )
{
  // opcode 20
  if( ISSET_Z() )
  {
    state.pc += 2;
  }
  else
  {
    address = state.pc + 1;
    READ_BYTE();
    state.pc += (s8)memByte + 2;
    branched = 1;
  }
  return;
}

void LD_HL_WORD( void )
{
  // opcode 21
  address = state.pc + 1;
  READ_WORD();
  state.hl = memWord;
  
  state.pc+=3;
  return;
}

void LDI_HL_A( void )
{
  // opcode 22
  address = STATE_HL;
  memByte = state.a;
  WRITE_BYTE();
  state.hl++;
  
  state.pc++;
  return;
}

void INC_HL( void )
{
  // opcode 23
  // no flags affected
  state.hl++;
  state.pc++;
}

void LD_H_BYTE( void )
{
  // opcode 26
  address = state.pc + 1;
  READ_BYTE();
  state.h = memByte;
  // no flags changed
  state.pc += 2;
}

void DAA( void )
{
  // opcode 27
  // fuck everything about this
  
  int temp = state.a;
  
  if ( ISSET_N() )
  {
    if ( ISSET_H() )
      temp = (temp - 6) & 0xFF;
    
    if ( ISSET_C() )
      temp -= 0x60;
  }
  else
  {
    if ( ISSET_H() || (temp & 0xF) > 0x09)
      temp += 0x06;
    
    if ( ISSET_C() || temp > 0x9F)
      temp += 0x60;
  }
  
  state.a = temp;
  
  // flag Z
  if (state.a == 0)
    SET_Z();
  else
    RESET_Z();
  
  // flag N is not affected
  
  // flag H
  RESET_H();
  
  // flag C
  if ((temp & 0x100) == 0x100)
    SET_C();
  
  state.pc++;
}

void JR_Z_INDEX( void )
{
  // opcode 28
  if( ISSET_Z() )
  {
    address = state.pc + 1;
    READ_BYTE();
    state.pc += (s8)memByte + 2;
    branched = 1;
  }
  else
  {
    state.pc += 2;
  }
  return;
}

void ADD_HL_HL( void )
{
  // opcode 29
  int result = (int)(STATE_HL) + (int)(STATE_HL);
  
  // flag Z in not affected
  
  // flag N
  RESET_N();
  
  // flag H
  int c = (int)state.l + (int)state.l;
  if( c > 0xFF )
    c = 1;
  else
    c = 0;
  
  if( ((state.h & 0x0f) + (state.h & 0x0f) + c) & 0x10 )
    SET_H();
  else
    RESET_H();
  
  // flag C
  if( result > 0xFFFF )
    SET_C();
  else
    RESET_C();
  
  state.hl = result;
  
  state.pc++;
}
  

void LDI_A_HL( void )
{
  // opcode 2A
  address = STATE_HL;
  READ_BYTE();
  state.a = memByte;
  
  state.hl++;
  
  state.pc++;
}

void DEC_HL( void )
{
  // opcode 2B
  state.hl--;
  state.pc++;
}

void LD_L_BYTE( void )
{
  // opcode 2E
  address = state.pc + 1;
  READ_BYTE();
  state.l = memByte;
  state.pc += 2;
}

void CPL( void )
{
  // opcode 2F
  state.a = ~state.a;
  state.pc++;
  
  // flag Z is not affected
  
  // flag N
  SET_N();
  
  // flag H
  SET_H();
  
  // flag C is not affected
}

void JR_NC_INDEX( void )
{
  // opcode 30
  if( ISSET_C() )
  {
    state.pc += 2;
  }
  else
  {
    address = state.pc + 1;
    READ_BYTE();
    state.pc += (s8)memByte + 2;
    branched = 1;
  }
  return;
}

void LD_SP_WORD( void )
{
  // opcode 31
  address = state.pc + 1;
  READ_WORD();
  state.sp = memWord;
  
  state.pc+=3;
  return;
}

void LDD_HL_A( void )
{
  // opcode 32
  address = STATE_HL;
  memByte = state.a;
  WRITE_BYTE();
  
  state.hl--;
  state.pc++;
  return;
}

void INC_SP( void )
{
  // opcode 33
  state.sp++;
  
  // no flags affected
  
  state.pc++;
}

void INC_AT_HL( void )
{
  // opcode 34
  address = STATE_HL;
  READ_BYTE();
  int old_memByte = memByte;
  memByte++;
  int new_memByte = memByte;
  WRITE_BYTE();
  
  // flag Z
  if( memByte == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  if( (new_memByte & 0xF0) != (old_memByte & 0xF0) )
    SET_H();
  else
    RESET_H();
  
  // flag C is not affected
  
  state.pc++;
}

void DEC_AT_HL( void )
{
  // opcode 35
  address = STATE_HL;
  READ_BYTE();
  int old_memByte = memByte;
  memByte--;
  int new_memByte = memByte;
  WRITE_BYTE();
  
  // flag Z
  if( memByte == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  SET_N();
  
  // flag H
  if( (new_memByte & 0xF0) != (old_memByte & 0xF0) )
    SET_H();
  else
    RESET_H();
  
  // flag C is not affected
  
  state.pc++;
}

void LD_HL_BYTE( void )
{
  // opcode 36
  // ld [hl], $memByte
  address = state.pc + 1;
  READ_BYTE();
  address = STATE_HL;
  WRITE_BYTE();
  state.pc += 2;
}

void SCF( void )
{
  // opcode 37
  
  // just flags
  
  // flag Z is not affected
  
  // flag N
  RESET_N();
  
  // flag H
  RESET_H();
  
  // flag C
  SET_C();
  
  state.pc++;
}

void JR_C_INDEX( void )
{
  // opcode 38
  if( ISSET_C() )
  {
    address = state.pc + 1;
    READ_BYTE();
    state.pc += (s8)memByte + 2;
    branched = 1;
  }
  else
  {
    state.pc += 2;
  }
  return;
}

void ADD_HL_SP( void )
{
  // opcode 39
  int result = (int)(STATE_HL) + (int)(state.sp);
  
  // flag Z in not affected
  
  // flag N
  RESET_N();
  
  // flag H
  int c = (int)state.l + (int)(state.sp & 0x00ff);
  if( c > 0xFF )
    c = 1;
  else
    c = 0;
  
  if( ((state.h & 0x0f) + ((state.sp & 0xff00) >> 8 & 0x0f) + c) & 0x10 )
    SET_H();
  else
    RESET_H();
  
  // flag C
  if( result > 0xFFFF )
    SET_C();
  else
    RESET_C();
  
  state.hl = result;
  
  state.pc++;
}

void LDD_A_HL( void )
{
  // opcode 3A
  address = STATE_HL;
  READ_BYTE();
  state.a = memByte;
  
  state.hl--;
  state.pc++;
}

void DEC_SP( void )
{
  // opcode 3B
  state.sp--;
  
  // no flags affected
  
  state.pc++;
}

void LD_A_BYTE( void )
{
  // opcode 3E
  address = state.pc + 1;
  READ_BYTE();
  state.a = memByte;
  state.pc += 2;
}

void CCF( void )
{
  // opcode 3F
  
  // just flags
  
  // flag Z is not affected
  
  // flag N
  RESET_N();
  
  // flag H
  // The Z80 manual says: "previous carry is copied [into flag H]".
  // On a Game Boy, though, flag H just gets reset.
  RESET_H();
  
  // flag C
  if( ISSET_C() )
    RESET_C();
  else
    SET_C();
  
  state.pc++;
}

void LD_R_R( void )
{
  // LD r, R
  // 01rrrRRR
  
  address = state.pc;
  READ_BYTE();
  int targetRegNumber = ((int)(memByte) & 0x38) >> 3;
  int sourceRegNumber = ((int)(memByte) & 0x07);
  
  u8* targetReg = cpu_getReg(targetRegNumber);
  u8* sourceReg = cpu_getReg(sourceRegNumber);
  
  (*targetReg) = (*sourceReg);
  
  state.pc++;
}

void LD_B_HL( void )
{
  // opcode 46
  address = STATE_HL;
  READ_BYTE();
  state.b = memByte;
  state.pc++;
}

void LD_C_HL( void )
{
  // opcode 4E
  address = STATE_HL;
  READ_BYTE();
  state.c = memByte;
  state.pc++;
}

void LD_D_HL( void )
{
  // opcode 56
  address = STATE_HL;
  READ_BYTE();
  state.d = memByte;
  state.pc++;
}

void LD_E_HL( void )
{
  // opcode 5E
  address = STATE_HL;
  READ_BYTE();
  state.e = memByte;
  state.pc++;
}

void LD_H_HL( void )
{
  // opcode 66
  address = STATE_HL;
  READ_BYTE();
  state.h = memByte;
  state.pc++;
}

void LD_L_HL( void )
{
  // opcode 6E
  address = STATE_HL;
  READ_BYTE();
  state.l = memByte;
  state.pc++;
}

void LD_HL_R( void )
{
  // opcode 70-75,77
  // LD (HL), r
  // 01110rrr
  
  address = state.pc;
  READ_BYTE();
  int regNumber = ((int)(memByte) & 0x07);
  u8* reg = cpu_getReg(regNumber);
  
  address = STATE_HL;
  memByte = (*reg);
  WRITE_BYTE();
  state.pc++;
  return;
}

void HALT( void )
{
  // opcode 76
  state.halt = 1;
  state.pc++;
}

void LD_A_HL( void )
{
  // opcode 7E
  address = STATE_HL;
  READ_BYTE();
  state.a = memByte;
  state.pc++;
}

void ADD_A_R( void )
{
  // opcode 80-85,87
  // 10000rrr
  
  address = state.pc;
  READ_BYTE();
  int regNumber = ((int)(memByte) & 0x07);
  u8* reg = cpu_getReg(regNumber);
  
  int old_a = (int)(state.a);
  int new_a = (int)(state.a) + (int)(*reg);
  
  state.a = new_a;
  
  // flag Z
  if( state.a == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  if( ( (old_a & 0x0F) + (*reg & 0x0F) ) & 0x10 )
    SET_H();
  else
    RESET_H();
  
  // flag C
  if( new_a > 0xFF )
    SET_C();
  else
    RESET_C();
  
  state.pc++;
}

void ADD_A_HL( void )
{
  // opcode 86
  address = STATE_HL;
  READ_BYTE();
  
  int new_a = (int)(state.a) + (int)(memByte);
  
  state.a = new_a;
  
  // flag Z
  if( state.a == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  if( ((int)(state.a & 0x0F) - (int)(memByte & 0x0F)) < 0 )
    SET_H();
  else
    RESET_H();
  
  // flag C
  if( new_a > 0xFF )
    SET_C();
  else
    RESET_C();
  
  state.pc++;
}

void ADC_A_R( void )
{
  // opcode 88-8D,8F
  // 10001rrr
  
  int temp=0;
  int c;
  if( ISSET_C() )
    c = 1;
  else
    c = 0;
  
  address = state.pc;
  READ_BYTE();
  int regNumber = ((int)(memByte) & 0x07);
  u8* reg = cpu_getReg(regNumber);
  
  temp = state.a + *reg + c;
  int old_a = state.a;
  state.a = temp;
  
  // flag Z
  if( state.a == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  if( ( (old_a & 0x0F) + (*reg & 0x0F) + c ) & 0x10 )
    SET_H();
  else
    RESET_H();
  
  // flag C
  if( temp > 0xFF )
    SET_C();
  else
    RESET_C();
  
  state.pc++;
}

void ADC_A_HL( void )
{
  // opcode 8E
  
  int temp=0;
  int c;
  if( ISSET_C() )
    c = 1;
  else
    c = 0;
  
  address = STATE_HL;
  READ_BYTE();
  
  temp = state.a + memByte + c;
  int old_a = state.a;
  state.a = temp;
  
  // flag Z
  if( state.a == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  if( ( (old_a & 0x0F) + (memByte & 0x0F) + c ) & 0x10 )
    SET_H();
  else
    RESET_H();
  
  // flag C
  if( temp > 0xFF )
    SET_C();
  else
    RESET_C();
  
  state.pc++;
}

void SUB_R( void )
{
  // opcodes 90-95,97
  // 10010rrr
  
  address = state.pc;
  READ_BYTE();
  int regNumber = ((int)(memByte) & 0x07);
  u8* reg = cpu_getReg(regNumber);
  
  int old_a = (int)(state.a);
  int new_a = (int)(state.a) - (int)(*reg);
  
  state.a = new_a;
  
  // flag Z
  if( state.a == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  SET_N();
  
  // flag H
  if( ( (old_a & 0x0F) - (*reg & 0x0F) ) & 0x10 )
    SET_H();
  else
    RESET_H();
  
  // flag C
  if( new_a < 0 )
    SET_C();
  else
    RESET_C();
  
  state.pc++;
}

void SUB_HL( void )
{
  // opcode 96
  
  address = STATE_HL;
  READ_BYTE();
  
  int old_a = (int)(state.a);
  int new_a = (int)(state.a) - (int)(memByte);
  
  state.a = new_a;
  
  // flag Z
  if( state.a == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  SET_N();
  
  // flag H
  if( ( (old_a & 0x0F) - (memByte & 0x0F) ) & 0x10 )
    SET_H();
  else
    RESET_H();
  
  // flag C
  if( new_a < 0 )
    SET_C();
  else
    RESET_C();
  
  state.pc++;
}

void SBC_A_R( void )
{
  // opcodes 98-9D,9F
  // 10011rrr
  
  int temp=0;
  int c;
  if( ISSET_C() )
    c = 1;
  else
    c = 0;
  
  address = state.pc;
  READ_BYTE();
  int regNumber = ((int)(memByte) & 0x07);
  u8* reg = cpu_getReg(regNumber);
  
  temp = state.a - *reg - c;
  int old_a = state.a;
  state.a = temp;
  
  // flag Z
  if( state.a == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  SET_N();
  
  // flag H
  if( ( (old_a & 0x0F) - (*reg & 0x0F) - c ) & 0x10 )
    SET_H();
  else
    RESET_H();
  
  // flag C
  if( temp < 0 )
    SET_C();
  else
    RESET_C();
  
  state.pc++;
}

void SBC_A_HL( void )
{
  // opcode 9E
  
  int temp=0;
  int c;
  if( ISSET_C() )
    c = 1;
  else
    c = 0;
  
  address = STATE_HL;
  READ_BYTE();
  
  temp = state.a - memByte - c;
  int old_a = state.a;
  state.a = temp;
  
  // flag Z
  if( state.a == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  SET_N();
  
  // flag H
  if( ( (old_a & 0x0F) - (memByte & 0x0F) - c ) & 0x10 )
    SET_H();
  else
    RESET_H();
  
  // flag C
  if( temp < 0 )
    SET_C();
  else
    RESET_C();
  
  state.pc++;
}

void AND_R( void )
{
  // opcodes A0-A5,A7
  // 10100rrr
  
  address = state.pc;
  READ_BYTE();
  int regNumber = ((int)(memByte) & 0x07);
  u8 *reg = cpu_getReg(regNumber);
  
  state.a &= (*reg);
  
  // flag Z
  if( state.a == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  SET_H();
  
  // flag C
  RESET_C();
  
  state.pc++;
}

void AND_HL( void )
{
  // opcode A6
  address = STATE_HL;
  READ_BYTE();
  state.a &= memByte;
  
  // flag Z
  if( state.a == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  SET_H();
  
  // flag C
  RESET_C();
  
  state.pc++;
}

void XOR_R( void )
{
  // opcodes A8-AD,AF
  // 10110rrr
  
  address = state.pc;
  READ_BYTE();
  int regNumber = ((int)(memByte) & 0x07);
  u8 *reg = cpu_getReg(regNumber);
  
  state.a = state.a ^ (*reg);
  
  // flag Z
  if( state.a == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  RESET_H();
  
  // flag C
  RESET_C();
  
  state.pc++;
}

void XOR_HL( void )
{
  // opcode AE
  address = STATE_HL;
  READ_BYTE();
  
  state.a ^= memByte;
  
  // flag Z
  if( state.a == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  RESET_H();
  
  // flag C
  RESET_C();
  
  state.pc++;
}

void OR_R( void )
{
  // opcode B0, B1, B2, B3, B4, B5, B7
  // OR r
  // 10110rrr
  
  address = state.pc;
  READ_BYTE();
  int regNumber = ((int)(memByte) & 0x07);
  u8* reg = cpu_getReg(regNumber);
  
  state.a |= (*reg);
  
  // flag Z
  if( state.a == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  RESET_H();
  
  // flag C
  RESET_C();
  
  state.pc++;
}

void OR_HL( void )
{
  // opcode B6
  // OR (HL)
  
  address = STATE_HL;
  READ_BYTE();
  
  state.a |= memByte;
  
  // flag Z
  if( state.a == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  RESET_H();
  
  // flag C
  RESET_C();
  
  state.pc++;
}

void CP_R( void )
{
  // opcodes B8-BD,BF
  // CP r
  // 10111rrr
  
  address = state.pc;
  READ_BYTE();
  
  int regNumber = (int)(memByte) & 0x07;
  u8* reg = cpu_getReg(regNumber);
  
  int temp = (int)state.a - (int)(*reg);
  
  // flag Z
  if( (temp & 0xFF) == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  SET_N();
  
  // flag H
  if( ((int)(state.a & 0x0F) - (int)(*reg & 0x0F)) < 0 )
    SET_H();
  else
    RESET_H();
  
  // flag C
  if( temp < 0 )
    SET_C();
  else
    RESET_C();
  
  state.pc++;
}

void CP_HL( void )
{
  // opcodes BE
  // CP (HL)
  // 10111110
  
  address = STATE_HL;
  READ_BYTE();
  
  int temp = (int)state.a - (int)(memByte);
  
  // flag Z
  if( (temp & 0xFF) == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  SET_N();
  
  // flag H
  if( ((int)(state.a & 0x0F) - (int)(memByte & 0x0F)) < 0 )
    SET_H();
  else
    RESET_H();
  
  // flag C
  if( temp < 0 )
    SET_C();
  else
    RESET_C();
  
  state.pc++;
}

void RET_CC( void)
{
//   printf("RET_CC: pc: %04X\n", state.pc);
  // opcodes C0, C8, D0, D8
  // 11ccc000
  // ccc: condition
  // 000: NZ
  // 001: Z
  // 010: NC
  // 011: C
  
  int condition = (state.op & 0x38) >> 3;
  
  switch( condition )
  {
    case 0:	// NZ
      if( ! ISSET_Z() )
      {
	RET();
	return;
      }
      break;
    case 1:	// Z
      if( ISSET_Z() )
      {
	RET();
	return;
      }
      break;
    case 2:	// NC
      if( ! ISSET_C() )
      {
	RET();
	return;
      }
      break;
    case 3:	// C
      if( ISSET_C() )
      {
	RET();
	return;
      }
      break;
    default:
      fprintf(stderr, "ERROR: RET_CC decode error, pc: %04X\n", state.pc);
      exit(1);
  }
  
  state.pc++;
//   printf("Didn't return. old_pc: %X, pc: %X\n", old_pc, state.pc);
}

  

void POP_BC( void )
{
  // opcode C1

  address = state.sp;
  READ_WORD();
  state.bc = memWord;
  
  state.sp += 2;
  
  state.pc++;
}

void JP_NZ_ADDR( void )
{
  // opcode C2
  if( ISSET_Z() )
  {
    state.pc += 3;
    return;
  }
  address = state.pc + 1;
  READ_WORD();
  state.pc = memWord;
  branched = 1;
}

void JP_ADDR( void )
{
  // opcode C3
  address = state.pc + 1;
  READ_WORD();
  state.pc = memWord;
}

void CALL_NZ( void )
{
  // opcode C4
  if( ISSET_Z() )
  {
    state.pc +=3;
  }
  else
  {
    state.sp -= 2;
    address = state.sp;
    memWord = state.pc + 3;
    WRITE_WORD();
    
    address = state.pc + 1;
    READ_WORD();
//     printf("CALL_NZ address: %x, pc: %x, memWord:%x\n", address, state.pc, memWord);
    state.pc = memWord;
  }
}

void PUSH_BC( void )
{
  // opcode C5
  state.sp -= 2;
  address = state.sp;
  memWord = state.bc;
  WRITE_WORD();
  
  // no flags affected
  
  state.pc++;
}

void ADD_A_BYTE( void )
{
  // opcode C6
  address = state.pc + 1;
  READ_BYTE();
  int temp = (int)(state.a) + (int)(memByte);
  int old_a = state.a;
  state.a += memByte;
  
  // flag Z
  if( state.a == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  if( ( (old_a & 0x0F) + (memByte & 0x0F) ) & 0x10 )
    SET_H();
  else
    RESET_H();
  
  // flag C
  if( temp > 0xff )
    SET_C();
  else
    RESET_C();
  
  state.pc += 2;
}

void RET( void )
{
  // opcode C9
//   int old_pc = state.pc;
//   int low, high;
//   address = state.sp;
//   READ_BYTE();
//   low = memByte;
//   state.sp++;
//   address = state.sp;
//   READ_BYTE();
//   high = (int)(memByte) << 8;
//   state.sp++;

  address = state.sp;
  READ_WORD();
  state.pc = memWord;
  state.sp += 2;
  
//   state.pc = low | high;
  
//   printf("RET pc: %04X, retaddr: %04X\n", old_pc, state.pc);
}

void JP_Z_ADDR( void )
{
  // opcode CA
  if( ISSET_Z() )
  {
    address = state.pc + 1;
    READ_WORD();
    state.pc = memWord;
    branched = 1;
    return;
  }
  state.pc += 3;
}

void CB_PREFIX( void )
{
  // opcode CB
  // lots of instructions use the CB prefix
//   printf("CB_PREFIX\n");
  address = state.pc + 1;
  READ_BYTE();
  cb_ops[memByte]();
  return;
}

void CB_RLC_R( void )
{
  // opcodes CB 00-05,07
  // 11001011 CB
  // 00000rrr
  
  address = state.pc + 1;
  READ_BYTE();
  int regNumber = memByte & 0x07;
  u8 *reg = cpu_getReg(regNumber);
  
  if( ((*reg) & 0x80) == 0 )
  {
    RESET_C();
    (*reg) = (*reg) << 1;
  } else {
    SET_C();
    (*reg) = ((*reg) << 1) | 1;
  }
  
  if( (*reg) == 0 )
    SET_Z();
  else
    RESET_Z();
  
  RESET_N();
  
  RESET_H();
  
  state.pc += 2;
}

void CB_RLC_HL( void )
{
  // opcode CB 06
  address = STATE_HL;
  READ_BYTE();
  
  if( (memByte & 0x80) == 0 )
  {
    RESET_C();
    memByte = memByte << 1;
  } else {
    SET_C();
    memByte = (memByte << 1) | 1;
  }
  
  WRITE_BYTE();
  
  // flag Z
  if( memByte == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  RESET_H();
  
  state.pc += 2;
}  

void CB_RRC_R( void )
{
  // opcodes CB 08-0D,0F
  // 11001011 CB
  // 0001rrr
  
  address = state.pc + 1;
  READ_BYTE();
  int regNumber = memByte & 0x07;
  u8 *reg = cpu_getReg(regNumber);
  
  if( ((*reg) & 0x01) == 0 )
  {
    RESET_C();
    (*reg) = (*reg) >> 1;
  } else {
    SET_C();
    (*reg) = ((*reg) >> 1) | 0x80;
  }
  
  
  // flag Z
  if( (*reg) == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  RESET_H();
  
  state.pc += 2;
}

void CB_RRC_HL( void )
{
  // opcode CB 0E
  address = STATE_HL;
  READ_BYTE();
  
  if( (memByte & 0x01) == 0 )
  {
    RESET_C();
    memByte = memByte >> 1;
  } else {
    SET_C();
    memByte = (memByte >> 1) | 0x80;
  }
  
  WRITE_BYTE();
  
  // flag Z
  if( memByte == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  RESET_H();
  
  state.pc += 2;
}

void CB_RL_R( void )
{
  // opcodes CB 10-15,17
  // 11001011 CB
  // 00010rrr
  
  address = state.pc + 1;
  READ_BYTE();
  int regNumber = memByte & 0x07;
  u8 *reg = cpu_getReg(regNumber);
  
  int old_cy = ISSET_C();
  if( *reg & 0x80 )
    SET_C();
  else
    RESET_C();
  
  *reg = *reg << 1;
  
  if( old_cy )
    *reg |= 0x01;
  
  // flag Z
  if( *reg == 0x00 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  RESET_H();
  
  state.pc += 2;
}

void CB_RL_HL( void )
{
  // opcode CB 16
  // 11001011 CB
  // 00010110
  
  address = STATE_HL;
  READ_BYTE();
  
  int old_cy = ISSET_C();
  if( memByte & 0x80 )
    SET_C();
  else
    RESET_C();
  
  memByte = memByte << 1;
  
  if( old_cy )
    memByte |= 0x01;
  
  // flag Z
  if( memByte == 0x00 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  RESET_H();
  
  WRITE_BYTE();
  
  state.pc += 2;
}

void CB_RR_R( void )
{
  // opcodes CB 18-1D,1F
  // 11001011 CB
  // 00001rrr
  
  address = state.pc + 1;
  READ_BYTE();
  int regNumber = memByte & 0x07;
  u8 *reg = cpu_getReg(regNumber);
  
  int old_cy = ISSET_C();
  if( *reg & 0x01 )
    SET_C();
  else
    RESET_C();
  
  *reg = *reg >> 1;
  
  if( old_cy )
    *reg |= 0x80;
  
  // flag Z
  if( *reg == 0x00 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  RESET_H();
  
  state.pc += 2;
}

void CB_RR_HL( void )
{
  // opcode CB 1E
  
  address = STATE_HL;
  READ_BYTE();
  
  int old_cy = ISSET_C();
  if( memByte & 0x01 )
    SET_C();
  else
    RESET_C();
  
  memByte = memByte >> 1;
  
  if( old_cy )
    memByte |= 0x80;
  
  // flag Z
  if( memByte == 0x00 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  RESET_H();
  
  WRITE_BYTE();
  
  state.pc += 2;
}

void CB_SLA_R( void )
{
  // opcode CB 20-25,7
  // 11001011 CB
  // 00100rrr
  
  address = state.pc + 1;
  READ_BYTE();
  int regNumber = memByte & 0x07;
  u8 *reg = cpu_getReg(regNumber);
  
  if( *reg & 0x80 )
    SET_C();
  else
    RESET_C();
  
  *reg = *reg << 1;
  
  // flag Z
  if( *reg == 0x00 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  RESET_H();
  
  state.pc += 2;
}

void CB_SLA_HL( void )
{
  // opcode CB 26
  
  address = STATE_HL;
  READ_BYTE();
  
  if( memByte & 0x80 )
    SET_C();
  else
    RESET_C();
  
  memByte = memByte << 1;
  
  // flag Z
  if( memByte == 0x00 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  RESET_H();
  
  WRITE_BYTE();
  
  state.pc += 2;
}

void CB_SRA_R( void )
{
  // opcode CB 28-2D,2F
  // 11001011 CB
  // 00101rrr
  
  address = state.pc + 1;
  READ_BYTE();
  int regNumber = memByte & 0x07;
  u8 *reg = cpu_getReg(regNumber);
  int bit7 = (*reg) & 0x80;
  
  // flag C
  if( (*reg) & 0x01 )
    SET_C();
  else
    RESET_C();
  
  (*reg) = (*reg) >> 1 | bit7;
  
  // flag Z
  if( (*reg) == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  RESET_H();
  
  state.pc += 2;
}

void CB_SRA_HL( void )
{
  // opcode CB 2E
  address = STATE_HL;
  READ_BYTE();
  int bit7 = memByte & 0x80;
  
  // flag C
  if( memByte & 0x01 )
    SET_C();
  else
    RESET_C();
  
  memByte = memByte >> 1 | bit7;
  
  WRITE_BYTE();
  
  // flag Z
  if( memByte == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  RESET_H();
  
  state.pc += 2;
}

void CB_SWAP_R( void )
{
  // opcode CB 30-35,37
  // 11001011 CB
  // 00110rrr
  
  address = state.pc + 1;
  READ_BYTE();
  int regNumber = memByte & 0x07;
  u8 *reg = cpu_getReg(regNumber);
  
  u8 temp = ((*reg) & 0xF0) >> 4;
  (*reg) = ((*reg) << 4) | temp;
  
  // flag Z
  if( *reg == 0x00 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  RESET_H();
  
  // flag C
  RESET_C();
  
  state.pc += 2;
}

void CB_SWAP_HL( void )
{
  // opcode CB 36
  
  address = STATE_HL;
  READ_BYTE();
  
  u8 temp = (memByte & 0xF0) >> 4;
  memByte = (memByte << 4) | temp;
  
  // flag Z
  if( memByte == 0x00 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  RESET_H();
  
  // flag C
  RESET_C();
  
  WRITE_BYTE();
  
  state.pc += 2;
}

void CB_SRL_R( void )
{
  // opcode CB 38-3D,3F
  // 11001011 CB
  // 00111rrr
  
  address = state.pc + 1;
  READ_BYTE();
  int regNumber = memByte & 0x07;
  u8 *reg = cpu_getReg(regNumber);
  
  if( (*reg) & 0x01 )
    SET_C();
  else
    RESET_C();
  
  (*reg) = (*reg) >> 1;
  
  // flag Z
  if( (*reg) == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  RESET_H();
  
  state.pc += 2;
}

void CB_SRL_HL( void )
{
  // opcode CB 38-3D,3F
  address = STATE_HL;
  READ_BYTE();
  
  if( memByte & 0x01 )
    SET_C();
  else
    RESET_C();
  
  memByte = memByte >> 1;
  
  WRITE_BYTE();
  
  // flag Z
  if( memByte == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  RESET_H();
  
  state.pc += 2;
}

void CB_BIT_R( void )
{
  // opcode CB 40-7F
  // BIT b, r
  // 11001011 CB
  // 01bbbrrr
  
  address = state.pc + 1;
  READ_BYTE();
  
  int bitToTest = ((int)(memByte) & 0x38) >> 3;
  int regNumber = ((int)(memByte) & 0x07);
  u8 *reg = cpu_getReg(regNumber);
  
  int bitMask = 1 << bitToTest;
  
  u8 temp = *reg & bitMask;
  
  // flag Z
  if( temp == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  SET_H();
  
  // flag C is not affected
  
  state.pc+=2;
}

void CB_BIT_HL( void )
{
  // opcode CB 46,4E,56,5E,66,6E,76,7E
  // BIT b, (HL)
  // 11001011 CB
  // 01bbb110
  
  address = state.pc + 1;
  READ_BYTE();
  
  int bitToTest = ((int)(memByte) & 0x38) >> 3;
  int bitMask = 1 << bitToTest;
  
  address = STATE_HL;
  READ_BYTE();
  int temp = memByte & bitMask;
  
  // flag Z
  if( temp == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  SET_H();
  
  // flag C is not affected
  
  state.pc+=2;
}

void CB_RES_B_R( void )
{
  // opcodes CB 80-BF (except (HL))
  // RES b, m
  // example: RES 0, B
  // 11001011 CB
  // 10bbbrrr
  
  address = state.pc + 1;
  READ_BYTE();
  
  int bitToReset = ((int)(memByte) & 0x38) >> 3;
  int regNumber = ((int)(memByte) & 0x07);
  u8 *reg = cpu_getReg(regNumber);
  
  int bitMask = ~(1 << bitToReset);
  
  *reg = *reg & bitMask;
  
  state.pc += 2;
}

void CB_RES_B_HL( void )
{
  // opcodes CB 86,8E,96,9E,A6,AE,B6,BE
  // RES b, (HL)
  // example: RES 0, (HL)
  // 11001011 CB
  // 10bbb110 86,8E,96,9E,A6,AE,B6,BE
  
  address = state.pc + 1;
  READ_BYTE();
  
  int bitToReset = ((int)(memByte) & 0x38) >> 3;
  int bitMask = ~(1 << bitToReset);
  
  address = STATE_HL;
  READ_BYTE();
  memByte = memByte & bitMask;
  WRITE_BYTE();
  
  state.pc += 2;
}

void CB_SET_B_R( void )
{
  // opcodes CB C0-FF (except (HL))
  // SET b, m
  // example: SET 0, B
  // 11001011 CB
  // 11bbbrrr
  
  address = state.pc + 1;
  READ_BYTE();
  
  int bitToReset = ((int)(memByte) & 0x38) >> 3;
  int regNumber = ((int)(memByte) & 0x07);
  u8 *reg = cpu_getReg(regNumber);
  
  int bitMask = 1 << bitToReset;
  
  *reg = *reg | bitMask;
  
  state.pc += 2;
}

void CB_SET_B_HL( void )
{
  // opcodes CB C6,CE,D6,DE,E6,EE,F6,FE
  // SET b, (HL)
  // example: SET 0, (HL)
  // 11001011 CB
  // 11bbb110 C6,CE,D6,DE,E6,EE,F6,FE
  
  address = state.pc + 1;
  READ_BYTE();
  
  int bitToReset = ((int)(memByte) & 0x38) >> 3;
  int bitMask = 1 << bitToReset;
  
  address = STATE_HL;
  READ_BYTE();
  memByte = memByte | bitMask;
  WRITE_BYTE();
  
  state.pc += 2;
}

void CALL_Z( void )
{
  // opcode CC
  if( ! ISSET_Z() )
  {
    state.pc +=3;
  }
  else
  {
    state.sp -= 2;
    address = state.sp;
    memWord = state.pc + 3;
    WRITE_WORD();
    
    address = state.pc + 1;
    READ_WORD();
    state.pc = memWord;
  }
}

void CALL( void )
{
  // opcode CD
  state.sp -= 2;
  address = state.sp;
  memWord = state.pc + 3;
  WRITE_WORD();
  
  address = state.pc + 1;
  READ_WORD();
//   printf("CALL address: %x, pc: %x, memWord:%x\n", address, state.pc, memWord);
  state.pc = memWord;
}

void ADC_A_BYTE( void )
{
  // opcode CE
  int temp=0;
  int c;
  if( ISSET_C() )
    c = 1;
  else
    c = 0;
  
  address = state.pc + 1;
  READ_BYTE();
  temp = state.a + memByte + c;
  int old_a = state.a;
  state.a = temp;
  
  // flag Z
  if( state.a == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  if( ( (old_a & 0x0F) + (memByte & 0x0F) + c ) & 0x10 )
    SET_H();
  else
    RESET_H();
  
  // flag C
  if( temp > 0xFF )
    SET_C();
  else
    RESET_C();
  
  state.pc += 2;
}

void RST_8( void )
{
  // opcode CF
  state.sp -= 2;
  address = state.sp;
  memWord = state.pc + 1;
  WRITE_WORD();
  
//   printf("RST_8 address: %x, pc: %x\n", address, state.pc);
  state.pc = 0x0008;
} 

void POP_DE( void )
{
  // opcode D1
//   address = state.sp;
//   READ_BYTE();
//   state.e = memByte;
//   
//   address = state.sp + 1;
//   READ_BYTE();
//   state.d = memByte;

  address = state.sp;
  READ_WORD();
  state.de = memWord;
  
  state.sp += 2;
  
  state.pc++;
}

void JP_NC_ADDR( void )
{
  // opcode C2
  if( ISSET_C() )
  {
    state.pc += 3;
    return;
  }
  address = state.pc + 1;
  READ_WORD();
  state.pc = memWord;
  branched = 1;
}

void RST_0( void )
{
  // opcode C7
  state.sp -= 2;
  address = state.sp;
  memWord = state.pc + 1;
  WRITE_WORD();
  
//   printf("RST_0 address: %x, pc: %x\n", address, state.pc);
  state.pc = 0x0000;
}

void CALL_NC( void )
{
  // opcode D4
  if( ISSET_C() )
  {
    state.pc +=3;
  }
  else
  {
    state.sp -= 2;
    address = state.sp;
    memWord = state.pc + 3;
    WRITE_WORD();
    
    address = state.pc + 1;
    READ_WORD();
    state.pc = memWord;
  }
}

void PUSH_DE( void )
{
  // opcode D5
  state.sp -= 2;
  address = state.sp;
  memWord = state.de;
  WRITE_WORD();
  state.pc++;
}

void SUB_A_BYTE( void )
{
  // opcode D6
  address = state.pc + 1;
  READ_BYTE();
  int temp = (int)(state.a) - (int)(memByte);
  int old_a = state.a;
  state.a -= memByte;
  
  // flag Z
  if( state.a == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  SET_N();
  
  // flag H
  if( ( (old_a & 0x0F) - (memByte & 0x0F) ) & 0x10 )
    SET_H();
  else
    RESET_H();
  
  // flag C
  if( temp < 0 )
    SET_C();
  else
    RESET_C();
  
  state.pc += 2;
}

void RST_10( void )
{
  // opcode D7
  state.sp -= 2;
  address = state.sp;
  memWord = state.pc + 1;
  WRITE_WORD();
  
//   printf("RST_10 address: %x, pc: %x\n", address, state.pc);
  state.pc = 0x0010;
}

void RETI( void )
{
  // opcode D9
  state.ime = IME_ENABLED;
  RET();
}

void JP_C_ADDR( void )
{
  // opcode DA
  if( ISSET_C() )
  {
    address = state.pc + 1;
    READ_WORD();
    state.pc = memWord;
    branched = 1;
    return;
  }
  state.pc += 3;
}

void CALL_C( void )
{
  // opcode DC
  if( ! ISSET_C() )
  {
    state.pc +=3;
  }
  else
  {
    state.sp -= 2;
    address = state.sp;
    memWord = state.pc + 3;
    WRITE_WORD();
    
    address = state.pc + 1;
    READ_WORD();
    state.pc = memWord;
  }
}

void SBC_A_BYTE( void )
{
  // opcode DE
  int temp=0;
  int c;
  if( ISSET_C() )
    c = 1;
  else
    c = 0;
  
  address = state.pc + 1;
  READ_BYTE();
  temp = state.a - memByte - c;
  int old_a = state.a;
  state.a = temp;
  
  // flag Z
  if( state.a == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  SET_N();
  
  // flag H
  if( ( (old_a & 0x0F) - (memByte & 0x0F) - c ) & 0x10 )
    SET_H();
  else
    RESET_H();
  
  // flag C
  if( temp < 0 )
    SET_C();
  else
    RESET_C();
  
  state.pc += 2;
}

void RST_18( void )
{
  // opcode DF
  state.sp -= 2;
  address = state.sp;
  memWord = state.pc + 1;
  WRITE_WORD();
  
//   printf("RST_18 address: %x, pc: %x\n", address, state.pc);
  state.pc = 0x0018;
}

void LD_FF_BYTE_A( void )
{
  // opcode E0
  // ld (ff00+memByte),a
  // also seen as: sta memByte
  address = state.pc + 1;
  READ_BYTE();
  address = 0xFF00 + memByte;
  memByte = state.a;
  WRITE_BYTE();
  state.pc += 2;
  return;
}

void POP_HL( void )
{
  // opcode E1
  address = state.sp;
  READ_WORD();
  state.hl = memWord;
  
  state.sp += 2;
  
  state.pc++;
}

void LD_FF_C_A( void )
{
  // opcode E2
  memByte = state.a;
  address = 0xFF00 + state.c;
  WRITE_BYTE();
  state.pc++;
  return;
}

void PUSH_HL( void )
{
  // opcode E5
  state.sp -= 2;
  address = state.sp;
  memWord = state.hl;
  WRITE_WORD();
  state.pc++;
}

void AND_BYTE( void )
{
  // opcode E6
  address = state.pc + 1;
  READ_BYTE();
  state.a &= memByte;
  
  //flag Z
  if( state.a == 0)
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  SET_H();
  
  // flag C
  RESET_C();
  
  state.pc += 2;
}

void RST_20( void )
{
  // opcode E7
  state.sp -= 2;
  address = state.sp;
  memWord = state.pc + 1;
  WRITE_WORD();
  
//   printf("RST_20 address: %x, pc: %x\n", address, state.pc);
  state.pc = 0x0020;
}

void ADD_SP_OFFSET( void )
{
  // opcode E8
  address = state.pc + 1;
  READ_BYTE();
  int old_sp = state.sp;
  int offset = (int)((s8)memByte);
  int temp = (int)state.sp + offset;
  state.sp = temp;
  
  // flag Z
  RESET_Z();
  
  // flag N
  RESET_N();
  
  // flags H and C are a little complicated.
  // special thanks to Fascia for clarification:
  // http://stackoverflow.com/questions/5159603/gbz80-how-does-ld-hl-spe-affect-h-and-c-flags/7261149#7261149
  if( offset >= 0 )
  {
    if( (old_sp & 0xFF) + offset > 0xFF )
      SET_C();
    else
      RESET_C();
    
    if( (old_sp & 0x0F) + (offset & 0x0F) > 0x0F )
      SET_H();
    else
      RESET_H();
  }
  else	// offset < 0
  {
    if( (temp & 0xFF) <= (old_sp & 0xFF) )
      SET_C();
    else
      RESET_C();
    
    if( (temp & 0x0F) <= (old_sp & 0x0F) )
      SET_H();
    else
      RESET_H();
  }
  
  state.pc += 2;
}

void JP_HL( void )
{
  // opcode E9
//   printf("JP_HL: hl: %04X\n", STATE_HL);
  state.pc = STATE_HL;
}

void LD_WORD_A( void )
{
  // opcode EA
  address = state.pc + 1;
  READ_WORD();
  address = memWord;
  memByte = state.a;
  WRITE_BYTE();
  state.pc += 3;
}

void XOR_BYTE( void )
{
  // opcode EE
  address = state.pc + 1;
  READ_BYTE();
  state.a ^= memByte;
  // flag Z
  if( state.a == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  RESET_H();
  
  // flag C
  RESET_C();
  
  state.pc += 2;
}

void RST_28( void )
{
  // opcode EF
  state.sp -= 2;
  address = state.sp;
  memWord = state.pc + 1;
  WRITE_WORD();
  
//   printf("RST_28 address: %x, pc: %x\n", address, state.pc);
  state.pc = 0x0028;
} 

void LD_A_FF_BYTE( void )
{
  // opcode F0
  // ld a,(ff00+memByte)
  address = state.pc + 1;
  READ_BYTE();
  
  address = 0xFF00 + memByte;
  READ_BYTE();
  
  state.a = memByte;
  state.pc += 2;
}

void POP_AF( void )
{
  // opcode F1
  address = state.sp;
  READ_WORD();
  state.af = memWord;
  
  // no flags affected
  
  state.sp += 2;
  state.pc++;
}

void LD_A_FF_C( void )
{
  // opcode F2
  // ld a,(ff00+c)
  address = 0xFF00 + (int)(state.c);
  READ_BYTE();
  state.a = memByte;
  
  state.pc++;
}

void DI( void )
{
  // opcode F3
  state.ime = IME_DISABLED;
  state.pc++;
}

void PUSH_AF( void )
{
  // opcode F5
  address = state.sp - 2;
  memWord = state.af;
  WRITE_WORD();
  
  state.sp -= 2;
  state.pc++;
}

void OR_BYTE( void )
{
  // opcode F6
  address = state.pc + 1;
  READ_BYTE();
  state.a |= memByte;
  
  // flag Z
  if( state.a == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  RESET_N();
  
  // flag H
  RESET_H();
  
  // flag C
  RESET_C();
  
  state.pc += 2;
}

void RST_30( void )
{
  // opcode F7
  state.sp -= 2;
  address = state.sp;
  memWord = state.pc + 1;
  WRITE_WORD();
  
//   printf("RST_30 address: %x, pc: %x\n", address, state.pc);
  state.pc = 0x0030;
}

void LDHL_SP_OFFSET( void )
{
  // opcode F8
  // ld hl, sp + SIGNED_OFFSET
  address = state.pc + 1;
  READ_BYTE();
  int offset = (int)((s8)memByte);
  int temp = (int)state.sp + offset;
  state.hl = temp;
  
  // flag Z
  RESET_Z();
  
  // flag N
  RESET_N();
  
  // flags H and C are a little complicated.
  // special thanks to Fascia for clarification:
  // http://stackoverflow.com/questions/5159603/gbz80-how-does-ld-hl-spe-affect-h-and-c-flags/7261149#7261149
  if( offset >= 0 )
  {
    if( (state.sp & 0xFF) + offset > 0xFF )
      SET_C();
    else
      RESET_C();
    
    if( (state.sp & 0x0F) + (offset & 0x0F) > 0x0F )
      SET_H();
    else
      RESET_H();
  }
  else	// offset < 0
  {
    if( (temp & 0xFF) <= (state.sp & 0xFF) )
      SET_C();
    else
      RESET_C();
    
    if( (temp & 0x0F) <= (state.sp & 0x0F) )
      SET_H();
    else
      RESET_H();
  }
  
  state.pc += 2;
}

void LD_SP_HL( void )
{
  // opcode F9
  state.sp = STATE_HL;
  
  // no flags affected
  
  state.pc++;
}

void LD_A_WORD( void )
{
  // opcode FA
  address = state.pc + 1;
  READ_WORD();
  address = memWord;
  READ_BYTE();
  state.a = memByte;
  state.pc += 3;
}

void EI( void )
{
  // opcode FB
  state.ime = IME_ENABLED;
  state.pc++;
}

void CP_BYTE( void )
{
  // opcode FE
  address = state.pc + 1;
  READ_BYTE();
  int temp = (int)state.a - (int)memByte;
  
  // flag Z
  if( (temp & 0xFF) == 0 )
    SET_Z();
  else
    RESET_Z();
  
  // flag N
  SET_N();
  
  // flag H
  if( ((int)(state.a & 0x0F) - (int)(memByte & 0x0F)) < 0 )
    SET_H();
  else
    RESET_H();
  
  // flag C
  if( temp < 0 )
    SET_C();
  else
    RESET_C();
  
  state.pc += 2;
}

void RST_38( void )
{
  // opcode FF
  state.sp -= 2;
  address = state.sp;
  memWord = state.pc + 1;
  WRITE_WORD();
  
//   printf("RST_38 address: %x, pc: %x\n", address, state.pc);
  state.pc = 0x0038;
}

void cpu_init() {
  state.masterClock = 0;
  state.div = 0;
  state.tima = 0;
  state.tma = 0;
  state.tac = 0;
  state.vid_cycles = 0;
  state.vid_mode = 0;
  state.lcdc = 0;
  state.ly = 0;
  state.bgp = 0;
  state.bootRomEnabled = 1;
  state.caps = 0x80;
  
  state.pc = 0x0000;
  state.sp = 0xFFFE;
  state.b  = 0;
  state.c  = 0;
  state.d  = 0;
  state.e  = 0;
  state.h  = 0;
  state.l  = 0;
  state.a  = 0;
  state.flag_c = 0;
  state.flag_h = 0;
  state.flag_n = 0;
  state.flag_z = 0;
  RESET_C();
  RESET_H();
  RESET_N();
  RESET_Z();
}

u8 cpu_get_flags_register( void )
{
    u8 temp = 0;
    if( ISSET_C() )
        temp |= FLAGS_C;
    if( ISSET_H() )
        temp |= FLAGS_H;
    if( ISSET_N() )
        temp |= FLAGS_N;
    if( ISSET_Z() )
        temp |= FLAGS_Z;
    printf( "cpu_get_flags_register: Z:%d N:%d H:%d C:%d %02x\n", ISSET_Z(), ISSET_N(), ISSET_H(), ISSET_C(), temp );
    return temp;
}

void cpu_set_flags_register( u8 flags )
{
    if( (flags & FLAGS_C) != 0 )
        SET_C();
    else
        RESET_C();
    
    if( (flags & FLAGS_H) != 0 )
        SET_H();
    else
        RESET_H();
    
    if( (flags & FLAGS_N) != 0 )
        SET_N();
    else
        RESET_N();
    
    if( (flags & FLAGS_Z) != 0 )
        SET_Z();
    else
        RESET_Z();
    
    printf( "cpu_set_flags_register: Z:%d N:%d H:%d C:%d %02x\n", ISSET_Z(), ISSET_N(), ISSET_H(), ISSET_C(), flags );
}

void cpu_do_one_frame()
{
  while( state.vid_cycles <= CYCLES_FRAME )
  {
    cpu_do_one_instruction();
  }
  
  state.vid_cycles %= CYCLES_FRAME;
  
}

// Run.
void cpu_do_one_instruction()
{
  
  // Handle pending interrupts.
  
  if( (state.ie & state.iflag) != 0x00 )
  {
    state.halt = 0;
  }
  
  u8 pendingInterrupts = state.ime & state.ie & state.iflag;
  if( pendingInterrupts != 0x00 )
  {
//     printf("ICHECK, ime: %02X, iflag: %02X, ie: %02X, pendingInterrupts: %02X\n", state.ime, state.iflag, state.ie, pendingInterrupts);
    state.ime = IME_DISABLED;
    if( pendingInterrupts & IMASK_VBLANK )
    {
//       printf("VBLANK, iflag: %02X, ie: %02X\n", state.iflag, state.ie);
      state.iflag &= ~IMASK_VBLANK;
      state.sp -= 2;
      address = state.sp;
      memWord = state.pc;
      WRITE_WORD();
      state.pc = 0x0040;
    }
    else if( pendingInterrupts & IMASK_LCD_STAT )
    {
//       printf("LCD_STAT, iflag: %02X, ie: %02X\n", state.iflag, state.ie);
      state.iflag &= ~IMASK_LCD_STAT;
      state.sp -= 2;
      address = state.sp;
      memWord = state.pc;
      WRITE_WORD();
      state.pc = 0x0048;
    }
    else if( pendingInterrupts & IMASK_TIMER )
    {
//       printf("TIMER, iflag: %02X, ie: %02X\n", state.iflag, state.ie);
      state.iflag &= ~IMASK_TIMER;
      state.sp -= 2;
      address = state.sp;
      memWord = state.pc;
      WRITE_WORD();
      state.pc = 0x0050;
    }
    else if( pendingInterrupts & IMASK_JOYPAD )
    {
//       printf("JOYPAD, iflag: %02X, ie: %02X\n", state.iflag, state.ie);
      state.iflag &= ~IMASK_JOYPAD;
      state.sp -= 2;
      address = state.sp;
      memWord = state.pc;
      WRITE_WORD();
      state.pc = 0x0060;
    }
  }
  
  int instr_time = 0;
//   int instr_length = 0;
  
  // Fetch one instruction.
  address = state.pc;
  READ_BYTE();
  state.op = memByte;
  
  // Set instruction length (in memBytes).
//   instr_length = op_lengths[state.op];
  
  // Set instruction time (in cycles).
  if( state.op == 0xCB ) {
    address = state.pc + 1;
    READ_BYTE();
    instr_time = op_cb_times[memByte];
  }
  else
  {
    if( branched )
    {
      branched = 0;
      instr_time = op_times_taken[state.op];
    }
    else
    {
      instr_time = op_times[state.op];
    }
  }
  
  // Execute the instruction.
  if( state.pc >= 0x10000 )
    state.pc -= 0x10000;
  
  if(state.halt == 0)
  {
    assert("Op address in range", state.pc <= 0xFFFE);
    ops[state.op]();
    // Reset the unused bits in the flags register
    state.f &= 0xF0;
  }
  
  // Update vid_cycles counter.
  state.vid_cycles += instr_time * 4;
  
  // Update LY.
  state.last_ly = state.ly;
  state.ly = state.vid_cycles / CYCLES_LINE;
  
  // Check LYC.
  if( state.ly == state.lyc )
  {
    state.stat |= 0x04;	// coincidence flag
    if( state.stat & 0x40 )
      state.iflag |= IMASK_LCD_STAT;
  }
  else
    state.stat &= 0xFB;
  
  
  // Update line_progress.
  int line_progress = state.vid_cycles % CYCLES_LINE;
  
  // Update vid_mode.
  if( state.ly >= 144 ) {
    // We're in the VBlank period (mode 1)
    state.old_vid_mode = state.vid_mode;
    state.vid_mode = 1;
    
    // Set the appropriate mode in the STAT register.
    state.stat = (state.stat & 0xFC) | 0x01;
    
    // Fire the interrupt, if enabled.
    if( (state.vid_mode != state.old_vid_mode) && (state.stat & 0x10) )
      state.iflag |= IMASK_LCD_STAT;
  }
  else
  {
    if(line_progress < (CYCLES_MODE_2))
    {
      // We're in the OAM period (mode 2)
      state.old_vid_mode = state.vid_mode;
      state.vid_mode = 2;
      
      // Set the appropriate mode in the STAT register.
      state.stat = (state.stat & 0xFC) | 0x02;
      
      // Fire the interrupt, if enabled.
      if( (state.vid_mode != state.old_vid_mode) && (state.stat & 0x20) )
        state.iflag |= IMASK_LCD_STAT;
    }
    else if(line_progress < (CYCLES_MODE_2 + CYCLES_MODE_3))
    {
      // We're in the VRAM/OAM period (mode 3)
      state.old_vid_mode = state.vid_mode;
      state.vid_mode = 3;
      
      // Set the appropriate mode in the STAT register.
      state.stat = (state.stat & 0xFC) | 0x03;
      
      // Is there a way for this event to fire an interrupt?
    }
    else
    {
      // We're in the HBlank period.
      state.old_vid_mode = state.vid_mode;
      state.vid_mode = 0;
      
      // Set the appropriate mode in the STAT register.
      state.stat = state.stat & 0xFC;
      
      // Fire the hblank interrupt, if enabled.
      if( (state.vid_mode != state.old_vid_mode) && (state.stat & 0x08) )
        state.iflag |= IMASK_LCD_STAT;
      
      // Is it time to render a frame?
      if( state.ly != state.last_line_rendered )
      {
	state.last_line_rendered = state.ly;
	vid_render_line();
      }
    }
  }
  
  
  // Update the master clock from which all timers are derived.
  // Incrememnted at 1048576 Hz (1/4 of a real gameboy).
  state.lastMasterClock = state.masterClock;
  state.masterClock = (state.masterClock + instr_time) % 1048576;
  
  // Update DIV.
  if( (state.masterClock % 64) < (state.lastMasterClock % 64) )
    state.div++;
  
  // Update TIMA.
  // TIMA is controlled by the TAC register.
  // TAC: -----210
  //		bit 2: timer stop (0=stop, 1=start)
  //		bits 1-0: clock select
  //			00:   4096 Hz
  //			01: 262144 Hz
  //			10:  65536 Hz
  //			11:  16384 Hz
  // When the clock overflows, it is reset to the value in TMA.
  if( (state.tac & 0x04) == 0x04 )
  {
//     printf("TAC check, tac: %02X\n", state.tac);
    int temp;
    switch(state.tac & 0x03)
    {
      case 0:
      default:
	temp = 256;	// 1048576 Hz / 4096 Hz
	break;
      case 1:
	temp = 4;	// 1048576 Hz / 262144 Hz
	break;
      case 2:
	temp = 16;	// 1048576 Hz / 65536 Hz
	break;
      case 3:
	temp = 64;	// 1048576 Hz / 16384 Hz
	break;
    }
//     printf("TAC temp: %d,one: %d, two: %d\n", temp, (state.masterClock % temp), (state.lastMasterClock % temp));
    if( (state.masterClock % temp) < (state.lastMasterClock % temp) )
    {
      state.tima++;
      if(state.tima == 0x00)
      {
	state.iflag |= IMASK_TIMER;
// 	printf("TIMER approaching! state.ime: %02X, state.ie: %02X, state.iflag: %02X\n", state.ime, state.ie, state.iflag);
      }
    }
    if( state.tima == 0x00 )
      state.tima = state.tma;
  }
  
  // Should we set any interrupts?
  // VBLANK
  if( (state.last_ly == 143) && (state.ly == 144) )
  {
    state.iflag |= IMASK_VBLANK;
  }
  
  // SERIAL
  // None of this actually works yet.
//   state.serialClocksSinceBitSent += instr_time;
//   if( (state.sc & (SC_SHIFT_CLOCK|SC_TRANSFER)) == (SC_SHIFT_CLOCK|SC_TRANSFER) )
//   {
//     // The internal clock is enabled.
//     if( state.serialClocksSinceBitSent >= 8192 )
//     {
//       printf("hi\n");
//       state.serialClocksSinceBitSent = 0;
//       state.sb = state.sb >> 1;
//       state.sb |= 0x80;
//       state.serialBitsSent++;
//       if( state.serialBitsSent == 8 )
//       {
// 	state.serialBitsSent = 0;
// 	state.iflag |= IMASK_SERIAL;
// 	state.sc &= ~SC_TRANSFER;
//       }
// 	
//       printf("%02X\n", state.sb);
//     }
//   }
    
  
  // JOYPAD
  // TODO
}