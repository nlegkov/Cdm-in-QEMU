#include "qemu/osdep.h"
#include "tcg/tcg-op.h"
#include "translate.h"

void cdm16_gen_branch(unsigned code, uint16_t target, uint16_t next_pc,
                      uint32_t fault_pc) {
    TCGv_i32 ps = cdm16_tcg_ps();

    TCGv_i32 c = tcg_temp_new_i32();
    TCGv_i32 v = tcg_temp_new_i32();
    TCGv_i32 z = tcg_temp_new_i32();
    TCGv_i32 n = tcg_temp_new_i32();

    TCGv_i32 take = tcg_temp_new_i32();
    TCGv_i32 tmp = tcg_temp_new_i32();
    TCGv_i32 new_pc = tcg_temp_new_i32();

    /* Извлекаем флаги PS */
    tcg_gen_shri_i32(c, ps, 3);
    tcg_gen_andi_i32(c, c, 1);

    tcg_gen_shri_i32(v, ps, 2);
    tcg_gen_andi_i32(v, v, 1);

    tcg_gen_shri_i32(z, ps, 1);
    tcg_gen_andi_i32(z, z, 1);

    tcg_gen_andi_i32(n, ps, 1);

    /* Проверяем условие перехода */
    switch (code >> 1) {
    case 0: /* EQ: Z */
        tcg_gen_mov_i32(take, z);
        break;

    case 1: /* HS: C */
        tcg_gen_mov_i32(take, c);
        break;

    case 2: /* MI: N */
        tcg_gen_mov_i32(take, n);
        break;

    case 3: /* VS: V */
        tcg_gen_mov_i32(take, v);
        break;

    case 4: /* HI: C && !Z */
        tcg_gen_xori_i32(tmp, z, 1);
        tcg_gen_and_i32(take, c, tmp);
        break;

    case 5: /* GE: N == V */
        tcg_gen_xor_i32(take, n, v);
        tcg_gen_xori_i32(take, take, 1);
        break;

    case 6: /* GT: !Z && (N == V) */
        tcg_gen_xor_i32(take, n, v);
        tcg_gen_or_i32(take, take, z);
        tcg_gen_xori_i32(take, take, 1);
        break;

    case 7: /* Безусловный переход */
        tcg_gen_movi_i32(take, 1);
        break;
    }

    /* Нечётный код условия инвертирует результат */
    if (code & 1) {
        tcg_gen_xori_i32(take, take, 1);
    }

    /*
     * Если take != 0:
     *     PC = target
     * Иначе:
     *     PC = next_pc
     */
    tcg_gen_movcond_i32(TCG_COND_NE, new_pc, take, tcg_constant_i32(0),
                        tcg_constant_i32(target), tcg_constant_i32(next_pc));

    /* Проверяем выравнивание и записываем PC */
    cdm16_gen_write_pc(new_pc, fault_pc);

    /* Выходим из текущего TB */
    tcg_gen_exit_tb(NULL, 0);
}