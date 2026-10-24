// Private, bounded GPU timestamps around actual no-wait submission batches.
struct GpuSample {
    VkDevice device{};VkCommandPool pool{};VkQueryPool queries{};VkCommandBuffer marks[2]{};
    PFN_vkDestroyCommandPool destroyPool{};PFN_vkDestroyQueryPool destroyQueries{};
    VkQueue queue{};uint32_t bits=0,commands=0;uint64_t sequence=0;bool submitted=false,reported=false;
    const char* api="unknown";
    ~GpuSample(){if(pool)destroyPool(device,pool,nullptr);if(queries)destroyQueries(device,queries,nullptr);}
};
