#ifndef CDM16_CPU_H
#define CDM16_CPU_H

#include "cpu-qom.h"

#ifdef CONFIG_USER_ONLY
#error "CdM-16 supports only system emulation"
#endif



#define CDM16_NUM_REGS 8

#define CDM16_PSR_N (1u << 0)
#define CDM16_PSR_Z (1u << 1)
#define CDM16_PSR_V (1u << 2)
#define CDM16_PSR_C (1u << 3)
#define CDM16_PSR_I (1u << 15)

#define CDM16_FP_REG 7

typedef struct CPUArchState {
    uint16_t regs[CDM16_NUM_REGS]; // 8 Общих регитров 0-6 + 1 регистр FP

    uint16_t sp; // указатель стека
    uint16_t pc; // счетчик команд
    uint16_t ps; // регистр состояния процессора

} CPUCdM16State;


struct ArchCPU {
    CPUState parent_obj;
    CPUCdM16State env;
};

struct CDM16CPUClass {
    CPUClass parent_class;

    ResettablePhases parent_phases; // при сбросе процессора вызывается родительские функции сброса
};

#define CPU_RESOLVING_TYPE TYPE_CDM16_CPU

#endif