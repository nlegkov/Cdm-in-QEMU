#include "qemu/osdep.h"

#include "tcg/tcg-op.h"
#include "translate.h"
#include "cpu.h"

static void gen_shift_flags(TCGv_i32 result, TCGv_i32 carry) {
    TCGv_i32 z = tcg_temp_new_i32();
    TCGv_i32 n = tcg_temp_new_i32();
    TCGv_i32 c = tcg_temp_new_i32();
    TCGv_i32 ps = cdm16_tcg_ps();

    /* C: последний вытесненный бит; Z,N: 16-битный результат. */
    tcg_gen_shli_i32(c, carry, 3);
    tcg_gen_setcondi_i32(TCG_COND_EQ, z, result, 0);
    tcg_gen_shli_i32(z, z, 1);
    tcg_gen_shri_i32(n, result, 15);

    /* Обновляем C,Z,N и сбрасываем V, как в эталонном эмуляторе. */
    tcg_gen_andi_i32(ps, ps, 0xfff0);
    tcg_gen_or_i32(ps, ps, c);
    tcg_gen_or_i32(ps, ps, z);
    tcg_gen_or_i32(ps, ps, n);
}

void cdm16_gen_shl(unsigned rs, unsigned rd, int32_t shift_val) {
    TCGv_i32 src = tcg_temp_new_i32();
    TCGv_i32 result = tcg_temp_new_i32();
    TCGv_i32 carry = tcg_temp_new_i32();

    g_assert(shift_val >= 1 && shift_val <= 8);

    /* Сохраняем источник до записи rd, в том числе при rs == rd. */
    tcg_gen_andi_i32(src, cdm16_tcg_reg(rs), 0xffff);
    tcg_gen_shri_i32(carry, src, 16 - shift_val);
    tcg_gen_andi_i32(carry, carry, 1);

    tcg_gen_shli_i32(result, src, shift_val);
    tcg_gen_andi_i32(result, result, 0xffff);

    gen_shift_flags(result, carry);
    tcg_gen_mov_i32(cdm16_tcg_reg(rd), result);
}

void cdm16_gen_shr(unsigned rs, unsigned rd, int32_t shift_val) {
    TCGv_i32 src = tcg_temp_new_i32();
    TCGv_i32 result = tcg_temp_new_i32();
    TCGv_i32 carry = tcg_temp_new_i32();

    g_assert(shift_val >= 1 && shift_val <= 8);

    /* Сохраняем источник до записи rd, в том числе при rs == rd. */
    tcg_gen_andi_i32(src, cdm16_tcg_reg(rs), 0xffff);
    tcg_gen_shri_i32(carry, src, shift_val - 1);
    tcg_gen_andi_i32(carry, carry, 1);

    tcg_gen_shri_i32(result, src, shift_val);
    tcg_gen_andi_i32(result, result, 0xffff);

    gen_shift_flags(result, carry);
    tcg_gen_mov_i32(cdm16_tcg_reg(rd), result);
}

void cdm16_gen_shra(unsigned rs, unsigned rd, int32_t shift_val) {
    TCGv_i32 src = tcg_temp_new_i32();
    TCGv_i32 result = tcg_temp_new_i32();
    TCGv_i32 carry = tcg_temp_new_i32();

    g_assert(shift_val >= 1 && shift_val <= 8);

    /* Сохраняем источник до записи rd, в том числе при rs == rd. */
    tcg_gen_andi_i32(src, cdm16_tcg_reg(rs), 0xffff);
    tcg_gen_shri_i32(carry, src, shift_val - 1);
    tcg_gen_andi_i32(carry, carry, 1);

    tcg_gen_ext16s_i32(result, src);
    tcg_gen_sari_i32(result, result, shift_val);
    tcg_gen_andi_i32(result, result, 0xffff);

    gen_shift_flags(result, carry);
    tcg_gen_mov_i32(cdm16_tcg_reg(rd), result);
}

void cdm16_gen_rol(unsigned rs, unsigned rd, int32_t shift_val) {
    TCGv_i32 src = tcg_temp_new_i32();
    TCGv_i32 result = tcg_temp_new_i32();
    TCGv_i32 carry = tcg_temp_new_i32();
    TCGv_i32 tmp = tcg_temp_new_i32();

    g_assert(shift_val >= 1 && shift_val <= 8);

    /* Сохраняем источник до записи rd, в том числе при rs == rd. */
    tcg_gen_andi_i32(src, cdm16_tcg_reg(rs), 0xffff);
    tcg_gen_shri_i32(carry, src, 16 - shift_val);
    tcg_gen_andi_i32(carry, carry, 1);

    /* Циклический сдвиг внутри 16-битного слова. */
    tcg_gen_shli_i32(result, src, shift_val);
    tcg_gen_shri_i32(tmp, src, 16 - shift_val);
    tcg_gen_or_i32(result, result, tmp);
    tcg_gen_andi_i32(result, result, 0xffff);

    gen_shift_flags(result, carry);
    tcg_gen_mov_i32(cdm16_tcg_reg(rd), result);
}

