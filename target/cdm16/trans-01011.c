#include "qemu/osdep.h"

#include "tcg/tcg-op.h"
#include "translate.h"
#include "cpu.h"

static void gen_logic_flags(TCGv_i32 result) {
    TCGv_i32 z = tcg_temp_new_i32();
    TCGv_i32 n = tcg_temp_new_i32();
    TCGv_i32 ps = cdm16_tcg_ps();

    /* Z: результат равен нулю; N: бит 15 результата. */
    tcg_gen_setcondi_i32(TCG_COND_EQ, z, result, 0);
    tcg_gen_shli_i32(z, z, 1);
    tcg_gen_shri_i32(n, result, 15);
    tcg_gen_andi_i32(n, n, 1);

    /* Обновляем только Z и N, сбрасываем C, V и остальные биты PS сохраняем. */
    tcg_gen_andi_i32(ps, ps, 0xfff0);
    tcg_gen_or_i32(ps, ps, z);
    tcg_gen_or_i32(ps, ps, n);
}

static void gen_neg_flags(TCGv_i32 src, TCGv_i32 result) {
    TCGv_i32 c = tcg_temp_new_i32();
    TCGv_i32 v = tcg_temp_new_i32();
    TCGv_i32 z = tcg_temp_new_i32();
    TCGv_i32 n = tcg_temp_new_i32();
    TCGv_i32 flags = tcg_temp_new_i32();
    TCGv_i32 ps = cdm16_tcg_ps();

    /* Флаги для операции отрицания: C - перенос, V - переполнение, Z - ноль, N - знак. */
    tcg_gen_setcondi_i32(TCG_COND_EQ, c, src, 0);
    tcg_gen_setcondi_i32(TCG_COND_EQ, v, src, 0x8000);
    tcg_gen_setcondi_i32(TCG_COND_EQ, z, result, 0);
    tcg_gen_shri_i32(n, result, 15);

    tcg_gen_shli_i32(c, c, 3);
    tcg_gen_shli_i32(v, v, 2);
    tcg_gen_shli_i32(z, z, 1);
    tcg_gen_or_i32(flags, c, v);
    tcg_gen_or_i32(flags, flags, z);
    tcg_gen_or_i32(flags, flags, n);

    /* Обновляем флаги в регистре PS. */
    tcg_gen_andi_i32(ps, ps, 0xfff0);
    tcg_gen_or_i32(ps, ps, flags);
}

void cdm16_gen_neg(unsigned rs, unsigned rd) {
    TCGv_i32 src = tcg_temp_new_i32();
    TCGv_i32 result = tcg_temp_new_i32();

    /* Сохраняем исходное значение, в том числе при rs == rd. */
    tcg_gen_andi_i32(src, cdm16_tcg_reg(rs), 0xffff);
    tcg_gen_neg_i32(result, src);
    tcg_gen_andi_i32(result, result, 0xffff);

    gen_neg_flags(src, result);
    tcg_gen_mov_i32(cdm16_tcg_reg(rd), result);
}

void cdm16_gen_not(unsigned rs, unsigned rd) {
    TCGv_i32 result = tcg_temp_new_i32();

    tcg_gen_not_i32(result, cdm16_tcg_reg(rs));
    tcg_gen_andi_i32(result, result, 0xffff);

    gen_logic_flags(result);
    tcg_gen_mov_i32(cdm16_tcg_reg(rd), result);
}

void cdm16_gen_sxt(unsigned rs, unsigned rd) {
    TCGv_i32 result = tcg_temp_new_i32();

    /* Расширяем знак младшего байта до 16 бит. */
    tcg_gen_ext8s_i32(result, cdm16_tcg_reg(rs));
    tcg_gen_andi_i32(result, result, 0xffff);

    gen_logic_flags(result);
    tcg_gen_mov_i32(cdm16_tcg_reg(rd), result);
}

void cdm16_gen_scl(unsigned rs, unsigned rd) {
    TCGv_i32 result = tcg_temp_new_i32();

    /* Очищаем старший байт 16-битного значения. */
    tcg_gen_andi_i32(result, cdm16_tcg_reg(rs), 0xff);

    gen_logic_flags(result);
    tcg_gen_mov_i32(cdm16_tcg_reg(rd), result);
}
