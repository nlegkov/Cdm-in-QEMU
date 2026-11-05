#include "qemu/osdep.h"

#include "cpu.h"
#include "tcg/tcg-op.h"
#include "translate.h"

void cdm16_gen_ei(void) {
    TCGv_i32 ps = cdm16_tcg_ps();

    tcg_gen_ori_i32(ps, ps, 0x8000);
}

void cdm16_gen_di(void) {
    TCGv_i32 ps = cdm16_tcg_ps();

    tcg_gen_andi_i32(ps, ps, 0x7fff);
}

void cdm16_gen_rti(uint32_t insn_pc) {

    TCGv_i32 sp = cdm16_tcg_sp();
    TCGv_i32 ps = cdm16_tcg_ps();

    TCGv_i32 new_sp = tcg_temp_new_i32();
    TCGv_i32 new_pc = tcg_temp_new_i32();
    TCGv_i32 new_ps = tcg_temp_new_i32();

    /*
     * Проверяем исходный SP до чтения памяти.
     * При успехе записывается то же значение SP.
     */
    cdm16_gen_write_sp(sp, insn_pc);

    /* Читаем сохранённый PC, пока не изменяя SP. */
    tcg_gen_qemu_ld_i32(new_pc, sp, CDM16_MMU_DATA, MO_LEUW);

    /* Адрес сохранённого PS. */
    tcg_gen_addi_i32(new_sp, sp, 2);
    tcg_gen_andi_i32(new_sp, new_sp, 0xffff);

    tcg_gen_qemu_ld_i32(new_ps, new_sp, CDM16_MMU_DATA, MO_LEUW);

    /* Итоговый SP после снятия двух слов. */
    tcg_gen_addi_i32(new_sp, sp, 4);
    tcg_gen_andi_i32(new_sp, new_sp, 0xffff);

    /*
     * Проверяем PC и записываем только при успехе.
     * При ошибке SP и PS ещё сохраняют исходные значения.
     */
    cdm16_gen_write_pc(new_pc, insn_pc);

    /* Оставшиеся операции не вызывают исключений. */
    tcg_gen_mov_i32(sp, new_sp);
    tcg_gen_andi_i32(ps, new_ps, 0xffff);

    tcg_gen_exit_tb(NULL, 0);
}

void cdm16_gen_jsr_label(uint16_t target, uint32_t insn_pc) {
    /*
     * target известен при трансляции.
     * При неверном адресе генерируем исключение,
     * не изменяя стек.
     */
    if (target & 1) {
        cdm16_gen_exception(CDM16_EXCP_UNALIGNED_PC, insn_pc);
        return;
    }

    TCGv_i32 sp = cdm16_tcg_sp();
    TCGv_i32 pc = cdm16_tcg_pc();
    TCGv_i32 new_sp = tcg_temp_new_i32();

    uint32_t return_pc = (insn_pc + 4) & 0xffff;

    /* Стек растёт в сторону меньших адресов. */
    tcg_gen_subi_i32(new_sp, sp, 2);
    tcg_gen_andi_i32(new_sp, new_sp, 0xffff);

    /* Проверяем выравнивание и обновляем SP. */
    cdm16_gen_write_sp(new_sp, insn_pc);

    /* [SP] = адрес инструкции после JSR imm16. */
    tcg_gen_qemu_st_i32(tcg_constant_i32(return_pc), sp, CDM16_MMU_DATA,
                        MO_LEUW);

    /* target уже проверен на выравнивание. */
    tcg_gen_movi_i32(pc, target);

    tcg_gen_exit_tb(NULL, 0);
}

void cdm16_gen_pupc(uint32_t insn_pc) {
    TCGv_i32 sp = cdm16_tcg_sp();
    TCGv_i32 pc = cdm16_tcg_pc();
    TCGv_i32 new_sp = tcg_temp_new_i32();

    uint32_t next_pc = (insn_pc + 2) & 0xffff;

    /* SP -= 2 с проверкой выравнивания. */
    tcg_gen_subi_i32(new_sp, sp, 2);
    tcg_gen_andi_i32(new_sp, new_sp, 0xffff);
    cdm16_gen_write_sp(new_sp, insn_pc);

    /* Сохраняем адрес следующей инструкции. */
    tcg_gen_qemu_st_i32(tcg_constant_i32(next_pc), sp, CDM16_MMU_DATA, MO_LEUW);

    /* Продолжаем после PUPC. */
    tcg_gen_movi_i32(pc, next_pc);
    tcg_gen_exit_tb(NULL, 0);
}

