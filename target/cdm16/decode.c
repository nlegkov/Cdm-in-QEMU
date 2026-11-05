#include "qemu/osdep.h"
#include "translate.h"
#include "decode.h"

static int decode_00000(CPUCdM16State *env, DisasContextBase *db,
                        uint16_t insn) {
    /* Формат: 00000 0000000 op_type(4) */

    if ((insn & 0xfff0u) != 0) {
        return 0;
    }

    unsigned op_type = insn & 0b1111;

    switch (op_type) {

    case 0b0000: /* ZERO: недопустимая инструкция */
    case 0b0001: /* Reserved */
    case 0b0010: /* Reserved */
    case 0b0011: /* Reserved */
        cdm16_gen_exception(CDM16_EXCP_INVALID_INSN, db->pc_next);

        db->is_jmp = DISAS_NORETURN;
        return 2;

    case 0b0100: /* HALT */
        cdm16_gen_halt((db->pc_next + 2) & 0xffff);
        db->is_jmp = DISAS_NORETURN;
        return 2;

    default:
        return 0;
    }
}

static int decode_0001(uint16_t insn) {
    /* Формат: 0001 alu_op(3) shift_val(3) rs(3) rd(3) */

    unsigned alu_op = (insn >> 9) & 0b111;
    unsigned shift_val = ((insn >> 6) & 0b111) + 1;
    unsigned rs = (insn >> 3) & 0b111;
    unsigned rd = insn & 0b111;

    switch (alu_op) {
    case 0b000: // SHL rs rd shift_val
        cdm16_gen_shl(rs, rd, shift_val);
        return 2;

    case 0b001: // SHR rs rd shift_val
        cdm16_gen_shr(rs, rd, shift_val);
        return 2;

    case 0b010: // SHRA rs rd shift_val
        cdm16_gen_shra(rs, rd, shift_val);
        return 2;

    case 0b011: // ROL rs rd shift_val
        cdm16_gen_rol(rs, rd, shift_val);
        return 2;

    case 0b100: // ROR rs rd shift_val
        cdm16_gen_ror(rs, rd, shift_val);
        return 2;

    case 0b101: // RCL rs rd shift_val
        cdm16_gen_rcl(rs, rd, shift_val);
        return 2;

    case 0b110: // RCR rs rd shift_val
        cdm16_gen_rcr(rs, rd, shift_val);
        return 2;

    default:
        return 0;
    }
}

static int decode_001(CPUCdM16State *env, DisasContextBase *db, uint16_t insn) {
    /* Формат: 001 000000 op_type(4) rd(3) */

    unsigned op_type = (insn >> 3) & 0b1111;
    unsigned rd = insn & 0b111;

    switch (op_type) {
    case 0b0000: // PUSH rd
        cdm16_gen_push_rd(rd);
        return 2;

    case 0b0001: // POP rd
        cdm16_gen_pop_rd(rd);
        return 2;

    case 0b0010: { // LDI rd, imm16
        uint16_t imm16 =
            translator_lduw_end(env, db, (db->pc_next + 2) & 0xffff, MO_LE);
        cdm16_gen_ldi_16(rd, imm16);
        return 4;
    }

    case 0b0011: // JSRR rd
        cdm16_gen_jsrr(rd, db->pc_next);
        db->is_jmp = DISAS_NORETURN;
        return 2;

    case 0b0100: // LDSP rd
        cdm16_gen_ldsp(rd);
        return 2;

    case 0b0101: // STSP rd
        cdm16_gen_stsp(rd, db->pc_next);
        return 2;

    case 0b0110: // LDPS rd
        cdm16_gen_ldps(rd);
        return 2;

    case 0b0111: // STPS rd
        cdm16_gen_stps(rd);
        return 2;

    case 0b1000: // LDPC rd
        cdm16_gen_ldpc(rd, db->pc_next);
        return 2;

    case 0b1001: // STPC rd
        cdm16_gen_stpc(rd, db->pc_next);
        db->is_jmp = DISAS_NORETURN;
        return 2;

    case 0b1010: // ADDSP rd
        cdm16_gen_addsp(rd, db->pc_next);
        return 2;
    }

    return 0;
}

