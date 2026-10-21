#include "qemu/osdep.h"
#include "cpu.h"
#include "qapi/error.h"
#include "qemu/qemu-print.h"
#include "accel/tcg/cpu-ops.h"

#include "accel/tcg/cpu-ops.h"
#include "exec/cputlb.h"
#include "exec/page-protection.h"
#include "exec/target_page.h"
#include "hw/core/sysemu-cpu-ops.h"

#include "accel/tcg/cpu-ldst.h"

#include "system/address-spaces.h"
#include "system/memory.h"

// Обрабатывает исключения, которые произошли во время исполнения кода.
static void cdm16_cpu_do_interrupt(CPUState *cs)
{
    CPUCdM16State *env = cpu_env(cs);

    uint32_t exception = cs->exception_index;

    /* Адрес записи в таблице векторов */
    uint32_t vector_addr = exception * 4;

    uint32_t old_pc = env->pc & 0xffff;
    uint32_t old_ps = env->ps & 0xffff;

    /* Сохраняем PS в стек */
    env->sp = (env->sp - 2) & 0xffff;
    cpu_stw_le_data(env, env->sp, old_ps);

    /* Сохраняем PC в стек */
    env->sp = (env->sp - 2) & 0xffff;
    cpu_stw_le_data(env, env->sp, old_pc);

    /* Загружаем адрес обработчика */
    env->pc = cpu_lduw_le_data(env, vector_addr);

    /* Загружаем новый PS */
    env->ps = cpu_lduw_le_data(env, vector_addr + 2);
}

// записать новое значение PC в процессор CdM-16.
static void cdm16_cpu_set_pc(CPUState *cs, vaddr value)
{
    // приведение абстрактного процессора к CDM16CPU
    CDM16CPU *cpu = CDM16_CPU(cs);

    // записываем новое значение в регистр pc процессора CdM-16
    cpu->env.pc = value & 0xffff;
}

// получить текущее значение PC процессора CdM-16.
static vaddr cdm16_cpu_get_pc(CPUState *cs)
{
    // приведение абстрактного процессора к CDM16CPU
    CDM16CPU *cpu = CDM16_CPU(cs);

    // возвращаем текущее значение регистра pc процессора CdM-16
    return cpu->env.pc & 0xffff;
}

// состояние CdM-16 после reset.
static void cdm16_cpu_reset_hold(Object *obj, ResetType type)
{
    CPUState *cs = CPU(obj);
    CDM16CPUClass *ccc = CDM16_CPU_GET_CLASS(obj);
    CPUCdM16State *env = cpu_env(cs);

    if (ccc->parent_phases.hold) {
        ccc->parent_phases.hold(obj, type);
    }

    memset(env, 0, sizeof(*env));


    uint8_t vector[4];

    MemTxResult result = address_space_read(
        &address_space_memory,
        0x0000,
        MEMTXATTRS_UNSPECIFIED,
        vector,
        sizeof(vector)
    );

    if (result == MEMTX_OK) {
        env->pc = (uint32_t)vector[0]
                | ((uint32_t)vector[1] << 8);

        env->ps = (uint32_t)vector[2]
                | ((uint32_t)vector[3] << 8);
    }

    /*fprintf(stderr,
        "RESET: result=%d vector=%02x %02x %02x %02x "
        "PC=0x%04x SP=0x%04x PS=0x%04x\n",
        result,
        vector[0], vector[1], vector[2], vector[3],
        env->pc, env->sp, env->ps
    );*/
}

// Отвечает за момент, когда объект CPU уже создан и QEMU переводит его в рабочее состояние.
static void cdm16_cpu_realize(DeviceState *dev, Error **errp)
{
    CPUState *cs = CPU(dev);
    CDM16CPUClass *ccc = CDM16_CPU_GET_CLASS(dev);
    Error *local_err = NULL;

    cpu_common_realize(cs, &local_err);
    if (local_err != NULL) {
        error_propagate(errp, local_err);
        return;
    }

    qemu_init_vcpu(cs);
    cpu_reset(cs);

    ccc->parent_realize(dev, errp);
}

// Вывод состояния процессора CdM-16 в файл f.
static void cdm16_cpu_dump_state(CPUState *cs, FILE *f, int flags)
{
    CPUCdM16State *env = cpu_env(cs);

    (void)flags;

    qemu_fprintf(f,
                 "PC=0x%04x SP=0x%04x PS=0x%04x "
                 "[I=%u C=%u V=%u Z=%u N=%u]\n",
                 (unsigned)env->pc,
                 (unsigned)env->sp,
                 (unsigned)env->ps,
                 (env->ps & CDM16_PSR_I) != 0,
                 (env->ps & CDM16_PSR_C) != 0,
                 (env->ps & CDM16_PSR_V) != 0,
                 (env->ps & CDM16_PSR_Z) != 0,
                 (env->ps & CDM16_PSR_N) != 0);

    qemu_fprintf(f,
                 "r0=0x%04x r1=0x%04x r2=0x%04x r3=0x%04x\n",
                 (unsigned)env->regs[0],
                 (unsigned)env->regs[1],
                 (unsigned)env->regs[2],
                 (unsigned)env->regs[3]);

    qemu_fprintf(f,
                 "r4=0x%04x r5=0x%04x r6=0x%04x r7(fp)=0x%04x\n",
                 (unsigned)env->regs[4],
                 (unsigned)env->regs[5],
                 (unsigned)env->regs[6],
                 (unsigned)env->regs[7]);
}

