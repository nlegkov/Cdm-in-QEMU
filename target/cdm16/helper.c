#include "qemu/osdep.h"
#include "cpu.h"

#include "exec/cpu-common.h"
#include "exec/helper-proto.h"
#include "accel/tcg/cpu-loop.h"
#include "system/runstate.h"

void helper_halt(CPUCdM16State *env)
{
    CPUState *cs = env_cpu(env);

    /* Останавливаем виртуальный CPU */
    cs->halted = 1;

    /* Запрашиваем завершение QEMU */
    qemu_system_shutdown_request(
        SHUTDOWN_CAUSE_GUEST_SHUTDOWN
    );

    /* Выходим из цикла исполнения TCG */
    cs->exception_index = EXCP_HLT;
    cpu_loop_exit(cs);
}

void helper_cdm16_raise_exception(CPUCdM16State *env,
                                  uint32_t exception)
{
    CPUState *cs = env_cpu(env);

    cs->exception_index = exception;

    cpu_loop_exit(cs);
}