void cdm16_gen_ror(unsigned rs, unsigned rd, int32_t shift_val) {
    TCGv_i32 src = tcg_temp_new_i32();
    TCGv_i32 result = tcg_temp_new_i32();
    TCGv_i32 carry = tcg_temp_new_i32();
    TCGv_i32 tmp = tcg_temp_new_i32();

    g_assert(shift_val >= 1 && shift_val <= 8);

    /* Сохраняем источник до записи rd, в том числе при rs == rd. */
    tcg_gen_andi_i32(src, cdm16_tcg_reg(rs), 0xffff);
    tcg_gen_shri_i32(carry, src, shift_val - 1);
    tcg_gen_andi_i32(carry, carry, 1);

    /* Циклический сдвиг внутри 16-битного слова. */
    tcg_gen_shri_i32(result, src, shift_val);
    tcg_gen_shli_i32(tmp, src, 16 - shift_val);
    tcg_gen_or_i32(result, result, tmp);
    tcg_gen_andi_i32(result, result, 0xffff);

    gen_shift_flags(result, carry);
    tcg_gen_mov_i32(cdm16_tcg_reg(rd), result);
}

void cdm16_gen_rcl(unsigned rs, unsigned rd, int32_t shift_val) {
    TCGv_i32 src = tcg_temp_new_i32();
    TCGv_i32 result = tcg_temp_new_i32();
    TCGv_i32 carry = tcg_temp_new_i32();
    TCGv_i32 tmp = tcg_temp_new_i32();
    TCGv_i32 ring = tcg_temp_new_i32();

    g_assert(shift_val >= 1 && shift_val <= 8);

    /* Сохраняем источник до записи rd, в том числе при rs == rd. */
    tcg_gen_andi_i32(src, cdm16_tcg_reg(rs), 0xffff);
    tcg_gen_shri_i32(carry, src, 16 - shift_val);
    tcg_gen_andi_i32(carry, carry, 1);

    /* Вращаем 17-битное кольцо C:src с исходным значением C. */
    tcg_gen_andi_i32(ring, cdm16_tcg_ps(), CDM16_PSR_C);
    tcg_gen_shli_i32(ring, ring, 13);
    tcg_gen_or_i32(ring, ring, src);
    tcg_gen_shli_i32(result, ring, shift_val);
    tcg_gen_shri_i32(tmp, ring, 17 - shift_val);
    tcg_gen_or_i32(result, result, tmp);
    tcg_gen_andi_i32(result, result, 0xffff);

    gen_shift_flags(result, carry);
    tcg_gen_mov_i32(cdm16_tcg_reg(rd), result);
}

void cdm16_gen_rcr(unsigned rs, unsigned rd, int32_t shift_val) {
    TCGv_i32 src = tcg_temp_new_i32();
    TCGv_i32 result = tcg_temp_new_i32();
    TCGv_i32 carry = tcg_temp_new_i32();
    TCGv_i32 tmp = tcg_temp_new_i32();
    TCGv_i32 ring = tcg_temp_new_i32();

    g_assert(shift_val >= 1 && shift_val <= 8);

    /* Сохраняем источник до записи rd, в том числе при rs == rd. */
    tcg_gen_andi_i32(src, cdm16_tcg_reg(rs), 0xffff);
    tcg_gen_shri_i32(carry, src, shift_val - 1);
    tcg_gen_andi_i32(carry, carry, 1);

    /* Вращаем 17-битное кольцо C:src с исходным значением C. */
    tcg_gen_andi_i32(ring, cdm16_tcg_ps(), CDM16_PSR_C);
    tcg_gen_shli_i32(ring, ring, 13);
    tcg_gen_or_i32(ring, ring, src);
    tcg_gen_shri_i32(result, ring, shift_val);
    tcg_gen_shli_i32(tmp, ring, 17 - shift_val);
    tcg_gen_or_i32(result, result, tmp);
    tcg_gen_andi_i32(result, result, 0xffff);

    gen_shift_flags(result, carry);
    tcg_gen_mov_i32(cdm16_tcg_reg(rd), result);
}
