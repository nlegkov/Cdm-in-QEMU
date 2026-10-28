#include "qemu/osdep.h"

#include "cpu.h"
#include "translate.h"
#include "tcg/tcg-op.h"
#include "exec/helper-gen.h"

void cdm16_gen_mov(unsigned rs, unsigned rd) {
    TCGv_i32 reg_rd = cdm16_tcg_reg(rd);
    TCGv_i32 reg_rs = cdm16_tcg_reg(rs);

    /* Ограничиваем значение регистра rs 16 битами */
    tcg_gen_andi_i32(reg_rs, reg_rs, 0xffff);

    /* Записываем значение регистра rs в регистр rd */
    tcg_gen_mov_i32(reg_rd, reg_rs);
}