#include "qemu/osdep.h"

#include "cpu.h"
#include "translate.h"
#include "tcg/tcg-op.h"
#include "exec/helper-gen.h"

void cdm16_gen_push_rd(unsigned rd) {
    TCGv_i32 sp = cdm16_tcg_sp();
    TCGv_i32 reg = cdm16_tcg_reg(rd);

    /* Вычитаем 2 из SP */
    tcg_gen_subi_i32(sp, sp, 2);

    /* Ограничиваем SP 16 битами */
    tcg_gen_andi_i32(sp, sp, 0xffff);

    /* Записываем значение регистра в память по адресу SP */
    tcg_gen_qemu_st_i32(reg, sp, CDM16_MMU_DATA, MO_LEUW);
}

void cdm16_gen_pop_rd(unsigned rd) {
    TCGv_i32 sp = cdm16_tcg_sp();
    TCGv_i32 reg = cdm16_tcg_reg(rd);

    /* Читаем значение из памяти по адресу SP */
    tcg_gen_qemu_ld_i32(reg, sp, CDM16_MMU_DATA, MO_LEUW);

    /* Прибавляем 2 к SP */
    tcg_gen_addi_i32(sp, sp, 2);

    /* Ограничиваем SP 16 битами */
    tcg_gen_andi_i32(sp, sp, 0xffff);
}

void cdm16_gen_ldi_16(unsigned rd, uint16_t imm16) {
    TCGv_i32 reg = cdm16_tcg_reg(rd);

    /* Загружаем 16-битное немедленное значение в регистр */
    tcg_gen_movi_i32(reg, imm16);
}

void cdm16_gen_jsrr(unsigned rd, uint32_t insn_pc) {
    TCGv_i32 reg = cdm16_tcg_reg(rd);
    TCGv_i32 sp = cdm16_tcg_sp();

    TCGv_i32 new_sp = tcg_temp_new_i32();

    /* Адрес возврата */
    uint32_t return_pc = (insn_pc + 2) & 0xffff;

    /* Вычисляем новый SP, пока не изменяя сам регистр */
    tcg_gen_subi_i32(new_sp, sp, 2);
    tcg_gen_andi_i32(new_sp, new_sp, 0xffff);

    /* SP = new_sp с проверкой выравнивания */
    cdm16_gen_write_sp(new_sp, insn_pc);

    /* Сохраняем адрес возврата в стек */
    tcg_gen_qemu_st_i32(tcg_constant_i32(return_pc), sp, CDM16_MMU_DATA, MO_LEUW);

    /* PC = rd с проверкой выравнивания */
    cdm16_gen_write_pc(reg, insn_pc);

    /* Завершаем текущий TB */
    tcg_gen_exit_tb(NULL, 0);
}

void cdm16_gen_ldsp(unsigned rd) {
    TCGv_i32 sp = cdm16_tcg_sp();
    TCGv_i32 reg = cdm16_tcg_reg(rd);

    tcg_gen_mov_i32(reg, sp);
}

void cdm16_gen_stsp(unsigned rd, uint32_t fault_pc) {
    TCGv_i32 reg = cdm16_tcg_reg(rd);

    cdm16_gen_write_sp(reg, fault_pc);
}

void cdm16_gen_ldps(unsigned rd) {
    TCGv_i32 ps = cdm16_tcg_ps();
    TCGv_i32 reg = cdm16_tcg_reg(rd);

    tcg_gen_andi_i32(reg, ps, 0xffff);
}

void cdm16_gen_stps(unsigned rd) {
    TCGv_i32 ps = cdm16_tcg_ps();
    TCGv_i32 reg = cdm16_tcg_reg(rd);

    tcg_gen_andi_i32(ps, reg, 0xffff);
}

void cdm16_gen_ldpc(unsigned rd, uint32_t insn_pc)
{
    TCGv_i32 reg = cdm16_tcg_reg(rd);

    uint32_t next_pc = (insn_pc + 2) & 0xffff;

    tcg_gen_movi_i32(reg, next_pc);
}

void cdm16_gen_stpc(unsigned rd, uint32_t fault_pc) {
    TCGv_i32 reg = cdm16_tcg_reg(rd);

    cdm16_gen_write_pc(reg, fault_pc);

    tcg_gen_exit_tb(NULL, 0);
}

void cdm16_gen_addsp(unsigned rd, uint32_t fault_pc) {
    TCGv_i32 sp = cdm16_tcg_sp();
    TCGv_i32 reg = cdm16_tcg_reg(rd);

    TCGv_i32 new_sp = tcg_temp_new_i32();

    // fprintf(stderr, "TRANSLATE ADDSP: rd=%u PC=0x%04x\n", rd, fault_pc);
    /* Вычисляем новый SP */
    tcg_gen_add_i32(new_sp, sp, reg);
    tcg_gen_andi_i32(new_sp, new_sp, 0xffff);

    // gen_helper_cdm16_debug_addsp(sp, reg, new_sp);

    /* SP = new_sp с проверкой выравнивания */
    cdm16_gen_write_sp(new_sp, fault_pc);
}