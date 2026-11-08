#include "qemu/osdep.h"

#include "tcg/tcg-op.h"
#include "translate.h"
#include "cpu.h"

void cdm16_gen_and(unsigned rs0, unsigned rs1, unsigned rd) {
    TCGv_i32 reg0 = cdm16_tcg_reg(rs0);
    TCGv_i32 reg1 = cdm16_tcg_reg(rs1);
    TCGv_i32 result = cdm16_tcg_reg(rd);

    tcg_gen_and_i32(result, reg0, reg1);
    tcg_gen_andi_i32(result, result, 0xffff);

    cdm16_gen_flags_logic(result);
}

void cdm16_gen_or(unsigned rs0, unsigned rs1, unsigned rd) {
    TCGv_i32 reg0 = cdm16_tcg_reg(rs0);
    TCGv_i32 reg1 = cdm16_tcg_reg(rs1);
    TCGv_i32 result = cdm16_tcg_reg(rd);

    tcg_gen_or_i32(result, reg0, reg1);
    tcg_gen_andi_i32(result, result, 0xffff);

    cdm16_gen_flags_logic(result);
}

void cdm16_gen_xor(unsigned rs0, unsigned rs1, unsigned rd) {
    TCGv_i32 reg0 = cdm16_tcg_reg(rs0);
    TCGv_i32 reg1 = cdm16_tcg_reg(rs1);
    TCGv_i32 result = cdm16_tcg_reg(rd);

    tcg_gen_xor_i32(result, reg0, reg1);
    tcg_gen_andi_i32(result, result, 0xffff);

    cdm16_gen_flags_logic(result);
}

void cdm16_gen_bic(unsigned rs0, unsigned rs1, unsigned rd) {
    TCGv_i32 reg0 = cdm16_tcg_reg(rs0);
    TCGv_i32 reg1 = cdm16_tcg_reg(rs1);
    TCGv_i32 result = cdm16_tcg_reg(rd);

    TCGv_i32 inverted = tcg_temp_new_i32();

    tcg_gen_not_i32(inverted, reg1);
    tcg_gen_and_i32(result, reg0, inverted);
    tcg_gen_andi_i32(result, result, 0xffff);

    cdm16_gen_flags_logic(result);
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
    cdm16_gen_flags_add(a, b, tcg_constant_i32(0), sum, result);

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
    cdm16_gen_flags_add(a, b, carry, sum, result);

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
    cdm16_gen_flags_sub(a, b, tcg_constant_i32(1), wide, result);

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
    cdm16_gen_flags_sub(a, b, carry, wide, result);

    /* Записываем результат */
    tcg_gen_mov_i32(cdm16_tcg_reg(rd), result);
}
