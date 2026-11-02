#include "qemu/osdep.h"

#include "tcg/tcg-op.h"
#include "translate.h"
#include "cpu.h"

static void gen_logic_flags(TCGv_i32 result) {
    TCGv_i32 z = tcg_temp_new_i32();
    TCGv_i32 n = tcg_temp_new_i32();
    TCGv_i32 ps = cdm16_tcg_ps();

    /* Z: результат равен нулю */
    tcg_gen_setcondi_i32(TCG_COND_EQ, z, result, 0);
    tcg_gen_shli_i32(z, z, 1);

    /* N: старший бит результата */
    tcg_gen_shri_i32(n, result, 15);
    tcg_gen_andi_i32(n, n, 1);

    /* Очищаем только Z и N */
    tcg_gen_andi_i32(ps, ps, 0xfffc);

    /* Записываем новые флаги */
    tcg_gen_or_i32(ps, ps, z);
    tcg_gen_or_i32(ps, ps, n);
}

static void gen_add_flags(TCGv_i32 a, TCGv_i32 b, TCGv_i32 sum,
                          TCGv_i32 result) {
    TCGv_i32 c = tcg_temp_new_i32();
    TCGv_i32 v = tcg_temp_new_i32();
    TCGv_i32 z = tcg_temp_new_i32();
    TCGv_i32 n = tcg_temp_new_i32();
    TCGv_i32 tmp = tcg_temp_new_i32();
    TCGv_i32 flags = tcg_temp_new_i32();

    /* C: перенос из 16-го бита */
    tcg_gen_shri_i32(c, sum, 16);
    tcg_gen_andi_i32(c, c, 1);

    /* V: знаковое переполнение */
    tcg_gen_xor_i32(tmp, a, b);
    tcg_gen_not_i32(tmp, tmp);

    tcg_gen_xor_i32(v, a, result);
    tcg_gen_and_i32(v, v, tmp);
    tcg_gen_shri_i32(v, v, 15);
    tcg_gen_andi_i32(v, v, 1);

    /* Z: результат равен нулю */
    tcg_gen_setcondi_i32(TCG_COND_EQ, z, result, 0);

    /* N: знак результата */
    tcg_gen_shri_i32(n, result, 15);
    tcg_gen_andi_i32(n, n, 1);

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

static void gen_sub_flags(TCGv_i32 a, TCGv_i32 b, TCGv_i32 wide,
                          TCGv_i32 result){
    TCGv_i32 c = tcg_temp_new_i32();
    TCGv_i32 v = tcg_temp_new_i32();
    TCGv_i32 z = tcg_temp_new_i32();
    TCGv_i32 n = tcg_temp_new_i32();

    TCGv_i32 tmp = tcg_temp_new_i32();
    TCGv_i32 flags = tcg_temp_new_i32();
    TCGv_i32 ps = cdm16_tcg_ps();

    /* C: отсутствие займа */
    tcg_gen_shri_i32(c, wide, 16);
    tcg_gen_andi_i32(c, c, 1);

    /* V: знаковое переполнение */
    tcg_gen_xor_i32(tmp, a, b);
    tcg_gen_xor_i32(v, a, result);
    tcg_gen_and_i32(v, v, tmp);
    tcg_gen_shri_i32(v, v, 15);
    tcg_gen_andi_i32(v, v, 1);

    /* Z: результат равен нулю */
    tcg_gen_setcondi_i32(TCG_COND_EQ, z, result, 0);

    /* N: знак результата */
    tcg_gen_shri_i32(n, result, 15);
    tcg_gen_andi_i32(n, n, 1);

    /* Собираем CVZN */
    tcg_gen_shli_i32(c, c, 3);
    tcg_gen_shli_i32(v, v, 2);
    tcg_gen_shli_i32(z, z, 1);

    tcg_gen_or_i32(flags, c, v);
    tcg_gen_or_i32(flags, flags, z);
    tcg_gen_or_i32(flags, flags, n);

    /* Обновляем только четыре младших бита PS */
    tcg_gen_andi_i32(ps, ps, 0xfff0);
    tcg_gen_or_i32(ps, ps, flags);
}

void cdm16_gen_and(unsigned rs0, unsigned rs1, unsigned rd) {
    TCGv_i32 reg0 = cdm16_tcg_reg(rs0);
    TCGv_i32 reg1 = cdm16_tcg_reg(rs1);
    TCGv_i32 result = cdm16_tcg_reg(rd);

    tcg_gen_and_i32(result, reg0, reg1);
    tcg_gen_andi_i32(result, result, 0xffff);

    gen_logic_flags(result);
}

void cdm16_gen_or(unsigned rs0, unsigned rs1, unsigned rd) {
    TCGv_i32 reg0 = cdm16_tcg_reg(rs0);
    TCGv_i32 reg1 = cdm16_tcg_reg(rs1);
    TCGv_i32 result = cdm16_tcg_reg(rd);

    tcg_gen_or_i32(result, reg0, reg1);
    tcg_gen_andi_i32(result, result, 0xffff);

    gen_logic_flags(result);
}

void cdm16_gen_xor(unsigned rs0, unsigned rs1, unsigned rd) {
    TCGv_i32 reg0 = cdm16_tcg_reg(rs0);
    TCGv_i32 reg1 = cdm16_tcg_reg(rs1);
    TCGv_i32 result = cdm16_tcg_reg(rd);

    tcg_gen_xor_i32(result, reg0, reg1);
    tcg_gen_andi_i32(result, result, 0xffff);

    gen_logic_flags(result);
}

void cdm16_gen_bic(unsigned rs0, unsigned rs1, unsigned rd) {
    TCGv_i32 reg0 = cdm16_tcg_reg(rs0);
    TCGv_i32 reg1 = cdm16_tcg_reg(rs1);
    TCGv_i32 result = cdm16_tcg_reg(rd);

    TCGv_i32 inverted = tcg_temp_new_i32();

    tcg_gen_not_i32(inverted, reg1);
    tcg_gen_and_i32(result, reg0, inverted);
    tcg_gen_andi_i32(result, result, 0xffff);

    gen_logic_flags(result);
}

void cdm16_gen_add(unsigned rs0, unsigned rs1, unsigned rd) {
    TCGv_i32 a = tcg_temp_new_i32();
    TCGv_i32 b = tcg_temp_new_i32();

    TCGv_i32 sum = tcg_temp_new_i32();
    TCGv_i32 result = tcg_temp_new_i32();

    /* Получаем исходные 16-битные операнды */
    tcg_gen_andi_i32(a, cdm16_tcg_reg(rs0), 0xffff);
    tcg_gen_andi_i32(b, cdm16_tcg_reg(rs1), 0xffff);

    /* Складываем, сохраняя бит переноса */
    tcg_gen_add_i32(sum, a, b);

    /* Ограничиваем результат 16 битами */
    tcg_gen_andi_i32(result, sum, 0xffff);

    /* Вычисляем и записываем флаги */
    gen_add_flags(a, b, sum, result);

    /* Записываем результат в rd */
    tcg_gen_mov_i32(cdm16_tcg_reg(rd), result);
}

void cdm16_gen_addc(unsigned rs0, unsigned rs1, unsigned rd) {
    TCGv_i32 a = tcg_temp_new_i32();
    TCGv_i32 b = tcg_temp_new_i32();
    TCGv_i32 carry = tcg_temp_new_i32();

    TCGv_i32 sum = tcg_temp_new_i32();
    TCGv_i32 result = tcg_temp_new_i32();

    /* Получаем исходные 16-битные операнды */
    tcg_gen_andi_i32(a, cdm16_tcg_reg(rs0), 0xffff);
    tcg_gen_andi_i32(b, cdm16_tcg_reg(rs1), 0xffff);

    /* Получаем значение флага переноса (C) */
    tcg_gen_shri_i32(carry, cdm16_tcg_ps(), 3);
    tcg_gen_andi_i32(carry, carry, 1);

    /* Складываем, сохраняя бит переноса */
    tcg_gen_add_i32(sum, a, b);
    tcg_gen_add_i32(sum, sum, carry);

    /* Ограничиваем результат 16 битами */
    tcg_gen_andi_i32(result, sum, 0xffff);

    /* Вычисляем и записываем флаги */
    gen_add_flags(a, b, sum, result);

    /* Записываем результат в rd */
    tcg_gen_mov_i32(cdm16_tcg_reg(rd), result);
}

void cdm16_gen_sub(unsigned rs0, unsigned rs1, unsigned rd) {
    TCGv_i32 a = tcg_temp_new_i32();
    TCGv_i32 b = tcg_temp_new_i32();

    TCGv_i32 b_not = tcg_temp_new_i32();
    TCGv_i32 wide = tcg_temp_new_i32();
    TCGv_i32 result = tcg_temp_new_i32();

    /* Получаем исходные 16-битные операнды */
    tcg_gen_andi_i32(a, cdm16_tcg_reg(rs0), 0xffff);
    tcg_gen_andi_i32(b, cdm16_tcg_reg(rs1), 0xffff);

    /* Выполняем вычитание */
    tcg_gen_xori_i32(b_not, b, 0xffff); // Получаем побитовое отрицание b
    tcg_gen_add_i32(wide, a, b_not);
    tcg_gen_addi_i32(wide, wide, 1); // Добавляем

    /* Ограничиваем результат 16 битами */
    tcg_gen_andi_i32(result, wide, 0xffff);

    /* Вычисляем и записываем флаги */
    gen_sub_flags(a, b, wide, result);

    /* Записываем результат в rd */
    tcg_gen_mov_i32(cdm16_tcg_reg(rd), result);
}

void cdm16_gen_subc(unsigned rs0, unsigned rs1, unsigned rd) {
    TCGv_i32 a = tcg_temp_new_i32();
    TCGv_i32 b = tcg_temp_new_i32();
    TCGv_i32 carry = tcg_temp_new_i32();

    TCGv_i32 b_not = tcg_temp_new_i32();
    TCGv_i32 wide = tcg_temp_new_i32();
    TCGv_i32 result = tcg_temp_new_i32();

    /* Получаем исходные 16-битные операнды */
    tcg_gen_andi_i32(a, cdm16_tcg_reg(rs0), 0xffff);
    tcg_gen_andi_i32(b, cdm16_tcg_reg(rs1), 0xffff);

    /* Получаем значение флага переноса (C) */
    tcg_gen_shri_i32(carry, cdm16_tcg_ps(), 3);
    tcg_gen_andi_i32(carry, carry, 1);

     /* wide = a + (~b & 0xffff) + C */
    tcg_gen_xori_i32(b_not, b, 0xffff);
    tcg_gen_add_i32(wide, a, b_not);
    tcg_gen_add_i32(wide, wide, carry);

    /* Ограничиваем результат 16 битами */
    tcg_gen_andi_i32(result, wide, 0xffff);

    /* Обновляем флаги */
    gen_sub_flags(a, b, wide, result);

    /* Записываем результат */
    tcg_gen_mov_i32(cdm16_tcg_reg(rd), result);
}