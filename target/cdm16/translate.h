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
void cdm16_gen_wait(uint16_t next_pc);

// Генерация кода для записи значения в регистр PC
void cdm16_gen_write_pc(TCGv_i32 value, uint32_t fault_pc);
void cdm16_gen_write_sp(TCGv_i32 value, uint32_t fault_pc);

void cdm16_gen_branch(unsigned code, uint16_t target, uint16_t next_pc,
                      uint32_t fault_pc);

/* Группа 00000 */
void cdm16_gen_ei(void);
void cdm16_gen_di(void);
void cdm16_gen_rti(uint32_t insn_pc);
void cdm16_gen_jsr_label(uint16_t target, uint32_t insn_pc);
void cdm16_gen_pupc(uint32_t insn_pc);
void cdm16_gen_popc(uint32_t insn_pc);
void cdm16_gen_pusp(uint32_t insn_pc);
void cdm16_gen_posp(uint32_t insn_pc);
void cdm16_gen_pups(uint32_t insn_pc);
void cdm16_gen_pops(uint32_t insn_pc);

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
void cdm16_gen_move(unsigned rs, unsigned rd);

/* Группа 01010 */
void cdm16_gen_ldw(unsigned rs, unsigned rd);
void cdm16_gen_ldb(unsigned rs, unsigned rd);
void cdm16_gen_ldsb(unsigned rs, unsigned rd);
void cdm16_gen_lcw(unsigned rs, unsigned rd);
void cdm16_gen_lcb(unsigned rs, unsigned rd);
void cdm16_gen_lcsb(unsigned rs, unsigned rd);
void cdm16_gen_stw(unsigned rs, unsigned rd);
void cdm16_gen_stb(unsigned rs, unsigned rd);

/* Группа 100*/
void cdm16_gen_int_imm9(uint16_t vector, uint32_t insn_pc);
void cdm16_gen_reset_imm9(uint16_t vector, uint32_t insn_pc);
void cdm16_gen_push_imm9(int32_t imm, uint32_t insn_pc);
void cdm16_gen_addsp_imm9(int32_t imm, uint32_t insn_pc);
void cdm16_gen_jsr_imm9(int32_t imm, uint32_t insn_pc);

/* Группа 1011 */
void cdm16_gen_and(unsigned rs0, unsigned rs1, unsigned rd);
void cdm16_gen_or(unsigned rs0, unsigned rs1, unsigned rd);
void cdm16_gen_xor(unsigned rs0, unsigned rs1, unsigned rd);
void cdm16_gen_bic(unsigned rs0, unsigned rs1, unsigned rd);
void cdm16_gen_add(unsigned rs0, unsigned rs1, unsigned rd);
void cdm16_gen_addc(unsigned rs0, unsigned rs1, unsigned rd);
void cdm16_gen_sub(unsigned rs0, unsigned rs1, unsigned rd);
void cdm16_gen_subc(unsigned rs0, unsigned rs1, unsigned rd);

/* Группа 1010 */
void cdm16_gen_ldw_add(unsigned rs1, unsigned rs2, unsigned rd);
void cdm16_gen_ldb_add(unsigned rs1, unsigned rs2, unsigned rd);
void cdm16_gen_ldsb_add(unsigned rs1, unsigned rs2, unsigned rd);
void cdm16_gen_stw_add(unsigned rs1, unsigned rs2, unsigned rd);
void cdm16_gen_stb_add(unsigned rs1, unsigned rs2, unsigned rd);
void cdm16_gen_lcw_add(unsigned rs1, unsigned rs2, unsigned rd);
void cdm16_gen_lcb_add(unsigned rs1, unsigned rs2, unsigned rd);
void cdm16_gen_lcsb_add(unsigned rs1, unsigned rs2, unsigned rd);

/* Группа 01011 */
void cdm16_gen_neg(unsigned rs, unsigned rd);
void cdm16_gen_not(unsigned rs, unsigned rd);
void cdm16_gen_sxt(unsigned rs, unsigned rd);
void cdm16_gen_scl(unsigned rs, unsigned rd);

/* Группа 01001 */
void cdm16_gen_bit(unsigned rs, unsigned rd);
void cdm16_gen_cmp(unsigned rs, unsigned rd);

/* Группа 0001 */
void cdm16_gen_shl(unsigned rs, unsigned rd, int32_t shift_val);
void cdm16_gen_shr(unsigned rs, unsigned rd, int32_t shift_val);
void cdm16_gen_shra(unsigned rs, unsigned rd, int32_t shift_val);
void cdm16_gen_rol(unsigned rs, unsigned rd, int32_t shift_val);
void cdm16_gen_ror(unsigned rs, unsigned rd, int32_t shift_val);
void cdm16_gen_rcl(unsigned rs, unsigned rd, int32_t shift_val);
void cdm16_gen_rcr(unsigned rs, unsigned rd, int32_t shift_val);

#endif