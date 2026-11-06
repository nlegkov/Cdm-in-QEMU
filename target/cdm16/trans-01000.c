#include "qemu/osdep.h"

#include "cpu.h"
#include "translate.h"
#include "tcg/tcg-op.h"
#include "exec/helper-gen.h"

void cdm16_gen_move(unsigned rs, unsigned rd)
{
    TCGv_i32 ps = cdm16_tcg_ps();
    TCGv_i32 result = tcg_temp_new_i32();
    TCGv_i32 z = tcg_temp_new_i32();
    TCGv_i32 n = tcg_temp_new_i32();

    /* Получаем 16-битное значение, не изменяя источник. */
    tcg_gen_andi_i32(result, cdm16_tcg_reg(rs), 0xffff);

    /* Копируем результат в назначение. */
    tcg_gen_mov_i32(cdm16_tcg_reg(rd), result);

    /* Z: результат равен нулю. Позиция в PS — бит 1. */
    tcg_gen_setcondi_i32(TCG_COND_EQ, z, result, 0);
    tcg_gen_shli_i32(z, z, 1);

    /* N: старший бит 16-битного результата. */
    tcg_gen_shri_i32(n, result, 15);

    /* Очищаем C, V, Z, N, остальные биты сохраняем. */
    tcg_gen_andi_i32(ps, ps, 0xfff0);

    /* Записываем новые Z и N. C и V остаются нулевыми. */
    tcg_gen_or_i32(ps, ps, z);
    tcg_gen_or_i32(ps, ps, n);
}