void cdm16_gen_popc(uint32_t insn_pc) {
    TCGv_i32 sp = cdm16_tcg_sp();
    TCGv_i32 new_sp = tcg_temp_new_i32();
    TCGv_i32 new_pc = tcg_temp_new_i32();

    /*
     * Проверяем исходный SP до чтения.
     * При успехе записывается то же значение.
     */
    cdm16_gen_write_sp(sp, insn_pc);

    /* Читаем PC, не снимая слово со стека. */
    tcg_gen_qemu_ld_i32(new_pc, sp, CDM16_MMU_DATA, MO_LEUW);

    /* Рассчитываем SP после снятия слова. */
    tcg_gen_addi_i32(new_sp, sp, 2);
    tcg_gen_andi_i32(new_sp, new_sp, 0xffff);

    /*
     * Проверяем и устанавливаем PC.
     * При нечётном PC исключение возникает
     * с исходным значением SP.
     */
    cdm16_gen_write_pc(new_pc, insn_pc);

    /* Исходный SP чётный, поэтому SP + 2 тоже чётный. */
    tcg_gen_mov_i32(sp, new_sp);

    tcg_gen_exit_tb(NULL, 0);
}

void cdm16_gen_pusp(uint32_t insn_pc) {
    TCGv_i32 sp = cdm16_tcg_sp();
    TCGv_i32 new_sp = tcg_temp_new_i32();

    /* Сначала уменьшаем SP. */
    tcg_gen_subi_i32(new_sp, sp, 2);
    tcg_gen_andi_i32(new_sp, new_sp, 0xffff);
    cdm16_gen_write_sp(new_sp, insn_pc);

    /* Сохраняем уже уменьшенное значение SP. */
    tcg_gen_qemu_st_i32(sp, sp, CDM16_MMU_DATA, MO_LEUW);
}

void cdm16_gen_posp(uint32_t insn_pc) {
    TCGv_i32 sp = cdm16_tcg_sp();
    TCGv_i32 new_sp_value = tcg_temp_new_i32();

    /* Проверяем исходный SP до чтения памяти. */
    cdm16_gen_write_sp(sp, insn_pc);

    /* Читаем сохранённое значение SP. */
    tcg_gen_qemu_ld_i32(new_sp_value, sp, CDM16_MMU_DATA, MO_LEUW);

    /* Проверяем и устанавливаем загруженный SP. */
    cdm16_gen_write_sp(new_sp_value, insn_pc);
}

void cdm16_gen_pups(uint32_t insn_pc) {
    TCGv_i32 sp = cdm16_tcg_sp();
    TCGv_i32 ps = cdm16_tcg_ps();
    TCGv_i32 new_sp = tcg_temp_new_i32();

    /* SP -= 2 с проверкой выравнивания. */
    tcg_gen_subi_i32(new_sp, sp, 2);
    tcg_gen_andi_i32(new_sp, new_sp, 0xffff);
    cdm16_gen_write_sp(new_sp, insn_pc);

    /* [SP] = PS, сохраняем младшие 16 бит. */
    tcg_gen_qemu_st_i32(ps, sp, CDM16_MMU_DATA, MO_LEUW);
}

void cdm16_gen_pops(uint32_t insn_pc) {
    TCGv_i32 sp = cdm16_tcg_sp();
    TCGv_i32 ps = cdm16_tcg_ps();
    TCGv_i32 new_sp = tcg_temp_new_i32();
    TCGv_i32 new_ps = tcg_temp_new_i32();

    /* Проверяем исходный SP до чтения памяти. */
    cdm16_gen_write_sp(sp, insn_pc);

    /* Читаем сохранённый PS. */
    tcg_gen_qemu_ld_i32(new_ps, sp, CDM16_MMU_DATA, MO_LEUW);

    /* Рассчитываем SP после снятия слова. */
    tcg_gen_addi_i32(new_sp, sp, 2);
    tcg_gen_andi_i32(new_sp, new_sp, 0xffff);

    /* Исходный SP чётный, поэтому новое значение тоже чётное. */
    tcg_gen_mov_i32(sp, new_sp);
    tcg_gen_andi_i32(ps, new_ps, 0xffff);
}