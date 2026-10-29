#ifndef CDM16_TRANSLATE_H
#define CDM16_TRANSLATE_H

#include "tcg/tcg.h"
#include <stdint.h>

/* Доступ к TCG-регистрам */
TCGv_i32 cdm16_tcg_reg(unsigned reg);
TCGv_i32 cdm16_tcg_sp(void);
TCGv_i32 cdm16_tcg_pc(void);
TCGv_i32 cdm16_tcg_ps(void);

void cdm16_gen_exception(unsigned exception, uint32_t pc);
void cdm16_gen_halt(uint16_t next_pc);

// Генерация кода для записи значения в регистр PC
void cdm16_gen_write_pc(TCGv_i32 value, uint32_t fault_pc);
void cdm16_gen_write_sp(TCGv_i32 value, uint32_t fault_pc);

void cdm16_gen_branch(unsigned code, uint16_t target, uint16_t next_pc,
                      uint32_t fault_pc);

/* Группа 011 */
void cdm16_gen_lsw(unsigned rd, int32_t imm);
void cdm16_gen_lsb(unsigned rd, int32_t imm);
void cdm16_gen_lssb(unsigned rd, int32_t imm);
void cdm16_gen_ssw(unsigned rd, int32_t imm);
void cdm16_gen_ssb(unsigned rd, int32_t imm);
void cdm16_gen_ldi_short(unsigned rd, int32_t imm);
void cdm16_gen_add_imm(unsigned rd, int32_t imm);
void cdm16_gen_cmp_imm(unsigned rd, int32_t imm);

/* Группа 001 */
void cdm16_gen_push_rd(unsigned rd);
void cdm16_gen_pop_rd(unsigned rd);
void cdm16_gen_ldi_16(unsigned rd, uint16_t imm16);
void cdm16_gen_jsrr(unsigned rd, uint32_t insn_pc);
void cdm16_gen_ldsp(unsigned rd);
void cdm16_gen_stsp(unsigned rd, uint32_t fault_pc);
void cdm16_gen_ldps(unsigned rd);
void cdm16_gen_stps(unsigned rd);
void cdm16_gen_ldpc(unsigned rd, uint32_t insn_pc);
void cdm16_gen_stpc(unsigned rd, uint32_t fault_pc);
void cdm16_gen_addsp(unsigned rd, uint32_t fault_pc);


/* Группа 01000 */
void cdm16_gen_mov(unsigned rs, unsigned rd);

/* Группа 01010 */
void cdm16_gen_ldw(unsigned rs, unsigned rd);
void cdm16_gen_ldb(unsigned rs, unsigned rd);
void cdm16_gen_ldsb(unsigned rs, unsigned rd);
void cdm16_gen_lcw(unsigned rs, unsigned rd);
void cdm16_gen_lcb(unsigned rs, unsigned rd);
void cdm16_gen_lcsb(unsigned rs, unsigned rd);
void cdm16_gen_stw(unsigned rs, unsigned rd);
void cdm16_gen_stb(unsigned rs, unsigned rd);
#endif