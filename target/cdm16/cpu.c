#include "qemu/osdep.h"
#include "cpu.h"
#include "qapi/error.h"
#include "qemu/qemu-print.h"

// записать новое значение PC в процессор CdM-16.
static void cdm16_cpu_set_pc(CPUState *cs, vaddr value)
{
    // приведение абстрактного процессора к CDM16CPU
    CDM16CPU *cpu = CDM16_CPU(cs);

    // записываем новое значение в регистр pc процессора CdM-16
    cpu->env.pc = (uint16_t)value;
}

// получить текущее значение PC процессора CdM-16.
static vaddr cdm16_cpu_get_pc(CPUState *cs)
{
    // приведение абстрактного процессора к CDM16CPU
    CDM16CPU *cpu = CDM16_CPU(cs);

    // возвращаем текущее значение регистра pc процессора CdM-16
    return cpu->env.pc;
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