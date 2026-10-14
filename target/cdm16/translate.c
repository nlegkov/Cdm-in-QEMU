#include "qemu/osdep.h"
#include "cpu.h"
#include "tcg/tcg-op.h"
#include "exec/translator.h"
#include "exec/translation-block.h"

typedef struct DisasContext {
    DisasContextBase base;
} DisasContext;

static TCGv_i32 cpu_regs[CDM16_NUM_REGS];
static TCGv_i32 cpu_sp;
static TCGv_i32 cpu_pc;
static TCGv_i32 cpu_ps;

// инициализация представлнения регистров процессора CdM-16 в TCG.
void cdm16_translate_init(void)
{
    static const char * const regnames[CDM16_NUM_REGS] = {
        "r0", "r1", "r2", "r3",
        "r4", "r5", "r6", "r7"
    };

    int i;

    for (i = 0; i < CDM16_NUM_REGS; i++) {
        cpu_regs[i] =
            tcg_global_mem_new_i32(tcg_env,
                                   offsetof(CPUCdM16State, regs[i]),
                                   regnames[i]);
    }

    cpu_sp =
        tcg_global_mem_new_i32(tcg_env,
                               offsetof(CPUCdM16State, sp),
                               "sp");

    cpu_pc =
        tcg_global_mem_new_i32(tcg_env,
                               offsetof(CPUCdM16State, pc),
                               "pc");

    cpu_ps =
        tcg_global_mem_new_i32(tcg_env,
                               offsetof(CPUCdM16State, ps),
                               "ps");
}

// Инициализация контекста дизассемблера для CdM-16 CPU.
static void cdm16_tr_init_disas_context(DisasContextBase *db,
                                        CPUState *cs)
{
    /*
     * Пока настоящего декодера нет:
     * один Translation Block = одна условная инструкция.
     */
    db->max_insns = 1;
}

// Начало Translation Block
static void cdm16_tr_tb_start(DisasContextBase *db, CPUState *cs)
{
}


// начинается гостевая инструкция по адресу pc_next
static void cdm16_tr_insn_start(DisasContextBase *db, CPUState *cs)
{
    tcg_gen_insn_start(db->pc_next, 0, 0);
}


// временная тестовая инструкция
static void cdm16_tr_translate_insn(DisasContextBase *db, CPUState *cs)
{
    db->pc_next = (db->pc_next + 2) & 0xffff;
    db->is_jmp = DISAS_TOO_MANY;
}

// завершение Translation Block
static void cdm16_tr_tb_stop(DisasContextBase *db, CPUState *cs)
{
    tcg_gen_movi_i32(cpu_pc, db->pc_next & 0xffff);
    tcg_gen_exit_tb(NULL, 0);
}

// CdM-16 CPU использует один и тот же набор функций TranslatorOps.
static const TranslatorOps cdm16_tr_ops = {
    .init_disas_context = cdm16_tr_init_disas_context,
    .tb_start = cdm16_tr_tb_start,
    .insn_start = cdm16_tr_insn_start,
    .translate_insn = cdm16_tr_translate_insn,
    .tb_stop = cdm16_tr_tb_stop,
};


// Главная функция, которая вызывается для перевода кода гостевой программы в код хоста.
void cdm16_translate_code(CPUState *cs,
                          TranslationBlock *tb,
                          int *max_insns,
                          vaddr pc,
                          void *host_pc)
{
    DisasContext dc = { };

    translator_loop(cs,
                    tb,
                    max_insns,
                    pc,
                    host_pc,
                    &cdm16_tr_ops,
                    &dc.base,
                    TCG_TYPE_VA);
}