#define main mavk_previous_probe_main
#include "hybris_probe.c"
#undef main
#include <stdbool.h>

int main(int argc, char **argv)
{
    if (argc != 2 && argc != 3) return 2;
    bool cached = argc == 3 && !strncmp(argv[2], "cached", 6);
    bool implicit = argc == 3 && !strcmp(argv[2], "cached-implicit");
    bool offset_binding = argc == 3 && strstr(argv[2], "offset");
    bool bind2 = argc == 3 && strstr(argv[2], "bind2");
    setvbuf(stdout, NULL, _IOLBF, 0);
    void *library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!library) { fprintf(stderr, "%s\n", dlerror()); return 2; }
    get = (PFN_vkGetInstanceProcAddr)dlsym(library, "vkGetInstanceProcAddr");
    if (!get) return 2;
    vkCreateInstance = (PFN_vkCreateInstance)get(VK_NULL_HANDLE, "vkCreateInstance");
    VkApplicationInfo app = {.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName="Mavk DDNet buffer-copy control", .apiVersion=VK_API_VERSION_1_1};
    VkInstanceCreateInfo ci = {.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pApplicationInfo=&app};
    OK(vkCreateInstance(&ci, NULL, &instance));
#define LOAD_COPY_CONTROL(n) if (strcmp("vk" #n, "vkCreateInstance")) { \
    vk##n=(PFN_vk##n)get(instance,"vk" #n); if(!vk##n)return fail("resolve " #n,-1); }
    FUNCTIONS(LOAD_COPY_CONTROL)
    PFN_vkCmdCopyBuffer copy = (PFN_vkCmdCopyBuffer)get(instance, "vkCmdCopyBuffer");
    PFN_vkFlushMappedMemoryRanges flush = (PFN_vkFlushMappedMemoryRanges)get(instance, "vkFlushMappedMemoryRanges");
    PFN_vkBindBufferMemory2 bind_memory2 = (PFN_vkBindBufferMemory2)get(instance, "vkBindBufferMemory2");
    if (!copy || !flush || (bind2 && !bind_memory2)) return 2;
    uint32_t count = 0;
    OK(vkEnumeratePhysicalDevices(instance, &count, NULL));
    if (!count || count > 16) return 2;
    VkPhysicalDevice devices[16];
    OK(vkEnumeratePhysicalDevices(instance, &count, devices));
    for (unsigned i=0; i<count; i++) {
        VkPhysicalDeviceProperties p;
        vkGetPhysicalDeviceProperties(devices[i], &p);
        if (p.vendorID == 0x13b5 && p.deviceType != VK_PHYSICAL_DEVICE_TYPE_CPU) {
            physical=devices[i]; printf("COPY_DEVICE=%s\n", p.deviceName); break;
        }
    }
    if (!physical) return 2;
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &count, NULL);
    if (!count || count > 64) return 2;
    VkQueueFamilyProperties families[64];
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &count, families);
    uint32_t family=UINT32_MAX;
    for (unsigned i=0; i<count; i++) if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) { family=i; break; }
    if (family == UINT32_MAX) return 2;
    float priority=1;
    VkDeviceQueueCreateInfo qi={.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex=family,.queueCount=1,.pQueuePriorities=&priority};
    VkDeviceCreateInfo di={.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,.queueCreateInfoCount=1,.pQueueCreateInfos=&qi};
    OK(vkCreateDevice(physical,&di,NULL,&device));
    VkQueue queue; vkGetDeviceQueue(device,family,0,&queue);
    VkBuffer buffers[2]; VkDeviceMemory memories[2]; uint32_t *mapped[2];
    VkMemoryAllocateInfo ai={.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    VkMemoryPropertyFlags properties[2];
    for (unsigned i=0; i<2; i++) {
        VkBufferCreateInfo bi={.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,.size=160,
            .usage=VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT};
        OK(vkCreateBuffer(device,&bi,NULL,&buffers[i]));
        VkMemoryRequirements req; vkGetBufferMemoryRequirements(device,buffers[i],&req);
        ai.memoryTypeIndex=memory_type(req.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            (i && cached ? VK_MEMORY_PROPERTY_HOST_CACHED_BIT : 0));
        if (ai.memoryTypeIndex==UINT32_MAX) return 2;
        VkPhysicalDeviceMemoryProperties mp; vkGetPhysicalDeviceMemoryProperties(physical,&mp);
        properties[i]=mp.memoryTypes[ai.memoryTypeIndex].propertyFlags;
        VkDeviceSize offset = offset_binding ? ((65536 + req.alignment - 1) / req.alignment) * req.alignment : 0;
        ai.allocationSize=req.size+offset;
        OK(vkAllocateMemory(device,&ai,NULL,&memories[i]));
        if (bind2) {
            VkBindBufferMemoryInfo bind={.sType=VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO,
                .buffer=buffers[i],.memory=memories[i],.memoryOffset=offset};
            OK(bind_memory2(device,1,&bind));
        } else {
            OK(vkBindBufferMemory(device,buffers[i],memories[i],offset));
        }
        OK(vkMapMemory(device,memories[i],0,VK_WHOLE_SIZE,0,(void **)&mapped[i]));
        mapped[i]=(uint32_t *)((uint8_t *)mapped[i]+offset);
        printf("COPY_BIND index=%u offset=%llu bind2=%u\n",i,(unsigned long long)offset,bind2);
        for (unsigned j=0; j<40; j++) mapped[i][j]=i ? 0xcdcdcdcd : 0xa5a50000u^j;
        VkMappedMemoryRange range={.sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .memory=memories[i],.size=VK_WHOLE_SIZE};
        OK(flush(device,1,&range));
    }
    VkCommandPool pool;
    VkCommandPoolCreateInfo pi={.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,.queueFamilyIndex=family};
    OK(vkCreateCommandPool(device,&pi,NULL,&pool));
    VkCommandBuffer cmd;
    VkCommandBufferAllocateInfo ca={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool=pool,.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,.commandBufferCount=1};
    OK(vkAllocateCommandBuffers(device,&ca,&cmd));
    VkCommandBufferBeginInfo begin={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    OK(vkBeginCommandBuffer(cmd,&begin));
    VkMemoryBarrier barrier={.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .srcAccessMask=VK_ACCESS_HOST_WRITE_BIT,.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT|VK_ACCESS_TRANSFER_WRITE_BIT};
    vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,1,&barrier,0,NULL,0,NULL);
    VkBufferCopy region={.size=160}; copy(cmd,buffers[0],buffers[1],1,&region);
    barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT; barrier.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
    vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&barrier,0,NULL,0,NULL);
    OK(vkEndCommandBuffer(cmd));
    VkFence fence; VkFenceCreateInfo fi={.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    OK(vkCreateFence(device,&fi,NULL,&fence));
    VkSubmitInfo submit={.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,.commandBufferCount=1,.pCommandBuffers=&cmd};
    OK(vkQueueSubmit(queue,1,&submit,fence));
    OK(vkWaitForFences(device,1,&fence,VK_TRUE,5000000000ull));
    VkMappedMemoryRange range={.sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,.memory=memories[1],.size=VK_WHOLE_SIZE};
    if (!implicit || !(properties[1]&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
        OK(vkInvalidateMappedMemoryRanges(device,1,&range));
    unsigned mismatch=0; for(unsigned j=0;j<40;j++) mismatch += mapped[1][j] != (0xa5a50000u^j);
    printf("COPY_RESULT words=40 mismatch=%u source=0x%x destination=0x%x memory_flags=0x%x/0x%x\n",
        mismatch,mapped[0][0],mapped[1][0],properties[0],properties[1]);
    vkDestroyFence(device,fence,NULL); vkDestroyCommandPool(device,pool,NULL);
    for(unsigned i=0;i<2;i++){vkUnmapMemory(device,memories[i]);vkDestroyBuffer(device,buffers[i],NULL);vkFreeMemory(device,memories[i],NULL);}
    vkDestroyDevice(device,NULL); vkDestroyInstance(instance,NULL);
    return mismatch ? 1 : 0;
}
