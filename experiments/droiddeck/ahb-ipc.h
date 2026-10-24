#pragma once
#include <stdint.h>
typedef struct {
    uint64_t id;
    uint32_t width,height,layers,format,stride;
    int32_t status;
} MavkAhbAck;
_Static_assert(sizeof(MavkAhbAck)==32,"AHB diagnostic acknowledgement ABI");
typedef struct { uint32_t magic,version,async_fence,reserved; } MavkAhbHello;
typedef struct { uint32_t magic,signaled; uint64_t id; } MavkAhbFencePacket;
#define MAVK_AHB_HELLO_MAGIC 0x4d414832u
#define MAVK_AHB_FENCE_MAGIC 0x4d464431u
