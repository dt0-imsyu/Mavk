std::string bc67Gate(const std::vector<uint32_t>& vs,const std::vector<uint32_t>& fs,const std::vector<uint32_t>& hdrFs,bool validated,ValidationCapture& validation){
    Root root(validation);root.init(validated);mavk::DeviceContext context(root.physical,root.device,root.family);
    uint64_t verified=0,negative=0,hdrValues=0;uint32_t rng=0x67bc;
    for(unsigned variant=0;variant<3;variant++){
        bool hdr=variant!=0,sign=variant==2;
        mavk::SampledPixels texture;texture.width=texture.height=8;texture.levels=4;texture.sampler.maxLod=3;
        texture.format=hdr?VK_FORMAT_R32G32B32A32_SFLOAT:VK_FORMAT_R8G8B8A8_UNORM;
        auto layout=mavk::sampledMipLayout(8,8,4,texture.format);texture.rgba.resize(size_t(layout.back().offset+layout.back().bytes));
        std::vector<std::vector<float>> expected;
        for(unsigned level=0;level<4;level++){
            const auto& m=layout[level];size_t blocks=((m.width+3)/4)*((m.height+3)/4);std::vector<uint8_t> bytes(blocks*16);
            for(size_t i=0;i<blocks;i++){
                auto* p=bytes.data()+i*16;for(unsigned j=0;j<16;j++){rng=1664525*rng+1013904223;p[j]=uint8_t(rng>>24);}
                if(hdr)p[0]=uint8_t((p[0]&~31u)|(3+4*level));
                else{unsigned mode=(level+unsigned(i))%8;p[0]=uint8_t((p[0]&~((1u<<(mode+1))-1u))|(1u<<mode));}
            }
            expected.emplace_back(size_t(m.width)*m.height*4);
            if(hdr){
                auto decoded=mavk::compat::decodeBc6h(sign,m.width,m.height,bytes.data(),bytes.size());
                for(size_t i=0;i<decoded.rgba.size();i++)if(i%4!=3){negative+=decoded.rgba[i]<0;hdrValues+=decoded.rgba[i]>1;}
                expected.back()=decoded.rgba;std::memcpy(texture.rgba.data()+size_t(m.offset),decoded.rgba.data(),size_t(m.bytes));
            }else{
                auto decoded=mavk::compat::decodeBc7(m.width,m.height,bytes.data(),bytes.size());
                for(size_t i=0;i<decoded.rgba.size();i++)expected.back()[i]=float(decoded.rgba[i])/255;
                std::copy(decoded.rgba.begin(),decoded.rgba.end(),texture.rgba.begin()+size_t(m.offset));
            }
        }
        {
            mavk::bridge::OffscreenSession renderer(context,root.queue,32,32,vs,hdr?hdrFs:fs,nullptr,16,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr,nullptr,&texture);
            for(unsigned level=0;level<4;level++){
                float push[]={float(level),0,0,0},clear[]={1,0,1,1};auto frame=renderer.render(push,clear);const auto& m=layout[level];
                if(frame.size()!=4096)throw std::runtime_error("BC67 native frame size");
                for(unsigned y=0;y<32;y++)for(unsigned x=0;x<32;x++){
                    size_t at=(y*m.height/32*m.width+x*m.width/32)*4;
                    for(unsigned c=0;c<4;c++){
                        float value=expected[level][at+c];if(hdr&&c<3)value=value/(1+std::abs(value))*.5f+.5f;
                        if(std::abs(float(frame[(y*32+x)*4+c])-value*255)>1.1f)throw std::runtime_error("BC67 hardware sampling mismatch");
                    }
                    verified++;
                }
            }
        }
        if(!context.counters().empty())throw std::runtime_error("BC67 resources remain");
    }
    if(verified!=12288||context.counters().totalSubmissions!=12||!negative||!hdrValues)throw std::runtime_error("BC67 native coverage");
    std::ostringstream out;out<<"{\"result\":\"EMULATED_BC67_NATIVE_SAMPLING_PASS\",\"classification\":\"EMULATED\",\"device\":"<<quote(root.properties.deviceName)
        <<",\"verified_pixels\":12288,\"variants\":3,\"mips_per_variant\":4,\"gpu_submissions\":12,\"negative_input_components\":"<<negative<<",\"hdr_input_components\":"<<hdrValues
        <<",\"core_live_zero\":true,\"textureCompressionBC_advertised\":false,\"scope\":\"Portable CPU BC7/BC6H signed+unsigned codec,existing Core native upload and hardware sampling,tonemapped RGBA8 verification tolerance1.1/255. NOT compressed-image API,full-precision GPU oracle,DXVK,ICD or performance PASS.\"}";
    return out.str();
}
