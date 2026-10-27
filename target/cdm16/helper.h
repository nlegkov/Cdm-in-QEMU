DEF_HELPER_1(halt, noreturn, env)

DEF_HELPER_2(cdm16_raise_exception, noreturn, env, i32)

DEF_HELPER_3(cdm16_debug_addsp, void, i32, i32, i32)