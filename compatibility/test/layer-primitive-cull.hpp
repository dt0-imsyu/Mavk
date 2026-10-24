void primitiveIdCulling(Root& root,mavk::DeviceContext& context,const std::vector<uint32_t>& vs,const std::vector<uint32_t>& fs,const std::vector<uint32_t>& reference){
    if(!std::getenv("MAVK_TEST_CULL_ORIGINAL"))throw std::runtime_error("primitive-ID test requires original application stages");
    std::cout<<"CULL_APPLICATION_STAGES count=2 originalShader=1 manualTransform=0\n";
    mavk::bridge::VertexGeometry geometry;geometry.stride=4;geometry.count=9;geometry.bytes.resize(36);
    const float clear[]={0,0,0,1};const float positive[]={1,1,1,0,1,1,1,0};
    std::vector<uint8_t> uniform;if(std::getenv("MAVK_TEST_CULL_DESCRIPTOR")){const float tint[]={1,0,0,1};uniform.resize(sizeof(tint));std::memcpy(uniform.data(),tint,sizeof(tint));}
    const auto uniforms=uniform.empty()?nullptr:&uniform;std::cout<<"CULL_APPLICATION_DESCRIPTOR uniform="<<!uniform.empty()<<'\n';
    if(std::getenv("MAVK_TEST_CULL_NO_GEOMETRY")){
        mavk::bridge::OffscreenSession rejected(context,root.queue,96,32,vs,fs,&geometry,32,VK_SHADER_STAGE_VERTEX_BIT,nullptr,uniforms);
        throw std::runtime_error("cull dependency unexpectedly admitted");
    }
    mavk::bridge::OffscreenSession native(context,root.queue,96,32,reference,fs,&geometry,32,VK_SHADER_STAGE_VERTEX_BIT,nullptr,uniforms);
    const auto expected=native.render(positive,clear);uint32_t coverage[3]{};std::vector<uint8_t> masks(expected.size()/4);
    for(size_t i=0;i<expected.size();i+=4){
        if(expected[i+3]!=255)throw std::runtime_error("primitive-ID reference alpha");
        unsigned mask=0;
        for(unsigned channel=0;channel<3;channel++)if(expected[i+channel]){if(expected[i+channel]!=255||mask)throw std::runtime_error("primitive-ID reference color");mask=1u<<channel;coverage[channel]++;if((i/4%96)/32!=channel)throw std::runtime_error("primitive-ID reference index/location");}
        masks[i/4]=uint8_t(mask);
    }
    for(auto count:coverage)if(!count)throw std::runtime_error("primitive-ID reference missing index");
    mavk::bridge::OffscreenSession emulated(context,root.queue,96,32,vs,fs,&geometry,32,VK_SHADER_STAGE_VERTEX_BIT,nullptr,uniforms);
    struct Case{float values[8];unsigned visible;};
    const Case cases[]={{{1,1,1,0,1,1,1,0},7},{{-1,1,1,0,1,1,1,0},6},{{1,-1,1,0,1,1,1,0},5},{{1,1,-1,0,1,1,1,0},3},{{-1,-1,-1,0,1,1,1,0},0},{{1,1,1,0,-1,-1,-1,0},0},{{0,1,1,0,1,1,1,0},7},{{1,1,1,0,1,1,1,0},7}};
    uint32_t checked=0;
    for(const auto& test:cases){const auto frame=emulated.render(test.values,clear);if(frame.size()!=expected.size())throw std::runtime_error("primitive-ID frame size");for(size_t i=0;i<frame.size();i++){const auto mask=masks[i/4];const uint8_t value=(mask&test.visible)?expected[i]:(i%4==3?255:0);if(frame[i]!=value)throw std::runtime_error("primitive-ID preservation mismatch case="+std::to_string(checked/12288)+" byte="+std::to_string(i));checked++;}}
    std::cout<<"CULL_PRIMITIVE_ID_GPU_PASS cases=8 primitives=3 checkedBytes="<<checked<<" referencePixels="<<coverage[0]<<','<<coverage[1]<<','<<coverage[2]<<" culledIdsNotRenumbered=1 resetBetweenDraws=1 noFeatureClaim=1\n";
}
void tessellationCulling(Root& root,mavk::DeviceContext& context,const std::vector<uint32_t>& evaluation,const std::vector<uint32_t>& fs,const std::vector<uint32_t>& reference){
    const auto vertex=std::getenv("MAVK_CULL_TESS_VERTEX"),control=std::getenv("MAVK_CULL_TESS_CONTROL");
    if(!vertex||!control||!std::getenv("MAVK_TEST_CULL_ORIGINAL"))throw std::runtime_error("original tessellation stages required");
    const auto vs=words(vertex),tcs=words(control);std::cout<<"CULL_APPLICATION_STAGES count=4 originalShader=1 manualTransform=0\n";
    const float clear[]={0,0,0,1};const float positive[]={1,1,1,0,1,1,1,0};
    std::vector<uint8_t> uniform;if(std::getenv("MAVK_TEST_CULL_DESCRIPTOR")){const float tint[]={1,0,0,1};uniform.resize(sizeof(tint));std::memcpy(uniform.data(),tint,sizeof(tint));}
    const auto uniforms=uniform.empty()?nullptr:&uniform;std::cout<<"CULL_APPLICATION_DESCRIPTOR uniform="<<!uniform.empty()<<'\n';
    mavk::bridge::RasterOptions options;options.tessellationControlShader=tcs;options.tessellationEvaluationShader=reference;options.uniformStages=VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    mavk::bridge::OffscreenSession native(context,root.queue,32,32,vs,fs,nullptr,32,VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,nullptr,uniforms,nullptr,nullptr,&options);
    const auto expected=native.render(positive,clear);uint32_t red=0,partialRed=0;
    auto survivingFan=[](size_t pixel){const auto x=float(pixel%32)+0.5f,y=float(pixel/32)+0.5f;return y>=12&&(y>=16||std::abs(x-16)<1.5f*(y-12));};
    for(size_t i=0;i<expected.size();i+=4){if(expected[i]==255&&expected[i+1]==0&&expected[i+2]==0&&expected[i+3]==255){red++;partialRed+=survivingFan(i/4);}else if(expected[i]!=0||expected[i+1]!=0||expected[i+2]!=0||expected[i+3]!=255)throw std::runtime_error("tessellation reference color");}
    if(!partialRed||partialRed>=red)throw std::runtime_error("tessellation reference coverage");
    options.tessellationEvaluationShader=evaluation;
    mavk::bridge::OffscreenSession emulated(context,root.queue,32,32,vs,fs,nullptr,32,VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,nullptr,uniforms,nullptr,nullptr,&options);
    struct Case{float values[8];unsigned visible;};
    const Case cases[]={{{1,1,1,0,1,1,1,0},2},{{-1,-1,-1,0,1,1,1,0},0},{{1,1,1,0,-1,-1,-1,0},0},{{0,0,0,0,1,1,1,0},2},{{-1,-1,0.5,0,1,1,1,0},1},{{1,1,1,0,1,1,1,0},2}};
    uint32_t checked=0;
    for(const auto& test:cases){const auto frame=emulated.render(test.values,clear);if(frame.size()!=expected.size())throw std::runtime_error("tessellation frame size");for(size_t i=0;i<frame.size();i++){const bool visible=test.visible==2||(test.visible==1&&survivingFan(i/4));const uint8_t value=visible?expected[i]:(i%4==3?255:0);if(frame[i]!=value)throw std::runtime_error("tessellation cull mismatch case="+std::to_string(checked/4096)+" byte="+std::to_string(i));checked++;}}
    std::cout<<"CULL_TESSELLATION_GPU_PASS cases=6 checkedBytes="<<checked<<" referenceRedPixels="<<red<<" survivingFanPixels="<<partialRed<<" tessLevel=2 generatedPrimitiveCull=1 restored=1 noFeatureClaim=1\n";
}
void primitiveCulling(Root& root,mavk::DeviceContext& context,const char* vertex,const char* geometry,const char* fragment){
    const auto referencePath=std::getenv("MAVK_CULL_REFERENCE");if(!referencePath)throw std::runtime_error("independent cull reference required");
    const auto vs=words(vertex),gs=words(geometry),fs=words(fragment),reference=words(referencePath);
    if(std::getenv("MAVK_TEST_CULL_TESSELLATION")){tessellationCulling(root,context,vs,fs,reference);return;}
    if(std::getenv("MAVK_TEST_CULL_PRIMITIVE_ID")){primitiveIdCulling(root,context,vs,fs,reference);return;}
    const bool original=std::getenv("MAVK_TEST_CULL_ORIGINAL")!=nullptr;
    std::cout<<"CULL_APPLICATION_STAGES count="<<(original?2:3)<<" originalShader="<<original<<" manualTransform="<<!original<<'\n';
    VkPhysicalDeviceProperties properties{};vkGetPhysicalDeviceProperties(root.gpu,&properties);
    const auto& limits=properties.limits;
    if(limits.maxGeometryInputComponents<10||limits.maxGeometryOutputComponents<8||limits.maxGeometryOutputVertices<3||limits.maxGeometryTotalOutputComponents<24)throw std::runtime_error("NO-GO: geometry limits insufficient");
    const float clear[]={0,0,0,1};float positive[]={1,1,1,0,1,1,1,0};
    std::vector<uint8_t> uniform;if(std::getenv("MAVK_TEST_CULL_DESCRIPTOR")){const float tint[]={1,0,0,1};uniform.resize(sizeof(tint));std::memcpy(uniform.data(),tint,sizeof(tint));}
    const auto uniforms=uniform.empty()?nullptr:&uniform;
    std::cout<<"CULL_APPLICATION_DESCRIPTOR uniform="<<!uniform.empty()<<'\n';
    mavk::bridge::OffscreenSession native(context,root.queue,32,32,reference,fs,nullptr,32,VK_SHADER_STAGE_VERTEX_BIT,nullptr,uniforms);
    const auto expected=native.render(positive,clear);uint32_t red=0;
    for(size_t i=0;i<expected.size();i+=4){if(expected[i]==255&&expected[i+1]==0&&expected[i+2]==0&&expected[i+3]==255)red++;else if(expected[i]!=0||expected[i+1]!=0||expected[i+2]!=0||expected[i+3]!=255)throw std::runtime_error("invalid independent reference color");}
    if(!red||red>=1024)throw std::runtime_error("invalid cull reference coverage");
    mavk::bridge::RasterOptions options;if(!original)options.geometryShader=gs;
    mavk::bridge::OffscreenSession emulated(context,root.queue,32,32,vs,fs,nullptr,32,VK_SHADER_STAGE_VERTEX_BIT,nullptr,uniforms,nullptr,nullptr,&options);
    struct Case{float values[8];bool culled;};
    const Case cases[]={{{1,1,1,0,1,1,1,0},false},{{-1,1,1,0,1,1,1,0},false},{{-1,-1,-1,0,1,1,1,0},true},{{0,-1,-1,0,1,1,1,0},false},{{-1,1,1,0,1,-1,1,0},false},{{1,1,1,0,-1,-1,-1,0},true},{{-0.f,-1,-1,0,1,1,1,0},false},{{1,1,1,0,1,1,1,0},false}};
    uint32_t checked=0;
    for(const auto& test:cases){const auto frame=emulated.render(test.values,clear);if(frame.size()!=expected.size())throw std::runtime_error("cull output size");uint32_t actualRed=0;for(size_t i=0;i<frame.size();i+=4)actualRed+=frame[i]==255&&frame[i+1]==0&&frame[i+2]==0&&frame[i+3]==255;std::cout<<"CULL_CASE index="<<checked/4096<<" referenceRed="<<red<<" actualRed="<<actualRed<<" expectedCull="<<test.culled<<'\n';for(size_t i=0;i<frame.size();i++){const uint8_t value=test.culled?(i%4==3?255:0):expected[i];if(frame[i]!=value)throw std::runtime_error("whole primitive cull mismatch case="+std::to_string(checked/4096)+" byte="+std::to_string(i)+" got="+std::to_string(frame[i])+" expected="+std::to_string(value));checked++;}}
    std::cout<<"WHOLE_PRIMITIVE_CULL_GPU_PASS cases=8 planes=2 checkedBytes="<<checked<<" referenceRedPixels="<<red<<" mixedSignUnclipped=1 zeroSurvives=1 restored=1 noFeatureClaim=1\n";
}
