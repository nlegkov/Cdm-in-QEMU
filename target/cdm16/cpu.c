#include "qemu/osdep.h"
#include "cpu.h"


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

// все CdM-16 CPU работают вот так
static void cdm16_cpu_class_init(ObjectClass *klass, const void *data)
{
    CPUClass *cc = CPU_CLASS(klass);
    CDM16CPUClass *ccc = CDM16_CPU_CLASS(klass);
    ResettableClass *rc = RESETTABLE_CLASS(klass);

    resettable_class_set_parent_phases(
        rc,
        NULL,
        cdm16_cpu_reset_hold,
        NULL,
        &ccc->parent_phases
    );

    cc->set_pc = cdm16_cpu_set_pc;
    cc->get_pc = cdm16_cpu_get_pc;
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