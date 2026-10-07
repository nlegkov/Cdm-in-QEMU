#include "qemu/osdep.h"
#include "cpu.h"

static const TypeInfo cdm16_cpu_type_info = {
    .name = TYPE_CDM16_CPU,
    .parent = TYPE_CPU,

    .instance_size = sizeof(CDM16CPU),
    .instance_align = __alignof(CDM16CPU),

    .class_size = sizeof(CDM16CPUClass),
};

static void cdm16_cpu_register_types(void)
{
    type_register_static(&cdm16_cpu_type_info);
}

type_init(cdm16_cpu_register_types)