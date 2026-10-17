// A/B actual shared renderer, bounded workload, not desktop/visible FPS.
std::string reuseGate(const std::vector<uint32_t>& vs,const std::vector<uint32_t>& fs,bool validated,ValidationCapture& validation){
    Root root(validation);root.init(validated);mavk::DeviceContext context(root.physical,root.device,root.family);
    mavk::SampledPixels texture;texture.width=texture.height=8;texture.levels=4;texture.sampler.maxLod=3;
    auto mips=mavk::rgbaMipLayout(8,8,4);texture.rgba.resize(size_t(mips.back().offset+mips.back().bytes));
    for(unsigned level=0;level<4;level++)for(size_t at=size_t(mips[level].offset);at<mips[level].offset+mips[level].bytes;at+=4){texture.rgba[at]=uint8_t(17+level*43);texture.rgba[at+1]=uint8_t(29+level*37);texture.rgba[at+2]=uint8_t(53+level*23);texture.rgba[at+3]=255;}
    uint64_t verified=0;unsigned rejects=0;std::vector<double> oldTimes,newTimes;std::vector<std::vector<uint8_t>> frames;
    auto verify=[&](const std::vector<uint8_t>& pixels,unsigned level){
        if(pixels.size()!=4096)throw std::runtime_error("Reuse readback size");int expected[]={int(17+level*43),int(29+level*37),int(53+level*23),255};
        for(unsigned p=0;p<1024;p++){for(unsigned ch=0;ch<4;ch++)if(std::abs(int(pixels[p*4+ch])-expected[ch])>1)throw std::runtime_error("Reuse actual pixel mismatch");verified++;}
    };
    const float clear[]={1,0,1,1};
    for(unsigned j=0;j<16;j++){
        float state[]={float(j%4),0,0,0};auto start=Clock::now();
        auto frame=mavk::bridge::drawOffscreen(context,root.queue,32,32,vs,fs,state,clear,nullptr,16,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr,nullptr,&texture);
        oldTimes.push_back(ms(start,Clock::now()));verify(frame,j%4);frames.push_back(std::move(frame));
        if(!context.counters().empty())throw std::runtime_error("Recreate resources remain");
    }
    uint64_t baselineCreates=context.counters().totalPipelineCreates,baselineAllocations=context.counters().totalAllocations;
    double initialization=0;uint64_t retainedCreates=0,retainedAllocations=0;
    {
        auto start=Clock::now();
        mavk::bridge::OffscreenSession renderer(context,root.queue,32,32,vs,fs,nullptr,16,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr,nullptr,&texture);
        initialization=ms(start,Clock::now());retainedAllocations=context.counters().totalAllocations-baselineAllocations;
        uint64_t allocated=context.counters().totalAllocations;
        float state[]={0,0,0,0};
        try{renderer.render(state,nullptr);throw std::runtime_error("Null clear accepted");}catch(const std::invalid_argument&){rejects++;}
        std::vector<uint8_t> invalidUniform(16);
        try{renderer.render(state,clear,&invalidUniform);throw std::runtime_error("Unbound UBO accepted");}catch(const std::invalid_argument&){rejects++;}
        for(unsigned j=0;j<16;j++){
            state[0]=float(j%4);start=Clock::now();auto frame=renderer.render(state,clear);newTimes.push_back(ms(start,Clock::now()));
            verify(frame,j%4);if(frame!=frames[j])throw std::runtime_error("Recreate/retained pixels differ");
            if(context.counters().totalAllocations!=allocated)throw std::runtime_error("Retained renderer allocated GPU memory per frame");
        }
        retainedCreates=context.counters().totalPipelineCreates-baselineCreates;
    }
    if(!context.counters().empty()||context.counters().totalSubmissions!=32||baselineCreates!=16||retainedCreates!=1||verified!=32768||rejects!=2)throw std::runtime_error("Reuse gate coverage/teardown");
    auto median=[](std::vector<double> v){std::sort(v.begin(),v.end());return (v[7]+v[8])*.5;};
    std::ostringstream out;out<<"{\"result\":\"NATIVE_SHARED_GRAPHICS_REUSE_PASS\",\"device\":"<<quote(root.properties.deviceName)<<",\"verified_pixels\":"<<verified<<",\"frames_per_arm\":16,\"distinct_lod_outputs\":4,\"gpu_submissions\":32,\"negative_tests\":2,\"baseline_pipeline_creates\":"<<baselineCreates<<",\"retained_pipeline_creates\":"<<retainedCreates<<",\"baseline_allocations\":"<<baselineAllocations<<",\"retained_allocations\":"<<retainedAllocations<<",\"retained_per_frame_allocations\":0,\"retained_initialize_ms\":"<<initialization<<",\"baseline_median_ms\":"<<median(oldTimes)<<",\"retained_median_ms\":"<<median(newTimes)<<",\"core_live_zero\":true,\"scope\":\"32x32 RGBA4mip shared renderer A/B with SyncVal. Cached pipeline/resources,changing push values,exact pixels. Preliminary sequential timing includes validation; NOT production/guest/visible FPS or statistical benchmark.\"}";return out.str();
}
