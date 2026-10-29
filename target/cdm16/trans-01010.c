#include "qemu/osdep.h"
#include "tcg/tcg-op.h"
#include "translate.h"
#include "cpu.h"

/* Получение 16-битного адреса */
static TCGv_i32 cdm16_mem_addr(unsigned rs) {
    TCGv_i32 addr = tcg_temp_new_i32();

    tcg_gen_andi_i32(addr, cdm16_tcg_reg(rs), 0xffff);

    return addr;
}

/* Чтение из заданного адресного пространства */
static void cdm16_gen_load(unsigned rs, unsigned rd, MemOp op, int mmu_idx) {
    TCGv_i32 addr = cdm16_mem_addr(rs);
    TCGv_i32 dst = cdm16_tcg_reg(rd);

    tcg_gen_qemu_ld_i32(dst, addr, mmu_idx, op);

    /* Знаковое расширение до 16 бит */
    if (op == MO_SB) {
        tcg_gen_andi_i32(dst, dst, 0xffff);
    }
}

/* Запись в память данных */
static void cdm16_gen_store(unsigned rs, unsigned rd, MemOp op) {
    TCGv_i32 addr = cdm16_mem_addr(rs);

    tcg_gen_qemu_st_i32(cdm16_tcg_reg(rd), addr, CDM16_MMU_DATA, op);
}

/* Память данных */

void cdm16_gen_ldw(unsigned rs, unsigned rd) {
    cdm16_gen_load(rs, rd, MO_LEUW, CDM16_MMU_DATA);
}

void cdm16_gen_ldb(unsigned rs, unsigned rd) {
    cdm16_gen_load(rs, rd, MO_UB, CDM16_MMU_DATA);
}

void cdm16_gen_ldsb(unsigned rs, unsigned rd) {
    cdm16_gen_load(rs, rd, MO_SB, CDM16_MMU_DATA);
}

void cdm16_gen_stw(unsigned rs, unsigned rd) {
    cdm16_gen_store(rs, rd, MO_LEUW);
}

void cdm16_gen_stb(unsigned rs, unsigned rd) { cdm16_gen_store(rs, rd, MO_UB); }

/* Память инструкций */

void cdm16_gen_lcw(unsigned rs, unsigned rd) {
    cdm16_gen_load(rs, rd, MO_LEUW, CDM16_MMU_CODE);
}

void cdm16_gen_lcb(unsigned rs, unsigned rd) {
    cdm16_gen_load(rs, rd, MO_UB, CDM16_MMU_CODE);
}

void cdm16_gen_lcsb(unsigned rs, unsigned rd) {
    cdm16_gen_load(rs, rd, MO_SB, CDM16_MMU_CODE);
}