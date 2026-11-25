#include "qemu/osdep.h"

#include "cpu.h"
#include "tcg/tcg-op.h"
#include "translate.h"

#define CDM16_ARITH_FLAGS \
    (CDM16_PSR_C | CDM16_PSR_V | CDM16_PSR_Z | CDM16_PSR_N)

/* Inputs c and v are booleans; only flags selected by mask are updated. */
static void gen_flags(TCGv_i32 result, TCGv_i32 c, TCGv_i32 v,
                      uint32_t mask)
{
    TCGv_i32 value = tcg_temp_new_i32();
    TCGv_i32 c_bit = tcg_temp_new_i32();
    TCGv_i32 v_bit = tcg_temp_new_i32();
    TCGv_i32 z_bit = tcg_temp_new_i32();
    TCGv_i32 n_bit = tcg_temp_new_i32();
    TCGv_i32 flags = tcg_temp_new_i32();
    TCGv_i32 ps = cdm16_tcg_ps();

    tcg_gen_andi_i32(value, result, 0xffff);
    tcg_gen_andi_i32(c_bit, c, 1);
    tcg_gen_shli_i32(c_bit, c_bit, 3);
    tcg_gen_andi_i32(v_bit, v, 1);
    tcg_gen_shli_i32(v_bit, v_bit, 2);
    tcg_gen_setcondi_i32(TCG_COND_EQ, z_bit, value, 0);
    tcg_gen_shli_i32(z_bit, z_bit, 1);
    tcg_gen_shri_i32(n_bit, value, 15);

    tcg_gen_or_i32(flags, c_bit, v_bit);
    tcg_gen_or_i32(flags, flags, z_bit);
    tcg_gen_or_i32(flags, flags, n_bit);
    tcg_gen_andi_i32(flags, flags, mask);
    tcg_gen_andi_i32(ps, ps, 0xffff & ~mask);
    tcg_gen_or_i32(ps, ps, flags);
}

/* Match cocoemu Alu.checkV(), including its unnormalized second operand. */
static void gen_overflow(TCGv_i32 v, TCGv_i32 a, TCGv_i32 b_eff,
                         TCGv_i32 result)
{
    TCGv_i32 a_negative = tcg_temp_new_i32();
    TCGv_i32 b_negative = tcg_temp_new_i32();
    TCGv_i32 r_negative = tcg_temp_new_i32();
    TCGv_i32 same_sign = tcg_temp_new_i32();

    tcg_gen_setcondi_i32(TCG_COND_GTU, a_negative, a, 0x7fff);
    tcg_gen_setcondi_i32(TCG_COND_GTU, b_negative, b_eff, 0x7fff);
    tcg_gen_setcondi_i32(TCG_COND_GTU, r_negative, result, 0x7fff);
    tcg_gen_xor_i32(same_sign, a_negative, b_negative);
    tcg_gen_xori_i32(same_sign, same_sign, 1);
    tcg_gen_xor_i32(v, a_negative, r_negative);
    tcg_gen_and_i32(v, v, same_sign);
}

static void gen_arithmetic_flags(TCGv_i32 a, TCGv_i32 b_eff,
                                 TCGv_i32 wide, TCGv_i32 result)
{
    TCGv_i32 c = tcg_temp_new_i32();
    TCGv_i32 v = tcg_temp_new_i32();

    tcg_gen_shri_i32(c, wide, 16);
    gen_overflow(v, a, b_eff, result);
    gen_flags(result, c, v, CDM16_ARITH_FLAGS);
}

void cdm16_gen_flags_add(TCGv_i32 a, TCGv_i32 b, TCGv_i32 carry,
                         TCGv_i32 wide, TCGv_i32 result)
{
    TCGv_i32 b_eff = tcg_temp_new_i32();

    /* Do not mask b + carry: the reference compares it against 0x7fff. */
    tcg_gen_add_i32(b_eff, b, carry);
    gen_arithmetic_flags(a, b_eff, wide, result);
}

void cdm16_gen_flags_sub(TCGv_i32 a, TCGv_i32 b, TCGv_i32 carry,
                         TCGv_i32 wide, TCGv_i32 result)
{
    TCGv_i32 b_eff = tcg_temp_new_i32();

    /* SUB/CMP use carry=1; SUBC uses the captured incoming carry. */
    tcg_gen_xori_i32(b_eff, b, 0xffff);
    tcg_gen_add_i32(b_eff, b_eff, carry);
    gen_arithmetic_flags(a, b_eff, wide, result);
}

void cdm16_gen_flags_neg(TCGv_i32 src, TCGv_i32 result)
{
    TCGv_i32 c = tcg_temp_new_i32();
    TCGv_i32 v = tcg_temp_new_i32();

    tcg_gen_setcondi_i32(TCG_COND_EQ, c, src, 0);
    tcg_gen_setcondi_i32(TCG_COND_EQ, v, src, 0x8000);
    gen_flags(result, c, v, CDM16_ARITH_FLAGS);
}

void cdm16_gen_flags_zn(TCGv_i32 result)
{
    gen_flags(result, tcg_constant_i32(0), tcg_constant_i32(0),
              CDM16_PSR_Z | CDM16_PSR_N);
}

void cdm16_gen_flags_logic(TCGv_i32 result)
{
    gen_flags(result, tcg_constant_i32(0), tcg_constant_i32(0),
              CDM16_ARITH_FLAGS);
}

void cdm16_gen_flags_shift(TCGv_i32 result, TCGv_i32 carry)
{
    gen_flags(result, carry, tcg_constant_i32(0), CDM16_ARITH_FLAGS);
}
