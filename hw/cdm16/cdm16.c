#include "qemu/osdep.h"
#include "cpu.h"
#include "hw/core/boards.h"
#include "hw/core/cpu.h"
#include "qapi/error.h"
#include "qemu/units.h"
#include "qemu/error-report.h"
#include "qom/object.h"
#include "system/address-spaces.h"
#include "system/reset.h"
#include "system/system.h"
#include "chardev/char.h"
#include "hw/char/cdm_uart.h"
#include "hw/core/qdev-properties-system.h"
#include "hw/core/sysbus.h"

#define TYPE_CDM16_MACHINE MACHINE_TYPE_NAME("cdm16")

OBJECT_DECLARE_SIMPLE_TYPE(CDM16MachineState, CDM16_MACHINE)

struct CDM16MachineState {
    MachineState parent_obj;

    CDM16CPU cpu;

    bool harvard;
    uint64_t uart_flags_addr;
    uint64_t uart_data_addr;

    MemoryRegion data_ram;
};

static bool cdm16_get_harvard(Object *obj, Error **errp) {
    CDM16MachineState *s = CDM16_MACHINE(obj);

    return s->harvard;
}

static void cdm16_set_harvard(Object *obj, bool value, Error **errp) {
    CDM16MachineState *s = CDM16_MACHINE(obj);

    s->harvard = value;
}

static void cdm16_machine_reset(MachineState *machine, ResetType type) {
    CDM16MachineState *s = CDM16_MACHINE(machine);

    qemu_devices_reset(type);
    cpu_reset(CPU(&s->cpu));
}

// создаёт конкретный экземпляр машины.
static void cdm16_machine_init(MachineState *machine) {

    CDM16MachineState *s = CDM16_MACHINE(machine);
    MemoryRegion *sysmem = get_system_memory();
    DeviceState *uart;
    hwaddr data_base = s->harvard ? 0x10000 : 0;

    if (s->uart_flags_addr > UINT16_MAX ||
        s->uart_data_addr > UINT16_MAX ||
        s->uart_flags_addr == s->uart_data_addr) {
        error_report("UART flags and data addresses must be distinct "
                     "16-bit addresses");
        exit(EXIT_FAILURE);
    }

    /*
     * Основная память:
     * - Harvard: память инструкций
     * - Von Neumann: общая память
     */
    memory_region_add_subregion(sysmem, 0x0000, machine->ram);

    if (s->harvard) {
        /*
         * Дополнительная память данных.
         */
        memory_region_init_ram(&s->data_ram, NULL, "cdm16.data",
                               64 * KiB, &error_fatal);

        memory_region_add_subregion(sysmem, 0x10000, &s->data_ram);
    }

    /*
     * Создаём процессор.
     */
    object_initialize_child(OBJECT(machine), "cpu", &s->cpu, machine->cpu_type);

    /*
     * Передаём конфигурацию CPU.
     */
    s->cpu.harvard = s->harvard;

    qdev_realize(DEVICE(&s->cpu), NULL, &error_fatal);

    uart = qdev_new(TYPE_CDM_UART);
    qdev_prop_set_chr(uart, "chardev", serial_hd(0));
    sysbus_realize_and_unref(SYS_BUS_DEVICE(uart), &error_fatal);
    
    /* MMIO takes precedence over RAM at these two byte addresses. */
    memory_region_add_subregion_overlap(sysmem, data_base + s->uart_flags_addr,
        sysbus_mmio_get_region(SYS_BUS_DEVICE(uart), 0), 1);
    memory_region_add_subregion_overlap(sysmem, data_base + s->uart_data_addr,
        sysbus_mmio_get_region(SYS_BUS_DEVICE(uart), 1), 1);
}

static void cdm16_machine_instance_init(Object *obj) {
    CDM16MachineState *s = CDM16_MACHINE(obj);

    s->uart_flags_addr = 0xf000;
    s->uart_data_addr = 0xf001;
    object_property_add_uint64_ptr(obj, "uart-flags-addr", &s->uart_flags_addr,
                                   OBJ_PROP_FLAG_READWRITE);
    object_property_add_uint64_ptr(obj, "uart-data-addr", &s->uart_data_addr,
                                   OBJ_PROP_FLAG_READWRITE);
}

// описывает, какая вообще должна быть CdM-16 машина.
static void cdm16_machine_class_init(ObjectClass *klass, const void *data) {
    MachineClass *mc = MACHINE_CLASS(klass);

    mc->desc = "CdM-16 minimal machine";

    mc->init = cdm16_machine_init;

    mc->default_cpu_type = TYPE_CDM16_CPU;

    mc->default_cpus = 1;
    mc->min_cpus = 1;
    mc->max_cpus = 1;

    mc->default_ram_size = 64 * KiB;
    mc->default_ram_id = "cdm16.ram";

    mc->reset = cdm16_machine_reset;

    object_class_property_add_bool(klass, "harvard", cdm16_get_harvard,
                                   cdm16_set_harvard);
}

static const TypeInfo cdm16_machine_type_info = {
    .name = TYPE_CDM16_MACHINE,
    .parent = TYPE_MACHINE,

    .instance_size = sizeof(CDM16MachineState),
    .instance_init = cdm16_machine_instance_init,

    .class_init = cdm16_machine_class_init,
};

// регистрирует тип машины в QOM.
static void cdm16_machine_register_types(void) {
    type_register_static(&cdm16_machine_type_info);
}

// автоматически запускает регистрацию при старте QEMU.
type_init(cdm16_machine_register_types)