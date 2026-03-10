#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define VOFA_MAX_CH 8

void vofa_justfloat_send(float *data, uint8_t ch_count);

#ifdef __cplusplus
}
#endif