static int decode_011(uint16_t insn) {
    /* Формат: 011 op_type(4) imm6(6) rd(3) */

    unsigned op_type = (insn >> 9) & 0b1111;
    unsigned rd = insn & 0b111;

    int16_t imm = (insn >> 3) & 0b111111;

    if (op_type & 0b1) {
        imm -= 64;
    }

    switch (op_type >> 1) {
    case 0b000: // LSW
        cdm16_gen_lsw(rd, imm);
        return 2;

    case 0b001: // LSB
        cdm16_gen_lsb(rd, imm);
        return 2;

    case 0b010: // LSSB
        cdm16_gen_lssb(rd, imm);
        return 2;

    case 0b011: // SSW
        cdm16_gen_ssw(rd, imm);
        return 2;

    case 0b100: // SSB
        cdm16_gen_ssb(rd, imm);
        return 2;

    case 0b101: // LDI imm6
        cdm16_gen_ldi_short(rd, imm);
        return 2;

    case 0b110: // ADD imm6
        cdm16_gen_add_imm(rd, imm);
        return 2;

    case 0b111: // CMP imm6
        cdm16_gen_cmp_imm(rd, imm);
        return 2;
    }

    return 0;
}

static int decode_01000(uint16_t insn) {
    /* Формат: 01000 op_type(5) rs(3) rd(3) */
    unsigned op_type = (insn >> 6) & 0b11111;
    unsigned rs = (insn >> 3) & 0b111;
    unsigned rd = insn & 0b111;

    switch (op_type) {
    case 0b00000: // MOV rs, rd
        cdm16_gen_mov(rs, rd);
        return 2;

    default:
        return 0;
    }
}

static int decode_01010(uint16_t insn) {
    /* Формат: 01010 op_type(5) rs(3) rd(3) */
    unsigned op_type = (insn >> 6) & 0b11111;
    unsigned rs = (insn >> 3) & 0b111;
    unsigned rd = insn & 0b111;

    switch (op_type) {
    case 0b00000: // ldw rs, rd
        cdm16_gen_ldw(rs, rd);
        return 2;

    case 0b00001: // ldb rs, rd
        cdm16_gen_ldb(rs, rd);
        return 2;

    case 0b00010: // ldsb rs, rd
        cdm16_gen_ldsb(rs, rd);
        return 2;

    case 0b00011: // lcw rs, rd
        cdm16_gen_lcw(rs, rd);
        return 2;

    case 0b00100: // lcb rs, rd
        cdm16_gen_lcb(rs, rd);
        return 2;

    case 0b00101: // lcsb rs, rd
        cdm16_gen_lcsb(rs, rd);
        return 2;

    case 0b00110: // stw rs, rd
        cdm16_gen_stw(rs, rd);
        return 2;
    
    case 0b00111: // stb rs, rd
        cdm16_gen_stb(rs, rd);
        return 2;

    default:
        return 0;
    }
}

static int decode_1011(uint16_t insn) {
    /* Формат: 1011 alu_op(3) rs1(3) rs0(3) rd(3) */
    unsigned alu_op = (insn >> 9) & 0b111;
    unsigned rs1 = (insn >> 6) & 0b111;
    unsigned rs0 = (insn >> 3) & 0b111;
    unsigned rd = insn & 0b111;

    switch (alu_op) {
    case 0b000: // AND rs0, rs1, rd
        cdm16_gen_and(rs0, rs1, rd);
        return 2;

    case 0b001: // OR rs0, rs1, rd
        cdm16_gen_or(rs0, rs1, rd);
        return 2;

    case 0b010: // XOR rs0, rs1, rd
        cdm16_gen_xor(rs0, rs1, rd);
        return 2;

    case 0b011: // BIC rs0, rs1, rd
        cdm16_gen_bic(rs0, rs1, rd);
        return 2;

    case 0b100: // ADD rs0, rs1, rd
        cdm16_gen_add(rs0, rs1, rd);
        return 2;

    case 0b101: // ADDC rs0, rs1, rd
        cdm16_gen_addc(rs0, rs1, rd);
        return 2;

    case 0b110: // SUB rs0, rs1, rd
        cdm16_gen_sub(rs0, rs1, rd);
        return 2;
        
    case 0b111: // SUBC rs0, rs1, rd
        cdm16_gen_subc(rs0, rs1, rd);
        return 2;

    default:
        return 0;
    }
}

