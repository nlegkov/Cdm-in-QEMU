#include "qemu/osdep.h"
#include "tcg/tcg-op.h"
#include "translate.h"

void cdm16_gen_lsw(unsigned rd, int32_t imm) {
    TCGv_i32 addr = tcg_temp_new_i32();

    /* addr = fp + imm * 2 */
    tcg_gen_addi_i32(addr, cdm16_tcg_reg(7), imm * 2);
    /* Ограничиваем адрес 16 битами */
    tcg_gen_andi_i32(addr, addr, 0xffff);

    /* rd = memory[addr] */
    tcg_gen_qemu_ld_i32(cdm16_tcg_reg(rd), addr, 0, MO_LEUW);
}

void cdm16_gen_lsb(unsigned rd, int32_t imm) {
    TCGv_i32 addr = tcg_temp_new_i32();

    /* addr = fp + imm */
    tcg_gen_addi_i32(addr, cdm16_tcg_reg(7), imm);

    /* Ограничиваем адрес 16 битами */
    tcg_gen_andi_i32(addr, addr, 0xffff);

    /* rd = memory[addr] */
    tcg_gen_qemu_ld_i32(cdm16_tcg_reg(rd), addr, 0, MO_UB);
}

void cdm16_gen_lssb(unsigned rd, int32_t imm) {
    TCGv_i32 addr = tcg_temp_new_i32();
    TCGv_i32 reg = cdm16_tcg_reg(rd);

    tcg_gen_addi_i32(addr, cdm16_tcg_reg(7), imm);
    tcg_gen_andi_i32(addr, addr, 0xffff);

    tcg_gen_qemu_ld_i32(reg, addr, 0, MO_SB);

    tcg_gen_andi_i32(reg, reg, 0xffff);
}

void cdm16_gen_ssw(unsigned rd, int32_t imm) {
    TCGv_i32 addr = tcg_temp_new_i32();

    /* addr = fp + imm * 2 */
    tcg_gen_addi_i32(addr, cdm16_tcg_reg(7), imm * 2);

    tcg_gen_andi_i32(addr, addr, 0xffff);

    /* memory[addr] = rd */
    tcg_gen_qemu_st_i32(cdm16_tcg_reg(rd), addr, 0, MO_LEUW);
}

void cdm16_gen_ssb(unsigned rd, int32_t imm) {
    TCGv_i32 addr = tcg_temp_new_i32();

    /* addr = fp + imm * 2 */
    tcg_gen_addi_i32(addr, cdm16_tcg_reg(7), imm);

    tcg_gen_andi_i32(addr, addr, 0xffff);

    /* memory[addr] = rd */
    tcg_gen_qemu_st_i32(cdm16_tcg_reg(rd), addr, 0, MO_UB);
}

void cdm16_gen_ldi_short(unsigned rd, int32_t imm) {
    tcg_gen_movi_i32(cdm16_tcg_reg(rd), (uint16_t)imm);
}

/* Обновление флагов C, V, Z, N после сложения */
static void gen_add_flags(TCGv_i32 a, uint16_t b, TCGv_i32 sum,
                          TCGv_i32 result) {
    TCGv_i32 c = tcg_temp_new_i32();
    TCGv_i32 v = tcg_temp_new_i32();
    TCGv_i32 z = tcg_temp_new_i32();
    TCGv_i32 n = tcg_temp_new_i32();
    TCGv_i32 tmp = tcg_temp_new_i32();
    TCGv_i32 flags = tcg_temp_new_i32();

    /* C: перенос из 16-го бита */
    tcg_gen_shri_i32(c, sum, 16);

    /* V: знаковое переполнение */
    tcg_gen_xori_i32(tmp, a, b);
    tcg_gen_not_i32(tmp, tmp);

    tcg_gen_xor_i32(v, a, result);
    tcg_gen_and_i32(v, v, tmp);
    tcg_gen_shri_i32(v, v, 15);
    tcg_gen_andi_i32(v, v, 1);

    /* Z: результат равен нулю */
    tcg_gen_setcondi_i32(TCG_COND_EQ, z, result, 0);

    /* N: знак результата */
    tcg_gen_shri_i32(n, result, 15);

    /* Собираем флаги: C V Z N */
    tcg_gen_shli_i32(c, c, 3);
    tcg_gen_shli_i32(v, v, 2);
    tcg_gen_shli_i32(z, z, 1);

    tcg_gen_or_i32(flags, c, v);
    tcg_gen_or_i32(flags, flags, z);
    tcg_gen_or_i32(flags, flags, n);

    /* Обновляем только младшие 4 бита PS */
    TCGv_i32 ps = cdm16_tcg_ps();

    tcg_gen_andi_i32(ps, ps, 0xfff0);
    tcg_gen_or_i32(ps, ps, flags);
}

