#define VK_NO_PROTOTYPES
#include "bc_decode.h"
#include <stdio.h>
static unsigned checks;
static int expect(VkResult value,VkResult wanted){checks++;if(value!=wanted){fprintf(stderr,"bounds check%u: %d != %d\n",checks,value,wanted);return 1;}return 0;}
int main(void){
    VkPhysicalDeviceLimits limits={.maxStorageBufferRange=4096,.maxComputeWorkGroupCount={64,64,64},.maxComputeWorkGroupSize={64,1,1},.maxComputeWorkGroupInvocations=64};
    for(int sign=0;sign<2;sign++){
        struct hybris_bc_region r={.format=sign?VK_FORMAT_BC5_SNORM_BLOCK:VK_FORMAT_BC5_UNORM_BLOCK,.width=4,.height=4,.layers=1,.source_range=16,.destination_range=128};struct hybris_bc_push push;
        if(expect(hybris_bc_prepare(&r,&limits,&push),VK_SUCCESS))return 1;
        r.destination_range=127;if(expect(hybris_bc_prepare(&r,&limits,&push),VK_ERROR_INITIALIZATION_FAILED))return 1;
        r.destination_range=148;r.destination_offset=20;if(expect(hybris_bc_prepare(&r,&limits,&push),VK_SUCCESS))return 1;
        r.destination_range=147;if(expect(hybris_bc_prepare(&r,&limits,&push),VK_ERROR_INITIALIZATION_FAILED))return 1;
        r.destination_range=148;r.source_range=15;if(expect(hybris_bc_prepare(&r,&limits,&push),VK_ERROR_INITIALIZATION_FAILED))return 1;
        r.source_range=16;r.format=sign?VK_FORMAT_BC4_SNORM_BLOCK:VK_FORMAT_BC4_UNORM_BLOCK;r.destination_offset=0;r.destination_range=32;
        if(expect(hybris_bc_prepare(&r,&limits,&push),VK_SUCCESS))return 1;
        r.destination_range=31;if(expect(hybris_bc_prepare(&r,&limits,&push),VK_ERROR_INITIALIZATION_FAILED))return 1;
    }
    printf("BC5_RG32F_BOUNDS_PASS checks=%u\n",checks);return checks==14?0:1;
}