// Возвращает состояние процессора CdM-16 для TCG.
static TCGTBCPUState cdm16_get_tb_cpu_state(CPUState *cs)
{
    CPUCdM16State *env = cpu_env(cs);

    return (TCGTBCPUState) {
        .pc = env->pc & 0xffff,
        .flags = 0,
    };
}

// CdM-16 CPU не поддерживает MMU, поэтому всегда возвращаем 0.
static int cdm16_cpu_mmu_index(CPUState *cs, bool ifetch)
{
    (void)cs;
    (void)ifetch;
    return 0;
}

// Пока без поддержки прерываний, поэтому всегда возвращаем false.
static bool cdm16_cpu_has_work(CPUState *cs)
{
    (void)cs;

    return false;
}

// Отвечает за обработку прерываний.
static bool cdm16_cpu_exec_interrupt(CPUState *cs, int interrupt_request)
{
    (void)cs;
    (void)interrupt_request;

    return false;
}

// Обрабатывает изменения указателей в адресном пространстве CdM-16, ограничивая их значением 16-бит.
static vaddr cdm16_cpu_pointer_wrap(CPUState *cs, int mmu_idx,
                                    vaddr result, vaddr base)
{
    (void)cs;
    (void)mmu_idx;
    (void)base;

    return (uint16_t)result & 0xffff;
}

// Обрабатывает заполнение TLB для CdM-16 CPU, ограничивая адреса значением 16-бит.
static bool cdm16_cpu_tlb_fill(CPUState *cs, vaddr addr, int size,
                               MMUAccessType access_type, int mmu_idx,
                               bool probe, uintptr_t retaddr)
{
    vaddr address;

    (void)size;
    (void)access_type;
    (void)probe;
    (void)retaddr;

    address = (addr & 0xffff) & TARGET_PAGE_MASK;

    tlb_set_page(cs,
                 address,
                 address,
                 PAGE_READ | PAGE_WRITE | PAGE_EXEC,
                 mmu_idx,
                 TARGET_PAGE_SIZE);

    return true;
}

// CPUClass -> sysemu_ops -> cdm16_sysemu_ops -> cdm16_cpu_has_work
static const SysemuCPUOps cdm16_sysemu_ops = {
    .has_work = cdm16_cpu_has_work,
};


// Все CdM-16 CPU используют один и тот же набор функций TCG.
static const TCGCPUOps cdm16_tcg_ops = {
    .guest_default_memory_order = TCG_MO_ALL,
    .mttcg_supported = false,

    .initialize = cdm16_translate_init,
    .translate_code = cdm16_translate_code,
    .get_tb_cpu_state = cdm16_get_tb_cpu_state,

    .mmu_index = cdm16_cpu_mmu_index,
    .tlb_fill = cdm16_cpu_tlb_fill,
    .pointer_wrap = cdm16_cpu_pointer_wrap,

    .do_interrupt = cdm16_cpu_do_interrupt,

    .cpu_exec_interrupt = cdm16_cpu_exec_interrupt,
    .cpu_exec_halt = cdm16_cpu_has_work,
    .cpu_exec_reset = cpu_reset,
};

// все CdM-16 CPU работают вот так
static void cdm16_cpu_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    CPUClass *cc = CPU_CLASS(klass);
    CDM16CPUClass *ccc = CDM16_CPU_CLASS(klass);
    ResettableClass *rc = RESETTABLE_CLASS(klass);

    device_class_set_parent_realize(
        dc,
        cdm16_cpu_realize,
        &ccc->parent_realize
    );

    resettable_class_set_parent_phases(
        rc,
        NULL,
        cdm16_cpu_reset_hold,
        NULL,
        &ccc->parent_phases
    );

    cc->set_pc = cdm16_cpu_set_pc;
    cc->get_pc = cdm16_cpu_get_pc;
    cc->dump_state = cdm16_cpu_dump_state;

    cc->sysemu_ops = &cdm16_sysemu_ops;
    cc->tcg_ops = &cdm16_tcg_ops;
}

// Это описание нового типа для QOM.
// "Я хочу создать новый тип объекта".
static const TypeInfo cdm16_cpu_type_info = {
    .name = TYPE_CDM16_CPU, // имя нового типа объекта
    .parent = TYPE_CPU, // родительский тип объекта

    .instance_size = sizeof(CDM16CPU), // размер экземпляра нового типа объекта
    .instance_align = __alignof(CDM16CPU), // выравнивание экземпляра нового типа объекта

    .class_size = sizeof(CDM16CPUClass), // размер класса нового типа объекта
    .class_init = cdm16_cpu_class_init, // когда QOM создаст класс cdm16-cpu, вызови cdm16_cpu_class_init().
};

// Регистрируем новый тип объекта в QOM, чтобы QOM знал о нашем новом типе объекта.
static void cdm16_cpu_register_types(void)
{
    type_register_static(&cdm16_cpu_type_info);
}

// Инициализация типов QOM. QOM вызывает эту функцию, чтобы зарегистрировать все типы объектов, которые мы создали.
type_init(cdm16_cpu_register_types)