/* ADD rd, imm6 */
void cdm16_gen_add_imm(unsigned rd, int32_t imm) {
    TCGv_i32 reg = cdm16_tcg_reg(rd);

    TCGv_i32 a = tcg_temp_new_i32();
    TCGv_i32 sum = tcg_temp_new_i32();
    TCGv_i32 result = tcg_temp_new_i32();

    uint16_t b = (uint16_t)imm;

    /* Берём 16-битное значение регистра */
    tcg_gen_andi_i32(a, reg, 0xffff);

    /* Выполняем сложение с сохранением переноса */
    tcg_gen_addi_i32(sum, a, b);

    /* Ограничиваем результат 16 битами */
    tcg_gen_andi_i32(result, sum, 0xffff);

    /* Обновляем флаги */
    gen_add_flags(a, b, sum, result);

    /* Записываем результат в rd */
    tcg_gen_mov_i32(reg, result);
}

static void gen_sub_flags(TCGv_i32 a, uint16_t b, TCGv_i32 result) {
    TCGv_i32 c = tcg_temp_new_i32();
    TCGv_i32 v = tcg_temp_new_i32();
    TCGv_i32 z = tcg_temp_new_i32();
    TCGv_i32 n = tcg_temp_new_i32();
    TCGv_i32 tmp = tcg_temp_new_i32();
    TCGv_i32 flags = tcg_temp_new_i32();

    /* C: вычитание без займа */
    tcg_gen_setcondi_i32(TCG_COND_GEU, c, a, b);

    /* V: знаковое переполнение */
    tcg_gen_xori_i32(tmp, a, b);
    tcg_gen_xor_i32(v, a, result);
    tcg_gen_and_i32(v, v, tmp);
    tcg_gen_shri_i32(v, v, 15);
    tcg_gen_andi_i32(v, v, 1);

    /* Z: результат равен нулю */
    tcg_gen_setcondi_i32(TCG_COND_EQ, z, result, 0);

    /* N: знак результата */
    tcg_gen_shri_i32(n, result, 15);

    /* Собираем CVZN */
    tcg_gen_shli_i32(c, c, 3);
    tcg_gen_shli_i32(v, v, 2);
    tcg_gen_shli_i32(z, z, 1);

    tcg_gen_or_i32(flags, c, v);
    tcg_gen_or_i32(flags, flags, z);
    tcg_gen_or_i32(flags, flags, n);

    /* Обновляем младшие четыре бита PS */
    TCGv_i32 ps = cdm16_tcg_ps();

    tcg_gen_andi_i32(ps, ps, 0xfff0);
    tcg_gen_or_i32(ps, ps, flags);
}

void cdm16_gen_cmp_imm(unsigned rd, int32_t imm) {
    TCGv_i32 a = tcg_temp_new_i32();
    TCGv_i32 result = tcg_temp_new_i32();

    uint16_t b = (uint16_t)imm;

    /* Получаем значение регистра */
    tcg_gen_andi_i32(a, cdm16_tcg_reg(rd), 0xffff);
    /* result = a - b */
    tcg_gen_subi_i32(result, a, b);

    /* Ограничиваем результат 16 битами */
    tcg_gen_andi_i32(result, result, 0xffff);

    /* Вычисляем флаги */
    gen_sub_flags(a, b, result);
}