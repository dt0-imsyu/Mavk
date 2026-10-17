// Included in core_comparison.cpp after the existing validated Root definition.
// Exercises the reusable native graphics path, NOT a Windows texture ICD PASS.
std::string sampledGate(const std::vector<uint32_t>& vs,const std::vector<uint32_t>& fs,bool validated,ValidationCapture& validation){
    Root root(validation);root.init(validated);
    mavk::DeviceContext context(root.physical,root.device,root.family);
    uint64_t checked=0;std::ostringstream samples;samples<<'[';unsigned jobs=0;
    for(unsigned seed:{0u,73u}){
        mavk::SampledPixels texture;texture.width=texture.height=8;texture.levels=4;
        auto mips=mavk::rgbaMipLayout(8,8,4);texture.rgba.resize(size_t(mips.back().offset+mips.back().bytes));
        for(unsigned level=0;level<mips.size();level++){
            auto m=mips[level];for(unsigned y=0;y<m.height;y++)for(unsigned x=0;x<m.width;x++){
                size_t at=size_t(m.offset)+(y*m.width+x)*4;
                texture.rgba[at]=uint8_t(x*17+y*3+level*31+seed);
                texture.rgba[at+1]=uint8_t(x*7+y*19+level*23+seed*3);
                texture.rgba[at+2]=uint8_t(x*29+y*11+level*47+seed*5);texture.rgba[at+3]=255;
            }
        }
        for(unsigned mode=0;mode<6;mode++){
            texture.sampler={};texture.sampler.maxLod=3;
            float lod=mode<4?float(mode):mode==4?.5f:0;
            if(mode==4)texture.sampler.mip=VK_SAMPLER_MIPMAP_MODE_LINEAR;
            if(mode==5)texture.sampler.mag=texture.sampler.min=VK_FILTER_LINEAR;
            float state[4]={lod,0,0,0},clear[4]={1,0,1,1};auto begin=Clock::now();
            mavk::bridge::OffscreenTiming timing;
            auto rgba=mavk::bridge::drawOffscreen(context,root.queue,32,32,vs,fs,state,clear,nullptr,16,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr,nullptr,&texture,nullptr,&timing);
            double elapsed=ms(begin,Clock::now());
            if(rgba.size()!=32*32*4)throw std::runtime_error("Sampled readback size");
            auto texel=[&](unsigned level,int x,int y,unsigned channel){const auto& m=mips[level];x=std::clamp(x,0,int(m.width)-1);y=std::clamp(y,0,int(m.height)-1);return float(texture.rgba[size_t(m.offset)+(y*m.width+x)*4+channel]);};
            auto nearest=[&](unsigned level,unsigned x,unsigned y,unsigned channel){auto m=mips[level];return texel(level,int((x+.5f)*m.width/32),int((y+.5f)*m.height/32),channel);};
            for(unsigned y=0;y<32;y++)for(unsigned x=0;x<32;x++){
                for(unsigned channel=0;channel<4;channel++){
                    float expected;
                    if(mode<4)expected=nearest(mode,x,y,channel);
                    else if(mode==4)expected=(nearest(0,x,y,channel)+nearest(1,x,y,channel))*.5f;
                    else {
                        float tx=(x+.5f)*8/32-.5f,ty=(y+.5f)*8/32-.5f;int ix=int(std::floor(tx)),iy=int(std::floor(ty));float fx=tx-ix,fy=ty-iy;
                        expected=(1-fy)*((1-fx)*texel(0,ix,iy,channel)+fx*texel(0,ix+1,iy,channel))+fy*((1-fx)*texel(0,ix,iy+1,channel)+fx*texel(0,ix+1,iy+1,channel));
                    }
                    if(std::abs(float(rgba[(y*32+x)*4+channel])-expected)>1.1f)throw std::runtime_error("Sampled mismatch mode="+std::to_string(mode)+" x="+std::to_string(x)+" y="+std::to_string(y)+" channel="+std::to_string(channel));
                }
                checked++;
            }
            if(!context.counters().empty())throw std::runtime_error("Sampled resources remain after completion");
            if(jobs)samples<<',';samples<<"{\"seed\":"<<seed<<",\"mode\":"<<mode<<",\"lod\":"<<lod<<",\"complete_draw_ms\":"<<elapsed<<",\"initialize_ms\":"<<timing.initializeMs<<",\"pipeline_create_ms\":"<<timing.pipelineCreateMs<<",\"record_ms\":"<<timing.recordMs<<",\"submit_wait_ms\":"<<timing.submitWaitMs<<",\"readback_ms\":"<<timing.readbackMs<<'}';jobs++;
        }
    }
    samples<<']';
    if(jobs!=12||checked!=12288||context.counters().totalSubmissions!=12)throw std::runtime_error("Sampled gate coverage");
    std::ostringstream out;out<<"{\"result\":\"NATIVE_CORE_SAMPLED_MIPS_PASS\",\"device\":"<<quote(root.properties.deviceName)<<",\"pixels_verified\":"<<checked<<",\"gpu_submissions\":"<<context.counters().totalSubmissions<<",\"mip_levels\":4,\"core_live_zero\":true,\"samples\":"<<samples.str()<<",\"scope\":\"Native reusable RGBA mip upload,nearest/linear sampling and resource teardown. NOT Bridge/Windows ICD/third-party texture or performance PASS.\"}";return out.str();
}
