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

typedef enum {
    CDM16_EXCP_UNALIGNED_SP = 1,
    CDM16_EXCP_UNALIGNED_PC = 2,
    CDM16_EXCP_INVALID_INSN = 3,
    CDM16_EXCP_DOUBLE_FAULT = 4,
} CdM16Exception;

// 32 так как для QEMU лучше 32
typedef struct CPUArchState {
    uint32_t regs[CDM16_NUM_REGS];

    uint32_t sp;
    uint32_t pc;
    uint32_t ps;
} CPUCdM16State;


struct ArchCPU {
    CPUState parent_obj;
    CPUCdM16State env;
};

struct CDM16CPUClass {
    CPUClass parent_class;

    DeviceRealize parent_realize; // родительская функция realize, которая вызывается после того, как CdM-16 CPU уже создан и QEMU переводит его в рабочее состояние
    ResettablePhases parent_phases; // при сбросе процессора вызывается родительские функции сброса
};

void cdm16_translate_init(void);

void cdm16_translate_code(CPUState *cs,
                          TranslationBlock *tb,
                          int *max_insns,
                          vaddr pc,
                          void *host_pc);

#define CPU_RESOLVING_TYPE TYPE_CDM16_CPU

#endif