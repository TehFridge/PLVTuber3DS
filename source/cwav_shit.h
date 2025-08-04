#ifndef CWAV_SHIT_H
#define CWAV_SHIT_H
#include <cwav.h>
#include <ncsnd.h>
#include <curl/curl.h>
#include <malloc.h>



size_t cwavExtractDSPADPCM(CWAV* cwav, int channel, s16* out, size_t maxSamples);
size_t cwavExtractPcmSamples(CWAV* cwav, int channel, s16* out, size_t maxSamples);

void print_u32_binary(u32 val);

typedef struct {
    char* filename;
    CWAV* cwav;
} CWAVInfo;

extern CWAV* sfx;
void populateCwavList();

void freeCwavList();

#endif