// Compatibility CPU decode -> existing Core upload/graphics -> real Mali.
// Does not expose native BC formats or the all-BC Vulkan feature.
std::string bcGate(const std::vector<uint32_t>& vs,const std::vector<uint32_t>& fs,bool validated,ValidationCapture& validation){
    Root root(validation);root.init(validated);mavk::DeviceContext context(root.physical,root.device,root.family);
    uint64_t verified=0;unsigned jobs=0;std::ostringstream samples;samples<<'[';
    for(auto format:{mavk::compat::BcFormat::BC1_RGB,mavk::compat::BcFormat::BC1_RGBA,mavk::compat::BcFormat::BC2_RGBA,mavk::compat::BcFormat::BC3_RGBA}){
        mavk::SampledPixels texture;texture.width=texture.height=8;texture.levels=4;texture.sampler.maxLod=3;
        auto layout=mavk::rgbaMipLayout(8,8,4);texture.rgba.resize(size_t(layout.back().offset+layout.back().bytes));
        auto begin=Clock::now();
        for(unsigned level=0;level<4;level++){
            const auto& m=layout[level];bool alpha=format==mavk::compat::BcFormat::BC2_RGBA||format==mavk::compat::BcFormat::BC3_RGBA;
            size_t bytesPerBlock=alpha?16:8,blocks=((m.width+3)/4)*((m.height+3)/4);
            std::vector<uint8_t> compressed(blocks*bytesPerBlock);
            for(size_t i=0;i<blocks;i++){
                auto* block=compressed.data()+i*bytesPerBlock;auto* color=block+(alpha?8:0);
                // Four selectors per row, rotate them by mip, both BC1 palettes.
                color[0]=0;color[1]=(level%2)?0:248;color[2]=(level%2)?255:224;color[3]=(level%2)?255:7;
                for(unsigned y=0;y<4;y++){uint8_t indices=0;for(unsigned x=0;x<4;x++)indices|=uint8_t(((x+y+level)%4)<<(2*x));color[y+4]=indices;}
                if(format==mavk::compat::BcFormat::BC2_RGBA)for(unsigned k=0;k<8;k++)block[k]=uint8_t((2*k)|((2*k+1)<<4));
                if(format==mavk::compat::BcFormat::BC3_RGBA){
                    block[0]=(level%2)?0:255;block[1]=(level%2)?255:0;uint64_t indices=0;
                    for(unsigned k=0;k<16;k++)indices|=uint64_t(k%8)<<(3*k);
                    for(unsigned k=0;k<6;k++)block[k+2]=uint8_t(indices>>(8*k));
                }
            }
            auto decoded=mavk::compat::decodeBc(format,m.width,m.height,compressed.data(),compressed.size());
            if(decoded.srgb||decoded.rgba.size()!=m.bytes)throw std::runtime_error("BC output encoding/extent");
            std::copy(decoded.rgba.begin(),decoded.rgba.end(),texture.rgba.begin()+size_t(m.offset));
        }
        double decodeMs=ms(begin,Clock::now());
        const float clear[]={1,0,1,1};
        {
            mavk::bridge::OffscreenSession renderer(context,root.queue,32,32,vs,fs,nullptr,16,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr,nullptr,&texture);
            for(unsigned level=0;level<4;level++){
                float push[]={float(level),0,0,0};auto pixels=renderer.render(push,clear);const auto& m=layout[level];
                if(pixels.size()!=4096)throw std::runtime_error("BC sampled frame size");
                for(unsigned y=0;y<32;y++)for(unsigned x=0;x<32;x++){
                    size_t source=size_t(m.offset)+(std::min(y*m.height/32,m.height-1)*m.width+std::min(x*m.width/32,m.width-1))*4;
                    for(unsigned ch=0;ch<4;ch++)if(std::abs(int(pixels[(y*32+x)*4+ch])-int(texture.rgba[source+ch]))>1)throw std::runtime_error("BC hardware sampling mismatch");
                    verified++;
                }
            }
        }
        if(!context.counters().empty())throw std::runtime_error("BC compatibility resources remain");
        if(jobs++)samples<<',';samples<<"{\"format\":"<<unsigned(format)<<",\"four_mips_decode_ms\":"<<decodeMs<<'}';
    }
    samples<<']';if(verified!=16384||context.counters().totalSubmissions!=16||jobs!=4)throw std::runtime_error("BC gate coverage");
    std::ostringstream out;out<<"{\"result\":\"EMULATED_BC123_NATIVE_SAMPLING_PASS\",\"device\":"<<quote(root.properties.deviceName)
        <<",\"classification\":\"EMULATED\",\"verified_pixels\":"<<verified<<",\"formats\":4,\"mips_per_format\":4,\"gpu_submissions\":16,\"core_live_zero\":true,\"textureCompressionBC_advertised\":false,\"samples\":"<<samples.str()
        <<",\"scope\":\"CPU BC1 RGB/RGBA,BC2,BC3 decode then existing native RGBA8 upload and real hardware sampling. UNORM only. NOT full BC1-7,compressed image API,DXVK,ICD or game PASS.\"}";
    return out.str();
}
