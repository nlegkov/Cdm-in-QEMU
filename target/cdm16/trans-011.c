#include "qemu/osdep.h"
#include "tcg/tcg-op.h"
#include "translate.h"
#include "cpu.h"

void cdm16_gen_lsw(unsigned rd, int32_t imm) {
    TCGv_i32 addr = tcg_temp_new_i32();

    /* addr = fp + imm * 2 */
    tcg_gen_addi_i32(addr, cdm16_tcg_reg(7), imm * 2);
    /* Ограничиваем адрес 16 битами */
    tcg_gen_andi_i32(addr, addr, 0xffff);

    /* rd = memory[addr] */
    tcg_gen_qemu_ld_i32(cdm16_tcg_reg(rd), addr, CDM16_MMU_DATA, MO_LEUW);
}

void cdm16_gen_lsb(unsigned rd, int32_t imm) {
    TCGv_i32 addr = tcg_temp_new_i32();

    /* addr = fp + imm */
    tcg_gen_addi_i32(addr, cdm16_tcg_reg(7), imm);

    /* Ограничиваем адрес 16 битами */
    tcg_gen_andi_i32(addr, addr, 0xffff);

    /* rd = memory[addr] */
    tcg_gen_qemu_ld_i32(cdm16_tcg_reg(rd), addr, CDM16_MMU_DATA, MO_UB);
}

void cdm16_gen_lssb(unsigned rd, int32_t imm) {
    TCGv_i32 addr = tcg_temp_new_i32();
    TCGv_i32 reg = cdm16_tcg_reg(rd);

    tcg_gen_addi_i32(addr, cdm16_tcg_reg(7), imm);
    tcg_gen_andi_i32(addr, addr, 0xffff);

    tcg_gen_qemu_ld_i32(reg, addr, CDM16_MMU_DATA, MO_SB);

    tcg_gen_andi_i32(reg, reg, 0xffff);
}

void cdm16_gen_ssw(unsigned rd, int32_t imm) {
    TCGv_i32 addr = tcg_temp_new_i32();

    /* addr = fp + imm * 2 */
    tcg_gen_addi_i32(addr, cdm16_tcg_reg(7), imm * 2);

    tcg_gen_andi_i32(addr, addr, 0xffff);

    /* memory[addr] = rd */
    tcg_gen_qemu_st_i32(cdm16_tcg_reg(rd), addr, CDM16_MMU_DATA, MO_LEUW);
}

void cdm16_gen_ssb(unsigned rd, int32_t imm) {
    TCGv_i32 addr = tcg_temp_new_i32();

    /* addr = fp + imm * 2 */
    tcg_gen_addi_i32(addr, cdm16_tcg_reg(7), imm);

    tcg_gen_andi_i32(addr, addr, 0xffff);

    /* memory[addr] = rd */
    tcg_gen_qemu_st_i32(cdm16_tcg_reg(rd), addr, CDM16_MMU_DATA, MO_UB);
}

void cdm16_gen_ldi_short(unsigned rd, int32_t imm) {
    tcg_gen_movi_i32(cdm16_tcg_reg(rd), (uint16_t)imm);
}

/* ADD rd, imm6 */
void cdm16_gen_add_imm(unsigned rd, int32_t imm) {
    TCGv_i32 reg = cdm16_tcg_reg(rd);

    TCGv_i32 a = tcg_temp_new_i32();
    TCGv_i32 sum = tcg_temp_new_i32();
    TCGv_i32 result = tcg_temp_new_i32();

    uint16_t b = (uint16_t)imm;

    /* Берём 16-битное значение регистра */
    tcg_gen_andi_i32(a, reg, 0xffff);

    /* Выполняем сложение с сохранением переноса */
    tcg_gen_addi_i32(sum, a, b);

    /* Ограничиваем результат 16 битами */
    tcg_gen_andi_i32(result, sum, 0xffff);

    /* Обновляем флаги */
    cdm16_gen_flags_add(a, tcg_constant_i32(b),
                        tcg_constant_i32(0), sum, result);

    /* Записываем результат в rd */
    tcg_gen_mov_i32(reg, result);
}

void cdm16_gen_cmp_imm(unsigned rd, int32_t imm)
{
    TCGv_i32 a = tcg_temp_new_i32();
    TCGv_i32 wide = tcg_temp_new_i32();
    TCGv_i32 result = tcg_temp_new_i32();
    uint16_t b = (uint16_t)imm;

    tcg_gen_andi_i32(a, cdm16_tcg_reg(rd), 0xffff);

    /* Preserve the carry-out, using the same subtraction as register CMP. */
    tcg_gen_addi_i32(wide, a, ((uint32_t)b ^ 0xffffu) + 1);
    tcg_gen_andi_i32(result, wide, 0xffff);

    cdm16_gen_flags_sub(a, tcg_constant_i32(b),
                        tcg_constant_i32(1), wide, result);
}
