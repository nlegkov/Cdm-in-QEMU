#ifndef CDM16_TRANSLATE_H
#define CDM16_TRANSLATE_H

#include <stdint.h>
#include "tcg/tcg.h"

/* Доступ к TCG-регистрам */
TCGv_i32 cdm16_tcg_reg(unsigned reg);
TCGv_i32 cdm16_tcg_sp(void);
TCGv_i32 cdm16_tcg_pc(void);
TCGv_i32 cdm16_tcg_ps(void);

void cdm16_gen_exception(unsigned exception, uint32_t pc);
void cdm16_gen_halt(uint16_t next_pc);


/* Группа 011 */
void cdm16_gen_lsw(unsigned rd, int32_t imm);
void cdm16_gen_lsb(unsigned rd, int32_t imm);
void cdm16_gen_lssb(unsigned rd, int32_t imm);
void cdm16_gen_ssw(unsigned rd, int32_t imm);
void cdm16_gen_ssb(unsigned rd, int32_t imm);
void cdm16_gen_ldi_short(unsigned rd, int32_t imm);
void cdm16_gen_add_imm(unsigned rd, int32_t imm);
void cdm16_gen_cmp_imm(unsigned rd, int32_t imm);
#endif