static int decode_1010(uint16_t insn) {
    /* Формат: 1010 op_type(3) rs1(3) rs0(3) rd(3) */
    unsigned alu_op = (insn >> 9) & 0b111;
    unsigned rs1 = (insn >> 6) & 0b111;
    unsigned rs0 = (insn >> 3) & 0b111;
    unsigned rd = insn & 0b111;

    switch (alu_op) {
    case 0b000: // LDW rs0, rs1, rd
        cdm16_gen_ldw_add(rs0, rs1, rd);
        return 2;

    case 0b001: // LDB rs0, rs1, rd
        cdm16_gen_ldb_add(rs0, rs1, rd);
        return 2;

    case 0b010: // LDSB rs0, rs1, rd
        cdm16_gen_ldsb_add(rs0, rs1, rd);
        return 2;

    case 0b011: // LCW rs0, rs1, rd
        cdm16_gen_lcw_add(rs0, rs1, rd);
        return 2;

    case 0b100: // LCB rs0, rs1, rd 
        cdm16_gen_lcb_add(rs0, rs1, rd);
        return 2;

    case 0b101: // LCSB rs0, rs1, rd    
        cdm16_gen_lcsb_add(rs0, rs1, rd);
        return 2;

    case 0b110: // STW rs0, rs1, rd
        cdm16_gen_stw_add(rs0, rs1, rd);
        return 2;

    case 0b111: // STB rs0, rs1, rd
        cdm16_gen_stb_add(rs0, rs1, rd);
        return 2;

    default:
        return 0;
    }
}

static int decode_01001(uint16_t insn) {
    /* Формат: 01001 alu_op(3) rs(3) rd(3) */
    unsigned alu_op = (insn >> 6) & 0b111;
    unsigned rs = (insn >> 3) & 0b111;
    unsigned rd = insn & 0b111;

    switch (alu_op) {
    case 0b000: // BIT rs, rd
        cdm16_gen_bit(rs, rd);
        return 2;

    case 0b110: // CMP rs, rd
        cdm16_gen_cmp(rs, rd);
        return 2;

    default:
        return 0;
    }
}

static int decode_01011(uint16_t insn) {
    /* Формат: 01011 alu_op(3) rs(3) rd(3) */
    unsigned alu_op = (insn >> 6) & 0b111;
    unsigned rs = (insn >> 3) & 0b111;
    unsigned rd = insn & 0b111;

    switch (alu_op) {
    case 0b000: // NEG rs, rd
        cdm16_gen_neg(rs, rd);
        return 2;

    case 0b001: // NOT rs, rd
        cdm16_gen_not(rs, rd);
        return 2;

    case 0b010: // SXT rs, rd
        cdm16_gen_sxt(rs, rd);
        return 2;

    case 0b011: // SCL rs, rd
        cdm16_gen_scl(rs, rd);
        return 2;

    default:
        return 0;
    }
}

static int decode_group_000(CPUCdM16State *env, DisasContextBase *db,
                            uint16_t insn) {
    switch (insn >> 12) {
    case 0b0000:
        switch (insn >> 11) {
        case 0b00000:
            return decode_00000(env, db, insn);

        case 0b00001:
            return 0; // return decode_00001(env, db, insn);
        }
        break;

    case 0b0001:
        return decode_0001(insn); // return decode_0001(env, db, insn);
    }

    return 0;
}

static int decode_group_010(CPUCdM16State *env, DisasContextBase *db,
                            uint16_t insn) {
    switch (insn >> 11) {
    case 0b01000:
        return decode_01000(insn);

    case 0b01010:
        return decode_01010(insn); // return decode_01010(env, db, insn);
    
    case 0b01011:
        return decode_01011(insn);

    case 0b01001:
        return decode_01001(insn);
    }

    return 0;
}

static int decode_group_101(CPUCdM16State *env, DisasContextBase *db, uint16_t insn) {

    switch (insn >> 12) {
        case 0b1011:
            return decode_1011(insn);
        case 0b1010:
            return decode_1010(insn);
    }

    return 0;
}

int cdm16_decode_instruction(CPUCdM16State *env, DisasContextBase *db,
                             uint16_t insn) {
    (void)env;
    (void)db;

    switch (insn >> 13) {
    case 0b000:
        return decode_group_000(env, db, insn);

    case 0b001:
        return decode_001(env, db, insn); // Позже decode_001

    case 0b010:
        return decode_group_010(env, db, insn);

    case 0b011:
        return decode_011(insn);

    case 0b100:
        return 0; // Позже decode_100

    case 0b101:
        return decode_group_101(env, db, insn); // Позже decode_group_101
    case 0b110:
    case 0b111:
        return 0; // Позже decode_branch
    }

    return 0;
}