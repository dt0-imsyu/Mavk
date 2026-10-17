std::string bcTextureGate(const std::vector<uint32_t>& vs,const std::vector<uint32_t>& fs,bool validated,ValidationCapture& validation){
    Root root(validation);root.init(validated);mavk::DeviceContext context(root.physical,root.device,root.family);
    uint64_t verified=0,partialBytes=0,decodedBlocks=0;unsigned negatives=0;
    for(bool srgb:{false,true}){
        mavk::compat::BcTexture texture(srgb?mavk::compat::TextureBcFormat::BC1_RGB_SRGB:mavk::compat::TextureBcFormat::BC1_RGB_UNORM,8,8,4);
        std::array<uint8_t,8> pattern{0,248,224,7,0xe4,0xe4,0xe4,0xe4},green{224,7},blue{31,0};
        for(uint32_t y:{0u,4u})for(uint32_t x:{0u,4u})texture.update(0,{x,y,4,4},pattern.data(),8);
        for(uint32_t i=1;i<4;i++)texture.update(i,{0,0,texture.width(i),texture.height(i)},green.data(),8);
        auto initial=mavk::compat::sampledPixels(texture);initial.sampler.maxLod=3;
        {
            mavk::SampledImage gpu(context,initial);gpu.upload(root.queue,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            for(uint32_t i=0;i<4;i++)if(!texture.acknowledge(i,texture.revision(i)))throw std::runtime_error("BC initial revision acknowledgement");
            mavk::bridge::OffscreenSession renderer(context,root.queue,32,32,vs,fs,nullptr,16,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr,nullptr,nullptr,&gpu);
            auto pipelineCount=context.counters().totalPipelineCreates;auto imageCount=context.counters().images;
            auto uploadDirty=[&](uint32_t level){
                auto region=texture.dirtyRegion(level);auto revision=texture.revision(level);const auto& bytes=texture.decodedMip(level);
                std::vector<uint8_t> packed(size_t(region.width)*region.height*4);
                for(uint32_t y=0;y<region.height;y++)std::memcpy(packed.data()+size_t(y)*region.width*4,bytes.data()+((size_t(region.y)+y)*texture.width(level)+region.x)*4,size_t(region.width)*4);
                gpu.update(root.queue,level,{{int32_t(region.x),int32_t(region.y)},{region.width,region.height}},packed);
                if(!texture.acknowledge(level,revision))throw std::runtime_error("BC upload acknowledgement");partialBytes+=packed.size();
            };
            for(unsigned phase=0;phase<3;phase++){
                uint32_t level=phase==2?1:0;
                if(phase==1){texture.update(0,{4,4,4,4},blue.data(),8);if(texture.decodedBlocks()!=8)throw std::runtime_error("BC patch redecoded whole texture");uploadDirty(0);}
                if(phase==2){texture.copyFrom(texture,0,{0,0,4,4},1,0,0);uploadDirty(1);}
                float push[]={float(level),0,0,0},clear[]={1,0,1,1};auto frame=renderer.render(push,clear);const auto& reference=texture.decodedMip(level);
                for(unsigned y=0;y<32;y++)for(unsigned x=0;x<32;x++){
                    size_t at=(y*texture.height(level)/32*texture.width(level)+x*texture.width(level)/32)*4;
                    for(unsigned c=0;c<4;c++){
                        float value=float(reference[at+c])/255;
                        if(srgb&&c<3)value=value<=.04045f?value/12.92f:std::pow((value+.055f)/1.055f,2.4f);
                        if(std::abs(float(frame[(y*32+x)*4+c])-value*255)>1.1f)throw std::runtime_error("BC partial/mip/sRGB hardware mismatch");
                    }
                    verified++;
                }
                if(context.counters().totalPipelineCreates!=pipelineCount||context.counters().images!=imageCount)throw std::runtime_error("BC partial upload recreated GPU image/pipeline");
            }
            auto allocated=context.counters().totalAllocations;std::vector<uint8_t> bytes(64);
            auto reject=[&](uint32_t mip,VkRect2D r,const std::vector<uint8_t>& p){try{gpu.update(root.queue,mip,r,p);}catch(const std::invalid_argument&){negatives++;return;}throw std::runtime_error("BC invalid native update accepted");};
            reject(9,{{0,0},{4,4}},bytes);reject(0,{{7,0},{4,4}},bytes);reject(0,{{0,0},{8,8}},bytes);
            if(context.counters().totalAllocations!=allocated)throw std::runtime_error("BC rejected native update allocated");
        }
        decodedBlocks+=texture.decodedBlocks();if(!context.counters().empty())throw std::runtime_error("BC lifecycle native resources remain");
    }
    if(verified!=6144||partialBytes!=256||decodedBlocks!=18||negatives!=6||context.counters().totalSubmissions!=12||context.counters().totalPipelineCreates!=2)throw std::runtime_error("BC texture gate coverage");
    std::ostringstream out;out<<"{\"result\":\"EMULATED_BC_TEXTURE_LIFECYCLE_PASS\",\"device\":"<<quote(root.properties.deviceName)
        <<",\"verified_pixels\":6144,\"variants\":2,\"gpu_submissions\":12,\"pipeline_creates\":2,\"decoded_blocks\":18,\"partial_upload_bytes\":256,\"negative_tests\":6,\"image_pipeline_retained\":true,\"srgb_native_sampling\":true,\"core_live_zero\":true,\"textureCompressionBC_advertised\":false,\"scope\":\"Common CPU BC texture shadow/mip/copy/dirty revisions,retained native image+pipeline,explicit synchronous partial upload,UNORM+sRGB correctness. NOT async/batched upload,all Vulkan image/view semantics,DXVK,ICD or performance PASS.\"}";
    return out.str();
}
