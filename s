[1mdiff --git a/target/cdm16/cpu.c b/target/cdm16/cpu.c[m
[1mindex 7e3828006f..3b5a70effa 100644[m
[1m--- a/target/cdm16/cpu.c[m
[1m+++ b/target/cdm16/cpu.c[m
[36m@@ -10,6 +10,36 @@[m
 #include "exec/target_page.h"[m
 #include "hw/core/sysemu-cpu-ops.h"[m
 [m
[32m+[m[32m#include "accel/tcg/cpu-ldst.h"[m
[32m+[m
[32m+[m[32m// Обрабатывает исключения, которые произошли во время исполнения кода.[m
[32m+[m[32mstatic void cdm16_cpu_do_interrupt(CPUState *cs)[m
[32m+[m[32m{[m
[32m+[m[32m    CPUCdM16State *env = cpu_env(cs);[m
[32m+[m
[32m+[m[32m    uint32_t exception = cs->exception_index;[m
[32m+[m
[32m+[m[32m    /* Адрес записи в таблице векторов */[m
[32m+[m[32m    uint32_t vector_addr = exception * 4;[m
[32m+[m
[32m+[m[32m    uint32_t old_pc = env->pc & 0xffff;[m
[32m+[m[32m    uint32_t old_ps = env->ps & 0xffff;[m
[32m+[m
[32m+[m[32m    /* Сохраняем PS в стек */[m
[32m+[m[32m    env->sp = (env->sp - 2) & 0xffff;[m
[32m+[m[32m    cpu_stw_le_data(env, env->sp, old_ps);[m
[32m+[m
[32m+[m[32m    /* Сохраняем PC в стек */[m
[32m+[m[32m    env->sp = (env->sp - 2) & 0xffff;[m
[32m+[m[32m    cpu_stw_le_data(env, env->sp, old_pc);[m
[32m+[m
[32m+[m[32m    /* Загружаем адрес обработчика */[m
[32m+[m[32m    env->pc = cpu_lduw_le_data(env, vector_addr);[m
[32m+[m
[32m+[m[32m    /* Загружаем новый PS */[m
[32m+[m[32m    env->ps = cpu_lduw_le_data(env, vector_addr + 2);[m
[32m+[m[32m}[m
[32m+[m
 // записать новое значение PC в процессор CdM-16.[m
 static void cdm16_cpu_set_pc(CPUState *cs, vaddr value)[m
 {[m
[36m@@ -187,6 +217,8 @@[m [mstatic const TCGCPUOps cdm16_tcg_ops = {[m
     .tlb_fill = cdm16_cpu_tlb_fill,[m
     .pointer_wrap = cdm16_cpu_pointer_wrap,[m
 [m
[32m+[m[32m    .do_interrupt = cdm16_cpu_do_interrupt,[m
[32m+[m
     .cpu_exec_interrupt = cdm16_cpu_exec_interrupt,[m
     .cpu_exec_halt = cdm16_cpu_has_work,[m
     .cpu_exec_reset = cpu_reset,[m
[36m@@ -203,7 +235,7 @@[m [mstatic void cdm16_cpu_class_init(ObjectClass *klass, const void *data)[m
     device_class_set_parent_realize([m
         dc,[m
         cdm16_cpu_realize,[m
[31m-        &ccc->parent_realizeсв и[m
[32m+[m[32m        &ccc->parent_realize[m
     );[m
 [m
     resettable_class_set_parent_phases([m
[1mdiff --git a/target/cdm16/cpu.h b/target/cdm16/cpu.h[m
[1mindex d6f4fc45e2..083eaf112a 100644[m
[1m--- a/target/cdm16/cpu.h[m
[1m+++ b/target/cdm16/cpu.h[m
[36m@@ -8,6 +8,11 @@[m
 #endif[m
 [m
 [m
[32m+[m[32m#define CDM16_EXCP_UNALIGNED_SP   1[m
[32m+[m[32m#define CDM16_EXCP_UNALIGNED_PC   2[m
[32m+[m[32m#define CDM16_EXCP_INVALID_INSN   3[m
[32m+[m[32m#define CDM16_EXCP_DOUBLE_FAULT   4[m
[32m+[m
 [m
 #define CDM16_NUM_REGS 8[m
 [m
[1mdiff --git a/target/cdm16/meson.build b/target/cdm16/meson.build[m
[1mindex b18c9bea36..4223691da3 100644[m
[1m--- a/target/cdm16/meson.build[m
[1m+++ b/target/cdm16/meson.build[m
[36m@@ -3,6 +3,8 @@[m [mcdm16_ss = ss.source_set()[m
 cdm16_ss.add(files([m
   'cpu.c',[m
   'translate.c',[m
[32m+[m[32m  'decode.c',[m
[32m+[m[32m  'helper.c',[m
 ))[m
 [m
 target_arch += {[m
[1mdiff --git a/target/cdm16/translate.c b/target/cdm16/translate.c[m
[1mindex 08b9e7fac7..5c4954a74c 100644[m
[1m--- a/target/cdm16/translate.c[m
[1m+++ b/target/cdm16/translate.c[m
[36m@@ -3,6 +3,13 @@[m
 #include "tcg/tcg-op.h"[m
 #include "exec/translator.h"[m
 #include "exec/translation-block.h"[m
[32m+[m[32m#include "decode.h"[m
[32m+[m[32m#include "exec/helper-proto.h"[m
[32m+[m[32m#include "exec/helper-gen.h"[m
[32m+[m
[32m+[m[32m#define HELPER_H "helper.h"[m
[32m+[m[32m#include "exec/helper-info.c.inc"[m
[32m+[m[32m#undef HELPER_H[m
 [m
 typedef struct DisasContext {[m
     DisasContextBase base;[m
[36m@@ -67,21 +74,58 @@[m [mstatic void cdm16_tr_tb_start(DisasContextBase *db, CPUState *cs)[m
 static void cdm16_tr_insn_start(DisasContextBase *db, CPUState *cs)[m
 {[m
     tcg_gen_insn_start(db->pc_next, 0, 0);[m
[32m+[m[41m    [m
 }[m
 [m
[31m-[m
[31m-// временная тестовая инструкция[m
[32m+[m[32m// Перевод гостевой инструкции[m
 static void cdm16_tr_translate_insn(DisasContextBase *db, CPUState *cs)[m
 {[m
[31m-    db->pc_next = (db->pc_next + 2) & 0xffff;[m
[31m-    db->is_jmp = DISAS_TOO_MANY;[m
[32m+[m[32m    CPUCdM16State *env = cpu_env(cs);[m
[32m+[m
[32m+[m[32m    uint16_t insn = translator_lduw_end([m
[32m+[m[32m        env, db, db->pc_next, MO_LE[m
[32m+[m[32m    );[m
[32m+[m
[32m+[m[32m    int size = cdm16_decode_instruction(env, db, insn);[m
[32m+[m
[32m+[m[32m    if (size == 0) {[m
[32m+[m[32m        cdm16_gen_exception([m
[32m+[m[32m            CDM16_EXCP_INVALID_INSN,[m
[32m+[m[32m            db->pc_next[m
[32m+[m[32m        );[m
[32m+[m
[32m+[m[32m        db->pc_next = (db->pc_next + 2) & 0xffff;[m
[32m+[m[32m        db->is_jmp = DISAS_NORETURN;[m
[32m+[m[32m        return;[m
[32m+[m[32m    }[m
[32m+[m
[32m+[m[32m    db->pc_next = (db->pc_next + size) & 0xffff;[m
[32m+[m
[32m+[m[32m    if (db->is_jmp == DISAS_NEXT) {[m
[32m+[m[32m        db->is_jmp = DISAS_TOO_MANY;[m
[32m+[m[32m    }[m
 }[m
 [m
 // завершение Translation Block[m
[31m-static void cdm16_tr_tb_stop(DisasContextBase *db, CPUState *cs)[m
[32m+[m[32mstatic void cdm16_tr_tb_stop(DisasContextBase *db,[m
[32m+[m[32m                             CPUState *cs)[m
 {[m
[31m-    tcg_gen_movi_i32(cpu_pc, db->pc_next & 0xffff);[m
[31m-    tcg_gen_exit_tb(NULL, 0);[m
[32m+[m[32m    switch (db->is_jmp) {[m
[32m+[m[32m    case DISAS_TOO_MANY:[m
[32m+[m[32m        tcg_gen_movi_i32([m
[32m+[m[32m            cpu_pc,[m
[32m+[m[32m            db->pc_next & 0xffff[m
[32m+[m[32m        );[m
[32m+[m[32m        tcg_gen_exit_tb(NULL, 0);[m
[32m+[m[32m        break;[m
[32m+[m
[32m+[m[32m    case DISAS_NORETURN:[m
[32m+[m[32m        /* HALT уже завершает исполнение через helper */[m
[32m+[m[32m        break;[m
[32m+[m
[32m+[m[32m    default:[m
[32m+[m[32m        g_assert_not_reached();[m
[32m+[m[32m    }[m
 }[m
 [m
 // CdM-16 CPU использует один и тот же набор функций TranslatorOps.[m
[36m@@ -111,4 +155,80 @@[m [mvoid cdm16_translate_code(CPUState *cs,[m
                     &cdm16_tr_ops,[m
                     &dc.base,[m
                     TCG_TYPE_VA);[m
[32m+[m[32m}[m
[32m+[m
[32m+[m[32mvoid cdm16_gen_exception(unsigned exception, uint32_t pc)[m
[32m+[m[32m{[m
[32m+[m[32m    /* Адрес инструкции, вызвавшей исключение */[m
[32m+[m[32m    tcg_gen_movi_i32(cpu_pc, pc & 0xffff);[m
[32m+[m
[32m+[m[32m    /* Вызов функции во время исполнения гостевого кода */[m
[32m+[m[32m    gen_helper_cdm16_raise_exception([m
[32m+[m[32m        tcg_env,[m
[32m+[m[32m        tcg_constant_i32(exception)[m
[32m+[m[32m    );[m
[32m+[m[32m}[m
[32m+[m
[32m+[m
[32m+[m
[32m+[m[32mvoid cdm16_gen_halt(uint16_t next_pc)[m
[32m+[m[32m{[m
[32m+[m[32m    tcg_gen_movi_i32(cpu_pc, next_pc);[m
[32m+[m[32m    gen_helper_halt(tcg_env);[m
[32m+[m[32m}[m
[32m+[m
[32m+[m
[32m+[m
[32m+[m
[32m+[m[32mvoid cdm16_gen_ldi_short(unsigned rd, int32_t imm)[m
[32m+[m[32m{[m
[32m+[m[32m    tcg_gen_movi_i32(cpu_regs[rd], (uint16_t)imm);[m
[32m+[m[32m}[m
[32m+[m
[32m+[m
[32m+[m
[32m+[m
[32m+[m[32mvoid cdm16_gen_ssw(unsigned rd, int32_t imm)[m
[32m+[m[32m{[m
[32m+[m[32m    TCGv_i32 addr = tcg_temp_new_i32();[m
[32m+[m
[32m+[m[32m    /* addr = fp + imm * 2 */[m
[32m+[m[32m    tcg_gen_addi_i32([m
[32m+[m[32m        addr,[m
[32m+[m[32m        cpu_regs[CDM16_FP_REG],[m
[32m+[m[32m        imm * 2[m
[32m+[m[32m    );[m
[32m+[m
[32m+[m[32m    tcg_gen_andi_i32(addr, addr, 0xffff);[m
[32m+[m
[32m+[m[32m    /* memory[addr] = rd */[m
[32m+[m[32m    tcg_gen_qemu_st_i32([m
[32m+[m[32m        cpu_regs[rd],[m
[32m+[m[32m        addr,[m
[32m+[m[32m        0,[m
[32m+[m[32m        MO_LEUW[m
[32m+[m[32m    );[m
[32m+[m[32m}[m
[32m+[m
[32m+[m[32mvoid cdm16_gen_lsw(unsigned rd, int32_t imm)[m
[32m+[m[32m{[m
[32m+[m[32m    TCGv_i32 addr = tcg_temp_new_i32();[m
[32m+[m
[32m+[m[32m    /* addr = fp + imm * 2 */[m
[32m+[m[32m    tcg_gen_addi_i32([m
[32m+[m[32m        addr,[m
[32m+[m[32m        cpu_regs[CDM16_FP_REG],[m
[32m+[m[32m        imm * 2[m
[32m+[m[32m    );[m
[32m+[m
[32m+[m[32m    /* Ограничиваем адрес 16 битами */[m
[32m+[m[32m    tcg_gen_andi_i32(addr, addr, 0xffff);[m
[32m+[m
[32m+[m[32m    /* rd = memory[addr] */[m
[32m+[m[32m    tcg_gen_qemu_ld_i32([m
[32m+[m[32m        cpu_regs[rd],[m
[32m+[m[32m        addr,[m
[32m+[m[32m        0,[m
[32m+[m[32m        MO_LEUW[m
[32m+[m[32m    );[m
 }[m
\ No newline at end of file[m
