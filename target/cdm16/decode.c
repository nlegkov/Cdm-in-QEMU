#include "qemu/osdep.h"
#include "decode.h"
#include "translate.h"

static int decode_011(uint16_t insn)
{
    /* Формат: 011 op_type(4) imm6(6) rd(3) */

    unsigned op_type = (insn >> 9) & 0b1111;
    unsigned rd = insn & 0b111;

    int16_t imm = (insn >> 3) & 0b111111;

    if (op_type & 0b1) {
        imm -= 64;
    }

    switch (op_type >> 1) {
    case 0b000:  // LSW
        cdm16_gen_lsw(rd, imm);
        return 2;

    case 0b001:  // LSB
        cdm16_gen_lsb(rd, imm);
        return 2;

    case 0b010:  // LSSB
        cdm16_gen_lssb(rd, imm);
        return 2;

    case 0b011:  // SSW
        cdm16_gen_ssw(rd, imm);
        return 2;

    case 0b100:  // SSB
        cdm16_gen_ssb(rd, imm);
        return 2;

    case 0b101:  // LDI imm6
        cdm16_gen_ldi_short(rd, imm);
        return 2;

    case 0b110:  // ADD imm6
        cdm16_gen_add_imm(rd, imm);
        return 2;

    case 0b111:  // CMP imm6
        cdm16_gen_cmp_imm(rd, imm);
        return 2;
    }

    return 0;
}


static int decode_00000(CPUCdM16State *env,
                        DisasContextBase *db,
                        uint16_t insn)
{
    /* Формат: 00000 0000000 op_type(4) */

    if ((insn & 0xfff0u) != 0) {
        return 0;
    }

    unsigned op_type = insn & 0b1111;

    switch (op_type) {

    case 0b0000:  /* ZERO: недопустимая инструкция */
    case 0b0001:  /* Reserved */
    case 0b0010:  /* Reserved */
    case 0b0011:  /* Reserved */
        cdm16_gen_exception(
            CDM16_EXCP_INVALID_INSN,
            db->pc_next
        );

        db->is_jmp = DISAS_NORETURN;
        return 2;

    case 0b0100:  /* HALT */
        cdm16_gen_halt((db->pc_next + 2) & 0xffff);
        db->is_jmp = DISAS_NORETURN;
        return 2;

    default:
        return 0;
    }
}

static int decode_group_000(CPUCdM16State *env,
                            DisasContextBase *db,
                            uint16_t insn)
{
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
        return 0; // return decode_0001(env, db, insn);
    }

    return 0;
}

int cdm16_decode_instruction(CPUCdM16State *env,
                             DisasContextBase *db,
                             uint16_t insn)
{
    (void)env;
    (void)db;

    switch (insn >> 13) {
    case 0b000:
        return decode_group_000(env, db, insn);

    case 0b001:
        return 0; // Позже decode_001

    case 0b010:
        return 0; // Позже decode_group_010

    case 0b011:
        return decode_011(insn);

    case 0b100:
        return 0; // Позже decode_100

    case 0b101:
        return 0; // Позже decode_group_101

    case 0b110:
    case 0b111:
        return 0; // Позже decode_branch
    }

    return 0;
}