#include "qemu/osdep.h"

#include "tcg/tcg-op.h"
#include "translate.h"
#include "cpu.h"

void cdm16_gen_neg(unsigned rs, unsigned rd) {
    TCGv_i32 src = tcg_temp_new_i32();
    TCGv_i32 result = tcg_temp_new_i32();

    /* Сохраняем исходное значение, в том числе при rs == rd. */
    tcg_gen_andi_i32(src, cdm16_tcg_reg(rs), 0xffff);
    tcg_gen_neg_i32(result, src);
    tcg_gen_andi_i32(result, result, 0xffff);

    cdm16_gen_flags_neg(src, result);
    tcg_gen_mov_i32(cdm16_tcg_reg(rd), result);
}

void cdm16_gen_not(unsigned rs, unsigned rd) {
    TCGv_i32 result = tcg_temp_new_i32();

    tcg_gen_not_i32(result, cdm16_tcg_reg(rs));
    tcg_gen_andi_i32(result, result, 0xffff);

    cdm16_gen_flags_logic(result);
    tcg_gen_mov_i32(cdm16_tcg_reg(rd), result);
}

void cdm16_gen_sxt(unsigned rs, unsigned rd) {
    TCGv_i32 result = tcg_temp_new_i32();

    /* Расширяем знак младшего байта до 16 бит. */
    tcg_gen_ext8s_i32(result, cdm16_tcg_reg(rs));
    tcg_gen_andi_i32(result, result, 0xffff);

    cdm16_gen_flags_logic(result);
    tcg_gen_mov_i32(cdm16_tcg_reg(rd), result);
}

void cdm16_gen_scl(unsigned rs, unsigned rd) {
    TCGv_i32 result = tcg_temp_new_i32();

    /* Очищаем старший байт 16-битного значения. */
    tcg_gen_andi_i32(result, cdm16_tcg_reg(rs), 0xff);

    cdm16_gen_flags_logic(result);
    tcg_gen_mov_i32(cdm16_tcg_reg(rd), result);
}
