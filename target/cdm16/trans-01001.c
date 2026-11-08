#include "qemu/osdep.h"

#include "tcg/tcg-op.h"
#include "translate.h"
#include "cpu.h"

void cdm16_gen_bit(unsigned rs, unsigned rd) {
    TCGv_i32 result = tcg_temp_new_i32();

    /* BIT меняет только флаги, сохраняя оба операнда. */
    tcg_gen_and_i32(result, cdm16_tcg_reg(rs), cdm16_tcg_reg(rd));
    tcg_gen_andi_i32(result, result, 0xffff);
    cdm16_gen_flags_zn(result);
}

void cdm16_gen_cmp(unsigned rs, unsigned rd) {
    TCGv_i32 a = tcg_temp_new_i32();
    TCGv_i32 b = tcg_temp_new_i32();
    TCGv_i32 b_not = tcg_temp_new_i32();
    TCGv_i32 wide = tcg_temp_new_i32();
    TCGv_i32 result = tcg_temp_new_i32();

    tcg_gen_andi_i32(a, cdm16_tcg_reg(rs), 0xffff);
    tcg_gen_andi_i32(b, cdm16_tcg_reg(rd), 0xffff);

    /* rs - rd с сохранением переноса, как в SUB. */
    tcg_gen_xori_i32(b_not, b, 0xffff);
    tcg_gen_add_i32(wide, a, b_not);
    tcg_gen_addi_i32(wide, wide, 1);
    tcg_gen_andi_i32(result, wide, 0xffff);

    /* Результат не записывается в регистры. */
    cdm16_gen_flags_sub(a, b, tcg_constant_i32(1), wide, result);
}
