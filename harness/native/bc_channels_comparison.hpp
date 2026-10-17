std::string bcChannelsGate(const std::vector<uint32_t>& vs,const std::vector<uint32_t>& fs,bool validated,ValidationCapture& validation){
    Root root(validation);root.init(validated);mavk::DeviceContext context(root.physical,root.device,root.family);
    uint64_t verified=0;unsigned formats=0;
    for(auto format:{mavk::compat::BcChannelFormat::BC4_UNORM,mavk::compat::BcChannelFormat::BC4_SNORM,mavk::compat::BcChannelFormat::BC5_UNORM,mavk::compat::BcChannelFormat::BC5_SNORM}){
        bool two=format==mavk::compat::BcChannelFormat::BC5_UNORM||format==mavk::compat::BcChannelFormat::BC5_SNORM;
        bool sign=format==mavk::compat::BcChannelFormat::BC4_SNORM||format==mavk::compat::BcChannelFormat::BC5_SNORM;
        mavk::SampledPixels texture;texture.width=texture.height=8;texture.levels=4;texture.sampler.maxLod=3;texture.format=VK_FORMAT_R32G32B32A32_SFLOAT;
        auto layout=mavk::sampledMipLayout(8,8,4,texture.format);texture.rgba.resize(size_t(layout.back().offset+layout.back().bytes));
        std::vector<mavk::compat::DecodedBcChannels> decoded;
        for(unsigned level=0;level<4;level++){
            const auto& m=layout[level];size_t blockBytes=two?16:8,blocks=((m.width+3)/4)*((m.height+3)/4);
            std::vector<uint8_t> bytes(blocks*blockBytes);
            for(size_t i=0;i<blocks;i++)for(unsigned channel=0;channel<(two?2u:1u);channel++){
                auto* p=bytes.data()+i*blockBytes+channel*8;
                p[0]=sign?128:0;p[1]=sign?127:255;
                if((level+channel)%2)std::swap(p[0],p[1]);
                uint64_t indices=0;for(unsigned k=0;k<16;k++)indices|=uint64_t((k+level+channel)%8)<<(3*k);
                for(unsigned k=0;k<6;k++)p[k+2]=uint8_t(indices>>(8*k));
            }
            decoded.push_back(mavk::compat::decodeBcChannels(format,m.width,m.height,bytes.data(),bytes.size()));
            if(decoded.back().rgba.size()*sizeof(float)!=m.bytes)throw std::runtime_error("BC45 float mip bytes");
            std::memcpy(texture.rgba.data()+size_t(m.offset),decoded.back().rgba.data(),size_t(m.bytes));
        }
        {
            mavk::bridge::OffscreenSession renderer(context,root.queue,32,32,vs,fs,nullptr,16,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr,nullptr,&texture);
            for(unsigned level=0;level<4;level++){
                float push[]={float(level),0,0,0},clear[]={1,0,1,1};auto frame=renderer.render(push,clear);const auto& m=layout[level];
                if(frame.size()!=4096)throw std::runtime_error("BC45 native frame size");
                for(unsigned y=0;y<32;y++)for(unsigned x=0;x<32;x++){
                    size_t at=(y*m.height/32*m.width+x*m.width/32)*4;
                    for(unsigned ch=0;ch<4;ch++){
                        float expected=ch<2?(decoded[level].rgba[at+ch]*.5f+.5f)*255:ch==2?63.75f:255;
                        if(std::abs(float(frame[(y*32+x)*4+ch])-expected)>1.1f)throw std::runtime_error("BC45 signed native sampling mismatch");
                    }
                    verified++;
                }
            }
        }
        if(!context.counters().empty())throw std::runtime_error("BC45 native resources remain");
        formats++;
    }
    if(formats!=4||verified!=16384||context.counters().totalSubmissions!=16)throw std::runtime_error("BC45 native gate coverage");
    std::ostringstream out;out<<"{\"result\":\"EMULATED_BC45_NATIVE_SAMPLING_PASS\",\"classification\":\"EMULATED\",\"device\":"<<quote(root.properties.deviceName)
        <<",\"verified_pixels\":16384,\"formats\":4,\"mips_per_format\":4,\"gpu_submissions\":16,\"core_live_zero\":true,\"textureCompressionBC_advertised\":false,\"scope\":\"CPU BC4/5 UNORM+SNORM float decode,RGBA32F native Core upload,signed shader sampling then remapped RGBA8 verification. Quantized readback tolerance1.1/255; NOT full-precision GPU oracle,BC6H/7,compressed image API,ICD,DXVK or performance PASS.\"}";
    return out.str();
}
