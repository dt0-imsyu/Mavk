#ifdef MAVK_NATIVE_UI
struct UiWire {
    mavk::bridge::Session session;uint64_t sequence=0;unsigned commands=0,negativeTests=0;
    UiWire(mavk::DeviceContext& c,VkQueue queue,const std::vector<uint32_t>& shader):session(c,queue,shader.data(),shader.size()*4){}
    static void put(std::vector<uint8_t>& b,uint32_t n){for(unsigned i=0;i<4;i++)b.push_back(uint8_t(n>>(i*8)));}
    static void put64(std::vector<uint8_t>& b,uint64_t n){put(b,uint32_t(n));put(b,uint32_t(n>>32));}
    static void number(std::vector<uint8_t>& b,float f){uint32_t n;std::memcpy(&n,&f,4);put(b,n);}
    static uint32_t word(const std::vector<uint8_t>& b,size_t at){if(at>b.size()||b.size()-at<4)throw std::runtime_error("UI wire read bound");return b[at]|(uint32_t(b[at+1])<<8)|(uint32_t(b[at+2])<<16)|(uint32_t(b[at+3])<<24);}
    static uint64_t wide(const std::vector<uint8_t>& b,size_t at){return word(b,at)|(uint64_t(word(b,at+4))<<32);}
    static void set(std::vector<uint8_t>& b,size_t at,uint32_t n){if(at>b.size()||b.size()-at<4)throw std::runtime_error("UI wire mutation bound");for(unsigned i=0;i<4;i++)b[at+i]=uint8_t(n>>(i*8));}
    std::vector<uint8_t> exchange(uint32_t op,const std::vector<uint8_t>& payload,uint32_t status=0){
        if(payload.size()>mavk::bridge::MaxPayload)throw std::runtime_error("UI packet payload bound");
        std::vector<uint8_t> request;put(request,mavk::bridge::Magic);put(request,1|(32<<16));put(request,op);put(request,0);put64(request,++sequence);put(request,uint32_t(payload.size()));put(request,0);request.insert(request.end(),payload.begin(),payload.end());
        auto reply=session.exchange(request);commands++;if(status)negativeTests++;
        if(reply.size()<32||word(reply,0)!=mavk::bridge::Magic||word(reply,4)!=(1|(32<<16))||word(reply,8)!=op||word(reply,12)!=1||wide(reply,16)!=sequence||word(reply,24)!=reply.size()-32||word(reply,28)!=status)throw std::runtime_error("UI Bridge response mismatch op="+std::to_string(op)+" status="+std::to_string(reply.size()<32?999:word(reply,28)));
        return {reply.begin()+32,reply.end()};
    }
    uint64_t upload(const mavk::SampledPixels& font){
        auto hello=exchange(1,{});if(hello.size()!=16)throw std::runtime_error("Bridge hello size");
        std::vector<uint8_t> meta;const auto& s=font.sampler;for(uint32_t n:{font.width,font.height,font.levels,uint32_t(s.mag),uint32_t(s.min),uint32_t(s.mip),uint32_t(s.u),uint32_t(s.v),uint32_t(s.w)})put(meta,n);
        number(meta,s.bias);number(meta,s.minLod);number(meta,s.maxLod);put(meta,uint32_t(font.rgba.size()));auto created=exchange(19,meta);if(created.size()!=8)throw std::runtime_error("Font ID size");uint64_t id=wide(created,0);
        std::vector<uint8_t> data;put64(data,id);put(data,0);put(data,uint32_t(font.rgba.size()));data.insert(data.end(),font.rgba.begin(),font.rgba.end());exchange(20,data);data.resize(8);exchange(21,data);return id;
    }
    static void layer(std::vector<uint8_t>& packet,const std::vector<uint32_t>& vs,const std::vector<uint32_t>& fs,const mavk::bridge::VertexGeometry& g,const mavk::bridge::SceneState& state,const float* clear,uint64_t texture,uint32_t blend,uint32_t binding){
        std::vector<uint8_t> geo;put(geo,g.stride);put(geo,uint32_t(g.attributes.size()));put(geo,g.count);put(geo,uint32_t(g.bytes.size()));
        for(const auto& a:g.attributes){put(geo,a.location);put(geo,uint32_t(a.format));put(geo,a.offset);}geo.insert(geo.end(),g.bytes.begin(),g.bytes.end());
        std::vector<uint8_t> indices;put(indices,uint32_t(geo.size()));put(indices,uint32_t(g.indexType));put(indices,g.indexCount);put(indices,uint32_t(g.indices.size()));indices.insert(indices.end(),geo.begin(),geo.end());indices.insert(indices.end(),g.indices.begin(),g.indices.end());
        std::vector<uint8_t> scene;for(uint32_t n:{256u,192u,uint32_t(vs.size()*4),uint32_t(fs.size()*4),state.pushStages,state.pushBytes,state.cull,uint32_t(state.front),uint32_t(state.depthFormat),uint32_t(state.draws.size()),uint32_t(indices.size())})put(scene,n);
        for(unsigned i=0;i<4;i++)number(scene,clear[i]);number(scene,state.clearDepth);for(auto n:vs)put(scene,n);for(auto n:fs)put(scene,n);scene.insert(scene.end(),indices.begin(),indices.end());
        for(const auto& d:state.draws){put(scene,d.firstIndex);put(scene,d.indexCount);put(scene,uint32_t(d.vertexOffset));put(scene,d.clipped?1:0);put(scene,d.clipped?uint32_t(d.clip.offset.x):0);put(scene,d.clipped?uint32_t(d.clip.offset.y):0);put(scene,d.clipped?d.clip.extent.width:0);put(scene,d.clipped?d.clip.extent.height:0);scene.insert(scene.end(),d.push.begin(),d.push.end());}
        put(packet,uint32_t(20+scene.size()));put(packet,blend);put(packet,binding);put64(packet,texture);put(packet,0);packet.insert(packet.end(),scene.begin(),scene.end());
    }
    std::vector<uint8_t> draw(const std::vector<uint8_t>& packet,mavk::DeviceContext& context){
        size_t second=8+4+word(packet,8)+4;auto before=context.counters();
        for(const auto& change:std::vector<std::pair<size_t,uint32_t>>{{0,0},{4,0},{4,9},{8,UINT32_MAX},{second,3},{second+4,2},{second+20,16}}){auto bad=packet;set(bad,change.first,change.second);exchange(24,bad,2);}
        auto bad=packet;set(bad,second+8,999);exchange(24,bad,4);bad=packet;set(bad,second+4,1);exchange(24,bad,3);
        if(context.counters().totalAllocations!=before.totalAllocations||context.counters().totalPipelineCreates!=before.totalPipelineCreates||context.counters().totalSubmissions!=before.totalSubmissions)throw std::runtime_error("Rejected layer packet allocated/submitted GPU work");
        auto description=exchange(24,packet);if(description.size()!=16||word(description,8)!=256*192*4||word(description,12)!=256*4)throw std::runtime_error("Layer snapshot layout");
        std::vector<uint8_t> read;put64(read,wide(description,0));put(read,0);put(read,256*192*4);auto pixels=exchange(15,read);read.resize(8);exchange(16,read);return pixels;
    }
};
#endif
