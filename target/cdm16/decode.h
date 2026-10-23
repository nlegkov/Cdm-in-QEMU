#ifndef CDM16_DECODER_H
#define CDM16_DECODER_H

#include "cpu.h"
#include "exec/translator.h"

int cdm16_decode_instruction(CPUCdM16State *env,
                             DisasContextBase *db,
                             uint16_t insn);

#endif