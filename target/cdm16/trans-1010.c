#include "qemu/osdep.h"

#include "tcg/tcg-op.h"
#include "translate.h"
#include "cpu.h"

/* Чтение из заданного адресного пространства */
static void cdm16_gen_load(TCGv_i32 addr, unsigned rd, MemOp op, int mmu_idx) {
    TCGv_i32 dst = cdm16_tcg_reg(rd);

    tcg_gen_qemu_ld_i32(dst, addr, mmu_idx, op);

    /* Знаковое расширение до 16 бит */
    if (op == MO_SB) {
        tcg_gen_andi_i32(dst, dst, 0xffff);
    }
}

/* Запись в память данных */
static void cdm16_gen_store(TCGv_i32 addr, unsigned rd, MemOp op) {

    tcg_gen_qemu_st_i32(cdm16_tcg_reg(rd), addr, CDM16_MMU_DATA, op);
}

/* Память данных */

void cdm16_gen_ldw_add(unsigned rs1, unsigned rs2, unsigned rd) {
    TCGv_i32 addr = tcg_temp_new_i32();
    tcg_gen_add_i32(addr, cdm16_tcg_reg(rs1), cdm16_tcg_reg(rs2));
    tcg_gen_andi_i32(addr, addr, 0xffff);
    cdm16_gen_load(addr, rd, MO_LEUW, CDM16_MMU_DATA);
}

void cdm16_gen_ldb_add(unsigned rs1, unsigned rs2, unsigned rd) {
    TCGv_i32 addr = tcg_temp_new_i32();
    tcg_gen_add_i32(addr, cdm16_tcg_reg(rs1), cdm16_tcg_reg(rs2));
    tcg_gen_andi_i32(addr, addr, 0xffff);
    cdm16_gen_load(addr, rd, MO_UB, CDM16_MMU_DATA);
}

void cdm16_gen_ldsb_add(unsigned rs1, unsigned rs2, unsigned rd) {
    TCGv_i32 addr = tcg_temp_new_i32();
    tcg_gen_add_i32(addr, cdm16_tcg_reg(rs1), cdm16_tcg_reg(rs2));
    tcg_gen_andi_i32(addr, addr, 0xffff);
    cdm16_gen_load(addr, rd, MO_SB, CDM16_MMU_DATA);
}

void cdm16_gen_stw_add(unsigned rs1, unsigned rs2, unsigned rd) {
    TCGv_i32 addr = tcg_temp_new_i32();
    tcg_gen_add_i32(addr, cdm16_tcg_reg(rs1), cdm16_tcg_reg(rs2));
    tcg_gen_andi_i32(addr, addr, 0xffff);
    cdm16_gen_store(addr, rd, MO_LEUW);
}

void cdm16_gen_stb_add(unsigned rs1, unsigned rs2, unsigned rd) {
    TCGv_i32 addr = tcg_temp_new_i32();
    tcg_gen_add_i32(addr, cdm16_tcg_reg(rs1), cdm16_tcg_reg(rs2));
    tcg_gen_andi_i32(addr, addr, 0xffff);
    cdm16_gen_store(addr, rd, MO_UB);
}

/* Память инструкций */

void cdm16_gen_lcw_add(unsigned rs1, unsigned rs2, unsigned rd) {
    TCGv_i32 addr = tcg_temp_new_i32();
    tcg_gen_add_i32(addr, cdm16_tcg_reg(rs1), cdm16_tcg_reg(rs2));
    tcg_gen_andi_i32(addr, addr, 0xffff);
    cdm16_gen_load(addr, rd, MO_LEUW, CDM16_MMU_CODE);
}

void cdm16_gen_lcb_add(unsigned rs1, unsigned rs2, unsigned rd) {
    TCGv_i32 addr = tcg_temp_new_i32();
    tcg_gen_add_i32(addr, cdm16_tcg_reg(rs1), cdm16_tcg_reg(rs2));
    tcg_gen_andi_i32(addr, addr, 0xffff);
    cdm16_gen_load(addr, rd, MO_UB, CDM16_MMU_CODE);
}

void cdm16_gen_lcsb_add(unsigned rs1, unsigned rs2, unsigned rd) {
    TCGv_i32 addr = tcg_temp_new_i32();
    tcg_gen_add_i32(addr, cdm16_tcg_reg(rs1), cdm16_tcg_reg(rs2));
    tcg_gen_andi_i32(addr, addr, 0xffff);
    cdm16_gen_load(addr, rd, MO_SB, CDM16_MMU_CODE);
}