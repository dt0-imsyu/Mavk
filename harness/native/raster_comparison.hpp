// Real native source-over and dynamic clipping through the shared renderer.
std::string rasterGate(const std::vector<uint32_t>& vs,const std::vector<uint32_t>& fs,bool validated,ValidationCapture& validation){
    Root root(validation);root.init(validated);mavk::DeviceContext context(root.physical,root.device,root.family);
    uint64_t verified=0;unsigned negatives=0;
    const float clear[]={24.f/255,96.f/255,168.f/255,192.f/255},push[]={0,0,0,0};
    const VkRect2D clip{{5,7},{19,13}};
    const VkRect2D invalid[]={{{-1,0},{1,1}},{{0,0},{0,1}},{{31,0},{2,1}},{{0,32},{1,1}},{{1,0},{UINT32_MAX,1}}};
    for(unsigned alpha:{0u,128u,255u})for(bool blend:{false,true}){
        {
            mavk::SampledPixels texture;texture.width=texture.height=texture.levels=1;
            texture.rgba={196,72,32,uint8_t(alpha)};
            mavk::bridge::RasterOptions raster;raster.sourceOver=blend;
            mavk::bridge::OffscreenSession renderer(context,root.queue,32,32,vs,fs,nullptr,16,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr,nullptr,&texture,nullptr,&raster);
            auto allocations=context.counters().totalAllocations;
            for(const auto& bad:invalid){try{renderer.render(push,clear,nullptr,nullptr,&bad);throw std::runtime_error("Invalid clip accepted");}catch(const std::invalid_argument&){negatives++;}}
            // null restores full clipping; no state may leak from frame1 to2.
            for(unsigned phase=0;phase<3;phase++){
                auto pixels=renderer.render(push,clear,nullptr,nullptr,phase==1?&clip:nullptr);
                if(pixels.size()!=4096)throw std::runtime_error("Raster readback length");
                for(unsigned y=0;y<32;y++)for(unsigned x=0;x<32;x++){
                    bool inside=phase!=1||(x>=5&&x<24&&y>=7&&y<20);
                    for(unsigned channel=0;channel<4;channel++){
                        float expected=clear[channel]*255;
                        if(inside){float src=texture.rgba[channel];expected=blend?(channel==3?float(alpha):src*(alpha/255.f))+clear[channel]*(255-alpha):src;}
                        if(std::abs(float(pixels[(y*32+x)*4+channel])-expected)>1.1f)throw std::runtime_error("Raster actual pixel mismatch alpha="+std::to_string(alpha)+" blend="+std::to_string(blend)+" phase="+std::to_string(phase)+" channel="+std::to_string(channel));
                    }
                    verified++;
                }
                if(context.counters().totalAllocations!=allocations)throw std::runtime_error("Raster allocated GPU memory per frame");
            }
        }
        if(!context.counters().empty())throw std::runtime_error("Raster resources remain");
    }
    if(verified!=18432||negatives!=30||context.counters().totalSubmissions!=18||context.counters().totalPipelineCreates!=6)throw std::runtime_error("Raster coverage mismatch");
    std::ostringstream out;out<<"{\"result\":\"NATIVE_SHARED_RASTER_PASS\",\"device\":"<<quote(root.properties.deviceName)<<",\"verified_pixels\":"<<verified<<",\"gpu_submissions\":18,\"pipeline_creates\":6,\"negative_tests\":"<<negatives<<",\"source_alpha_cases\":[0,128,255],\"blend_modes\":[\"replace\",\"straight_alpha_source_over\"],\"clip_phases\":[\"full\",\"partial\",\"full_reset\"],\"per_frame_gpu_allocations\":0,\"core_live_zero\":true,\"scope\":\"Native RGBA8 straight-alpha blend and dynamic scissor; exact CPU pixel oracle and negative bounds. Not arbitrary blend equations, guest ICD, UI library, game or CTS PASS.\"}";return out.str();
}
