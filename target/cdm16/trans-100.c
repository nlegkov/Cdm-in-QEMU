#include "qemu/osdep.h"

#include "cpu.h"
#include "tcg/tcg-op.h"
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

void cdm16_gen_reset_imm9(uint16_t vector, uint32_t insn_pc) {
    TCGv_i32 ps = cdm16_tcg_ps();
    TCGv_i32 new_pc = tcg_temp_new_i32();
    TCGv_i32 new_ps = tcg_temp_new_i32();

    uint32_t vector_addr = (uint32_t)vector * 4;

    /* RESET загружает вектор без сохранения кадра в стек. */
    tcg_gen_qemu_ld_i32(new_pc, tcg_constant_i32(vector_addr), CDM16_MMU_CODE,
                        MO_LEUW);

    tcg_gen_qemu_ld_i32(new_ps, tcg_constant_i32(vector_addr + 2),
                        CDM16_MMU_CODE, MO_LEUW);

    /* Проверяем PC до изменения PS. */
    cdm16_gen_write_pc(new_pc, insn_pc);
    tcg_gen_andi_i32(ps, new_ps, 0xffff);

    tcg_gen_exit_tb(NULL, 0);
}

void cdm16_gen_push_imm9(int32_t imm, uint32_t insn_pc) {
    TCGv_i32 sp = cdm16_tcg_sp();
    TCGv_i32 new_sp = tcg_temp_new_i32();

    tcg_gen_subi_i32(new_sp, sp, 2);
    tcg_gen_andi_i32(new_sp, new_sp, 0xffff);
    cdm16_gen_write_sp(new_sp, insn_pc);

    /* Отрицательное значение сохраняется в дополнительном коде. */
    tcg_gen_qemu_st_i32(tcg_constant_i32((uint16_t)imm), sp, CDM16_MMU_DATA,
                        MO_LEUW);
}

void cdm16_gen_addsp_imm9(int32_t imm, uint32_t insn_pc) {
    TCGv_i32 sp = cdm16_tcg_sp();
    TCGv_i32 new_sp = tcg_temp_new_i32();

    /* Закодированное смещение задаётся в словах. */
    tcg_gen_addi_i32(new_sp, sp, imm * 2);
    tcg_gen_andi_i32(new_sp, new_sp, 0xffff);

    cdm16_gen_write_sp(new_sp, insn_pc);
}

void cdm16_gen_jsr_imm9(int32_t imm, uint32_t insn_pc) {
    TCGv_i32 sp = cdm16_tcg_sp();
    TCGv_i32 pc = cdm16_tcg_pc();
    TCGv_i32 new_sp = tcg_temp_new_i32();

    uint32_t return_pc = (insn_pc + 2) & 0xffff;
    uint16_t target = ((int32_t)return_pc + imm * 2) & 0xffff;

    tcg_gen_subi_i32(new_sp, sp, 2);
    tcg_gen_andi_i32(new_sp, new_sp, 0xffff);
    cdm16_gen_write_sp(new_sp, insn_pc);

    tcg_gen_qemu_st_i32(tcg_constant_i32(return_pc), sp, CDM16_MMU_DATA,
                        MO_LEUW);

    /*
     * return_pc чётный, смещение кратно двум,
     * поэтому target тоже чётный.
     */
    tcg_gen_movi_i32(pc, target);
    tcg_gen_exit_tb(NULL, 0);
}