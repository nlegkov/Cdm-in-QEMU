#ifndef CDM16_CPU_H
#define CDM16_CPU_H

#include "cpu-qom.h"

#ifdef CONFIG_USER_ONLY
#error "CdM-16 supports only system emulation"
#endif

typedef struct CPUArchState {
    uint8_t placeholder;
} CPUCdM16State;

struct ArchCPU {
    CPUState parent_obj;
    CPUCdM16State env;
};

struct CDM16CPUClass {
    CPUClass parent_class;
};

#define CPU_RESOLVING_TYPE TYPE_CDM16_CPU

#endif