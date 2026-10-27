#include "qemu/osdep.h"

#include "cpu.h"
#include "exec/translation-block.h"
#include "exec/translator.h"
#include "exec/helper-proto.h"
#include "exec/helper-gen.h"
#include "tcg/tcg-op.h"

#include "translate.h"
#include "decode.h"

#define HELPER_H "helper.h"
#include "exec/helper-info.c.inc"
#undef HELPER_H

typedef struct DisasContext {
    DisasContextBase base;
} DisasContext;

static TCGv_i32 cpu_regs[CDM16_NUM_REGS];
static TCGv_i32 cpu_sp;
static TCGv_i32 cpu_pc;
static TCGv_i32 cpu_ps;


// Генерация кода для записи значения в регистр (PC или SP) с проверкой на
// выравнивание
static void cdm16_gen_write_aligned(TCGv_i32 dst, TCGv_i32 value,
                                    unsigned exception, uint32_t fault_pc) {
    TCGLabel *aligned = gen_new_label();

    TCGv_i32 value16 = tcg_temp_new_i32();
    TCGv_i32 odd = tcg_temp_new_i32();

    /* Значение регистра должно быть 16-битным */
    tcg_gen_andi_i32(value16, value, 0xffff);

    /* Проверяем младший бит */
    tcg_gen_andi_i32(odd, value16, 1);

    /* Если значение чётное, переходим к записи */
    tcg_gen_brcondi_i32(TCG_COND_EQ, odd, 0, aligned);

    /* Нечётное значение: вызываем исключение */
    cdm16_gen_exception(exception, fault_pc);

    gen_set_label(aligned);

    /* Запись выполняется только для чётного значения */
    tcg_gen_mov_i32(dst, value16);
}
// Генерация кода для записи значения в регистр PC
// value - новое значение регистра PC
// fault_pc - адрес инструкции, вызвавшей запись в PC
void cdm16_gen_write_pc(TCGv_i32 value, uint32_t fault_pc) {
    cdm16_gen_write_aligned(cpu_pc, value, CDM16_EXCP_UNALIGNED_PC, fault_pc);
}
// Генерация кода для записи значения в регистр SP
// value - новое значение регистра SP
// fault_pc - адрес инструкции, вызвавшей запись в SP
void cdm16_gen_write_sp(TCGv_i32 value, uint32_t fault_pc) {
    cdm16_gen_write_aligned(cpu_sp, value, CDM16_EXCP_UNALIGNED_SP, fault_pc);
}

TCGv_i32 cdm16_tcg_reg(unsigned reg) {
    g_assert(reg < CDM16_NUM_REGS);
    return cpu_regs[reg];
}
TCGv_i32 cdm16_tcg_sp(void) { return cpu_sp; }
TCGv_i32 cdm16_tcg_pc(void) { return cpu_pc; }
TCGv_i32 cdm16_tcg_ps(void) { return cpu_ps; }

// инициализация представлнения регистров процессора CdM-16 в TCG.
void cdm16_translate_init(void) {
    static const char *const regnames[CDM16_NUM_REGS] = {
        "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7"};

    int i;

    for (i = 0; i < CDM16_NUM_REGS; i++) {
        cpu_regs[i] = tcg_global_mem_new_i32(
            tcg_env, offsetof(CPUCdM16State, regs[i]), regnames[i]);
    }

    cpu_sp = tcg_global_mem_new_i32(tcg_env, offsetof(CPUCdM16State, sp), "sp");

    cpu_pc = tcg_global_mem_new_i32(tcg_env, offsetof(CPUCdM16State, pc), "pc");

    cpu_ps = tcg_global_mem_new_i32(tcg_env, offsetof(CPUCdM16State, ps), "ps");
}

// Инициализация контекста дизассемблера для CdM-16 CPU.
static void cdm16_tr_init_disas_context(DisasContextBase *db, CPUState *cs) {
    /*
     * Пока настоящего декодера нет:
     * один Translation Block = одна условная инструкция.
     */
    db->max_insns = 1;
}

// Начало Translation Block
static void cdm16_tr_tb_start(DisasContextBase *db, CPUState *cs) {}

// начинается гостевая инструкция по адресу pc_next
static void cdm16_tr_insn_start(DisasContextBase *db, CPUState *cs) {
    tcg_gen_insn_start(db->pc_next, 0, 0);
}

// Перевод гостевой инструкции
static void cdm16_tr_translate_insn(DisasContextBase *db, CPUState *cs) {
    CPUCdM16State *env = cpu_env(cs);

    uint32_t pc = db->pc_next & 0xffff;

    if (pc & 1) {
        cdm16_gen_exception(CDM16_EXCP_UNALIGNED_PC, pc);

        db->is_jmp = DISAS_NORETURN;
        return;
    }

    uint16_t insn = translator_lduw_end(env, db, db->pc_next, MO_LE);

    int size = cdm16_decode_instruction(env, db, insn);

    if (size == 0) {
        cdm16_gen_exception(CDM16_EXCP_INVALID_INSN, db->pc_next);

        db->pc_next = (db->pc_next + 2) & 0xffff;
        db->is_jmp = DISAS_NORETURN;
        return;
    }

    db->pc_next = (db->pc_next + size) & 0xffff;

    if (db->is_jmp == DISAS_NEXT) {
        db->is_jmp = DISAS_TOO_MANY;
    }
}

// завершение Translation Block
static void cdm16_tr_tb_stop(DisasContextBase *db, CPUState *cs) {
    switch (db->is_jmp) {
    case DISAS_TOO_MANY:
        tcg_gen_movi_i32(cpu_pc, db->pc_next & 0xffff);
        tcg_gen_exit_tb(NULL, 0);
        break;

    case DISAS_NORETURN:
        /*
         * Инструкция сама завершает TB:
         * HALT через helper,
         * JSRR/STPC через tcg_gen_exit_tb().
         */
        break;

    default:
        g_assert_not_reached();
    }
}

// CdM-16 CPU использует один и тот же набор функций TranslatorOps.
static const TranslatorOps cdm16_tr_ops = {
    .init_disas_context = cdm16_tr_init_disas_context,
    .tb_start = cdm16_tr_tb_start,
    .insn_start = cdm16_tr_insn_start,
    .translate_insn = cdm16_tr_translate_insn,
    .tb_stop = cdm16_tr_tb_stop,
};

// Главная функция, которая вызывается для перевода кода гостевой программы в
// код хоста.
void cdm16_translate_code(CPUState *cs, TranslationBlock *tb, int *max_insns,
                          vaddr pc, void *host_pc) {
    DisasContext dc = {};

    translator_loop(cs, tb, max_insns, pc, host_pc, &cdm16_tr_ops, &dc.base,
                    TCG_TYPE_VA);
}

void cdm16_gen_exception(unsigned exception, uint32_t pc) {
    /* Адрес инструкции, вызвавшей исключение */
    tcg_gen_movi_i32(cpu_pc, pc & 0xffff);

    /* Вызов функции во время исполнения гостевого кода */
    gen_helper_cdm16_raise_exception(tcg_env, tcg_constant_i32(exception));
}

void cdm16_gen_halt(uint16_t next_pc) {
    tcg_gen_movi_i32(cpu_pc, next_pc);
    gen_helper_halt(tcg_env);
}