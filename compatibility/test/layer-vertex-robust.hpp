// Bounded native vertex-fetch semantic gate,not full robustness2 conformance.
void vertexRobustness(Root& root,mavk::DeviceContext& context,const char* vertex,const char* fragment){
    const auto vs=words(vertex),fs=words(fragment);const float state[]={0,0,0,0},clear[]={1,0,1,1};
    unsigned checked=0;
    for(unsigned components:{1u,4u}){
        mavk::bridge::VertexGeometry geometry;geometry.stride=components*4;geometry.count=3;
        geometry.attributes.push_back({0,0,components==1?VK_FORMAT_R32_SFLOAT:VK_FORMAT_R32G32B32A32_SFLOAT,0});
        geometry.bytes.resize(3*geometry.stride);const float values[]={.25f,.5f,.75f,1.f};
        for(unsigned i=0;i<3;i++)std::memcpy(geometry.bytes.data()+i*geometry.stride,values,geometry.stride);
        for(unsigned first:{0u,3u,3000000u}){
            vertexFirst=first;
            const auto frame=mavk::bridge::drawOffscreen(context,root.queue,8,8,vs,fs,state,clear,&geometry);
            if(frame.size()!=256)throw std::runtime_error("vertex robustness frame size");
            for(unsigned i=0;i<frame.size();i++){
                const auto channel=i%4;
                const int expected=first?(channel==3&&components==1?255:0):
                    (components==1?(channel==0?64:channel==3?255:0):channel==0?64:channel==1?128:channel==2?191:255);
                if(std::abs(int(frame[i])-expected)>1)throw std::runtime_error("vertex robustness mismatch components="+std::to_string(components)+" first="+std::to_string(first)+" channel="+std::to_string(channel)+" value="+std::to_string(frame[i]));
                checked++;
            }
        }
    }
    vertexFirst=0;
    std::cout<<"VERTEX_ROBUSTNESS_SLICE_PASS formats=2 firstVertices=3 components="<<checked<<" nativeRobustBufferAccess=1 noRobustness2Claim=1\n";
}
