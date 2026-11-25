#include "qemu/osdep.h"

#include "cpu.h"
#include "translate.h"
#include "tcg/tcg-op.h"

void cdm16_gen_move(unsigned rs, unsigned rd)
{
    TCGv_i32 result = tcg_temp_new_i32();

    tcg_gen_andi_i32(result, cdm16_tcg_reg(rs), 0xffff);
    tcg_gen_mov_i32(cdm16_tcg_reg(rd), result);
    cdm16_gen_flags_logic(result);
}
