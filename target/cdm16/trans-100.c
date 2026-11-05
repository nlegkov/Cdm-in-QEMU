#include "qemu/osdep.h"

#include "cpu.h"
#include "qemu/atomic128.h"
#include "tcg/tcg-op.h"
#include "exec/helper-gen.h"
#include "translate.h"

void cdm16_gen_int_imm9(uint16_t vector, uint32_t insn_pc) {
    TCGv_i32 sp = cdm16_tcg_sp();
    TCGv_i32 ps = cdm16_tcg_ps();

    TCGv_i32 new_sp = tcg_temp_new_i32();
    TCGv_i32 new_pc = tcg_temp_new_i32();
    TCGv_i32 new_ps = tcg_temp_new_i32();

    uint32_t return_pc = (insn_pc + 2) & 0xffff;
    uint32_t vector_addr = (uint32_t)vector * 4;

    /* 1. SP -= 2 */
    tcg_gen_subi_i32(new_sp, sp, 2);
    tcg_gen_andi_i32(new_sp, new_sp, 0xffff);
    cdm16_gen_write_sp(new_sp, insn_pc);

    /* 2. Сохраняем PS */
    tcg_gen_qemu_st_i32(ps, sp, CDM16_MMU_DATA, MO_LEUW);

    /* 3. SP -= 2 */
    tcg_gen_subi_i32(new_sp, sp, 2);
    tcg_gen_andi_i32(new_sp, new_sp, 0xffff);
    cdm16_gen_write_sp(new_sp, insn_pc);

    /* 4. Сохраняем адрес возврата */
    tcg_gen_qemu_st_i32(tcg_constant_i32(return_pc), sp, CDM16_MMU_DATA,
                        MO_LEUW);

    /* 5. Читаем PC из таблицы векторов */
    tcg_gen_qemu_ld_i32(new_pc, tcg_constant_i32(vector_addr), CDM16_MMU_CODE,
                        MO_LEUW);

    /* 6. Читаем PS из таблицы векторов */
    tcg_gen_qemu_ld_i32(new_ps, tcg_constant_i32(vector_addr + 2),
                        CDM16_MMU_CODE, MO_LEUW);

    /* 7. Устанавливаем новые PC и PS */
    tcg_gen_andi_i32(ps, new_ps, 0xffff);
    cdm16_gen_write_pc(new_pc, insn_pc);

    /* 8. Завершаем TB */
    tcg_gen_exit_tb(NULL, 0);
}