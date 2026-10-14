#include "qemu/osdep.h"
#include "qemu/units.h"
#include "qapi/error.h"
#include "hw/core/boards.h"
#include "system/address-spaces.h"
#include "qom/object.h"
#include "cpu.h"

#define TYPE_CDM16_MACHINE MACHINE_TYPE_NAME("cdm16")

OBJECT_DECLARE_SIMPLE_TYPE(CDM16MachineState, CDM16_MACHINE)

struct CDM16MachineState {
    MachineState parent_obj;

    CDM16CPU cpu;
};

// создаёт конкретный экземпляр машины.
static void cdm16_machine_init(MachineState *machine)
{
    CDM16MachineState *s = CDM16_MACHINE(machine);
    MemoryRegion *sysmem = get_system_memory();

    /*
     * Пока всё 16-битное адресное пространство отображаем как RAM.
     */
    memory_region_add_subregion(sysmem, 0, machine->ram);

    /*
     * Создаём CdM-16 CPU как дочерний объект машины.
     */
    object_initialize_child(
        OBJECT(machine),
        "cpu",
        &s->cpu,
        machine->cpu_type
    );

    /*
     * Переводим созданный CPU в рабочее состояние.
     * Здесь будет вызван cdm16_cpu_realize().
     */
    qdev_realize(DEVICE(&s->cpu), NULL, &error_fatal);
}

// описывает, какая вообще должна быть CdM-16 машина.
static void cdm16_machine_class_init(ObjectClass *klass, const void *data)
{
    MachineClass *mc = MACHINE_CLASS(klass);

    mc->desc = "CdM-16 minimal machine";

    mc->init = cdm16_machine_init;

    mc->default_cpu_type = TYPE_CDM16_CPU;

    mc->default_cpus = 1;
    mc->min_cpus = 1;
    mc->max_cpus = 1;

    mc->default_ram_size = 64 * KiB;
    mc->default_ram_id = "cdm16.ram";
}

static const TypeInfo cdm16_machine_type_info = {
    .name = TYPE_CDM16_MACHINE,
    .parent = TYPE_MACHINE,

    .instance_size = sizeof(CDM16MachineState),

    .class_init = cdm16_machine_class_init,
};

// регистрирует тип машины в QOM.
static void cdm16_machine_register_types(void)
{
    type_register_static(&cdm16_machine_type_info);
}

// автоматически запускает регистрацию при старте QEMU.
type_init(cdm16_machine_register_types)