#ifdef MAVK_NATIVE_UI
// Unmodified upstream ImGui builds the actual font atlas, widgets and draw lists.
// The adapter translates draw data; it does not replace upstream shader logic.
std::string uiGate(const std::vector<uint32_t>& vs,const std::vector<uint32_t>& fs,bool validated,ValidationCapture& validation,const std::vector<uint32_t>* backgroundVertex=nullptr,const std::vector<uint32_t>* backgroundFragment=nullptr,const std::vector<uint32_t>* wireShader=nullptr){
    bool layered=backgroundVertex&&backgroundFragment;
    bool bridgeMode=wireShader!=nullptr;if(bridgeMode&&!layered)throw std::runtime_error("Bridge UI requires layered fixture");unsigned bridgeCommands=0;
    struct Gui {Gui(){ImGui::CreateContext();}~Gui(){ImGui::DestroyContext();}} gui;
    auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;io.DisplaySize=ImVec2(256,192);io.DeltaTime=1.f/60;
    io.Fonts->AddFontDefault();unsigned char* atlas=nullptr;int fw=0,fh=0;io.Fonts->GetTexDataAsRGBA32(&atlas,&fw,&fh);io.Fonts->SetTexID(reinterpret_cast<void*>(uintptr_t(1)));
    if(!atlas||fw<1||fh<1||fw>1024||fh>1024)throw std::runtime_error("Bounded actual font atlas unavailable");
    mavk::SampledPixels font;font.width=fw;font.height=fh;font.levels=1;font.rgba.assign(atlas,atlas+size_t(fw)*fh*4);font.sampler.mag=font.sampler.min=VK_FILTER_LINEAR;
    Root root(validation);root.init(validated);mavk::DeviceContext context(root.physical,root.device,root.family);
    uint64_t checked=0;unsigned draws=0,widgets=0,baseVertex=0,different=0,rejected=0;std::vector<uint8_t> previous;
    const float clear[]={24.f/255,96.f/255,168.f/255,192.f/255};
    {
        std::unique_ptr<mavk::SampledImage> gpuFont;std::unique_ptr<UiWire> wire;uint64_t fontId=0;std::vector<uint8_t> lastPacket;
        if(bridgeMode){wire=std::make_unique<UiWire>(context,root.queue,*wireShader);fontId=wire->upload(font);}else {gpuFont=std::make_unique<mavk::SampledImage>(context,font);gpuFont->upload(root.queue,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);}
        for(unsigned frame=0;frame<2;frame++){
            ImGui::NewFrame();ImGui::SetNextWindowPos(ImVec2(64,4));ImGui::SetNextWindowSize(ImVec2(188,184));
            ImGui::Begin("Mavk native UI",nullptr,ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoCollapse);
            ImGui::Text("Mali-G925 frame %u",frame);bool selected=frame!=0;ImGui::Checkbox("Hardware Vulkan",&selected);ImGui::Button(frame?"Frame one":"Frame zero");ImGui::End();
            ImDrawList a(ImGui::GetDrawListSharedData()),b(ImGui::GetDrawListSharedData());
            a.PushTextureID(io.Fonts->TexID);a.PushClipRect(ImVec2(0,0),io.DisplaySize);a.AddRectFilled(ImVec2(8,8),ImVec2(40,40),IM_COL32(196,72,32,128));
            b.PushTextureID(io.Fonts->TexID);b.PushClipRect(ImVec2(16,16),ImVec2(32,32));b.AddRectFilled(ImVec2(8,8),ImVec2(40,40),IM_COL32(32,200,80,192));
            ImGui::Render();auto* data=ImGui::GetDrawData();if(!data||!data->Valid||!data->CmdListsCount)throw std::runtime_error("No actual ImGui widget draw data");
            mavk::bridge::VertexGeometry geometry;geometry.stride=sizeof(ImDrawVert);geometry.indexType=VK_INDEX_TYPE_UINT16;
            static_assert(sizeof(ImDrawVert)==20&&sizeof(ImDrawIdx)==2,"Pinned upstream UI vertex/index ABI");
            geometry.attributes={{0,0,VK_FORMAT_R32G32_SFLOAT,uint32_t(offsetof(ImDrawVert,pos))},{1,0,VK_FORMAT_R32G32_SFLOAT,uint32_t(offsetof(ImDrawVert,uv))},{2,0,VK_FORMAT_R8G8B8A8_UNORM,uint32_t(offsetof(ImDrawVert,col))}};
            mavk::bridge::SceneState scene;scene.pushBytes=16;scene.pushStages=VK_SHADER_STAGE_VERTEX_BIT;
            auto append=[&](const ImDrawList& list,bool widget){
                int32_t vertexOffset=int32_t(geometry.count);uint32_t first=geometry.indexCount;
                const auto* vertex=reinterpret_cast<const uint8_t*>(list.VtxBuffer.Data);geometry.bytes.insert(geometry.bytes.end(),vertex,vertex+list.VtxBuffer.Size*sizeof(ImDrawVert));geometry.count+=list.VtxBuffer.Size;
                const auto* index=reinterpret_cast<const uint8_t*>(list.IdxBuffer.Data);geometry.indices.insert(geometry.indices.end(),index,index+list.IdxBuffer.Size*sizeof(ImDrawIdx));geometry.indexCount+=list.IdxBuffer.Size;
                for(const auto& cmd:list.CmdBuffer){
                    if(cmd.UserCallback||cmd.TextureId!=io.Fonts->TexID)throw std::runtime_error("Unsupported UI callback/texture, not silently ignored");
                    int x=std::clamp(int(cmd.ClipRect.x),0,256),y=std::clamp(int(cmd.ClipRect.y),0,192),right=std::clamp(int(cmd.ClipRect.z),0,256),bottom=std::clamp(int(cmd.ClipRect.w),0,192);
                    if(cmd.ElemCount&&right>x&&bottom>y){
                        mavk::bridge::SceneDraw draw;draw.firstIndex=first;draw.indexCount=cmd.ElemCount;draw.vertexOffset=vertexOffset;draw.clipped=true;draw.clip={{x,y},{uint32_t(right-x),uint32_t(bottom-y)}};
                        float transform[]={2.f/256,2.f/192,-1,-1};draw.push.resize(16);std::memcpy(draw.push.data(),transform,16);scene.draws.push_back(std::move(draw));draws++;if(widget)widgets++;if(vertexOffset)baseVertex++;
                    }
                    first+=cmd.ElemCount;
                }
            };
            append(a,false);append(b,false);for(int i=0;i<data->CmdListsCount;i++)append(*data->CmdLists[i],true);
            mavk::bridge::RasterOptions raster;raster.uiOverlayAlpha=true;raster.sampledBinding=0;
            std::vector<uint8_t> pixels;
            if(layered){
                // Distinct depth-enabled background pipeline writes only x0..32,
                // y0..48; UI must LOAD both its blue pixels and untouched clear.
                mavk::bridge::VertexGeometry bg;bg.stride=8;bg.count=4;bg.indexCount=6;bg.indexType=VK_INDEX_TYPE_UINT16;bg.attributes={{0,0,VK_FORMAT_R32G32_SFLOAT,0}};
                const float xy[]={-1,-1,-.75f,-1,-.75f,-.5f,-1,-.5f};bg.bytes.resize(sizeof(xy));std::memcpy(bg.bytes.data(),xy,sizeof(xy));
                const uint16_t idx[]={0,1,2,0,2,3};bg.indices.resize(sizeof(idx));std::memcpy(bg.indices.data(),idx,sizeof(idx));
                mavk::bridge::SceneState bgScene;bgScene.pushBytes=16;bgScene.pushStages=VK_SHADER_STAGE_VERTEX_BIT;bgScene.depthFormat=VK_FORMAT_D32_SFLOAT;
                mavk::bridge::SceneDraw bgDraw;bgDraw.indexCount=6;bgDraw.push.resize(16);bgScene.draws.push_back(bgDraw);
                if(wire){
                    std::vector<uint8_t> packet;UiWire::put(packet,1);UiWire::put(packet,2);UiWire::layer(packet,*backgroundVertex,*backgroundFragment,bg,bgScene,clear,0,0,1);UiWire::layer(packet,vs,fs,geometry,scene,clear,fontId,2,0);
                    pixels=wire->draw(packet,context);lastPacket=std::move(packet);
                }else{
                auto background=std::make_unique<mavk::bridge::OffscreenSession>(context,root.queue,256,192,*backgroundVertex,*backgroundFragment,&bg,16,VK_SHADER_STAGE_VERTEX_BIT,&bgScene);
                mavk::bridge::OffscreenSession renderer(context,root.queue,256,192,vs,fs,&geometry,16,VK_SHADER_STAGE_VERTEX_BIT,&scene,nullptr,nullptr,gpuFont.get(),&raster,background.get());
                // Child retains previous pipeline/target even if its public
                // parent handle is released. Font stays alive in caller scope.
                background.reset();if(context.counters().images!=3)throw std::runtime_error("Expected font+one shared color+one depth image");
                try{renderer.renderLayers({mavk::bridge::LayerFrame{}},clear);throw std::runtime_error("Bad layer count accepted");}catch(const std::invalid_argument&){rejected++;}
                try{renderer.render(nullptr,clear);throw std::runtime_error("Layered single-render accepted");}catch(const std::invalid_argument&){rejected++;}
                pixels=renderer.renderLayers({mavk::bridge::LayerFrame{},mavk::bridge::LayerFrame{}},clear);
                }
            }else {mavk::bridge::OffscreenSession renderer(context,root.queue,256,192,vs,fs,&geometry,16,VK_SHADER_STAGE_VERTEX_BIT,&scene,nullptr,nullptr,gpuFont.get(),&raster);pixels=renderer.render(nullptr,clear);}
            if(pixels.size()!=256*192*4)throw std::runtime_error("Actual UI readback size");
            for(unsigned y=0;y<48;y++)for(unsigned x=0;x<48;x++){
                float expected[]={24,96,168,192};
                if(layered&&x<32){expected[0]=0;expected[1]=63.75f;expected[2]=expected[3]=255;}
                auto over=[&](const float* color,float alpha){for(unsigned ch=0;ch<3;ch++)expected[ch]=color[ch]*alpha+expected[ch]*(1-alpha);expected[3]=255*alpha*(1-alpha);};
                const float red[]={196,72,32},green[]={32,200,80};
                if(x>=8&&x<40&&y>=8&&y<40)over(red,128.f/255);
                if(x>=16&&x<32&&y>=16&&y<32)over(green,192.f/255);
                for(unsigned ch=0;ch<4;ch++)if(std::abs(float(pixels[(y*256+x)*4+ch])-expected[ch])>1.5f)throw std::runtime_error("UI blend/clip/base-vertex actual pixel mismatch");checked++;
            }
            if(frame)for(unsigned y=0;y<192;y++)for(unsigned x=64;x<256;x++){size_t at=(y*256+x)*4;if(std::memcmp(previous.data()+at,pixels.data()+at,4))different++;}
            previous=std::move(pixels);
        }
        if(wire){std::vector<uint8_t> id;UiWire::put64(id,fontId);wire->exchange(22,id);wire->exchange(24,lastPacket,4);rejected=wire->negativeTests;bridgeCommands=wire->commands;}
    }
    if(!context.counters().empty()||context.counters().totalSubmissions!=3||context.counters().totalPipelineCreates!=(bridgeMode?5:layered?4:2)||checked!=4608||widgets<2||baseVertex<4||different<10||rejected!=(bridgeMode?19:layered?4:0)||(bridgeMode&&bridgeCommands!=30))throw std::runtime_error("Actual UI coverage/lifecycle mismatch");
    std::ostringstream out;out<<"{\"result\":"<<quote(bridgeMode?"NATIVE_UI_BRIDGE_LAYERS_PASS":layered?"NATIVE_UPSTREAM_UI_LAYERS_PASS":"NATIVE_UPSTREAM_UI_PACKETS_PASS")<<",\"device\":"<<quote(root.properties.deviceName)
        <<",\"upstream_commit\":\"4ee2501c8280cf71d1f9ce231cd8ebd47b58adde\",\"imgui_version\":"<<quote(IMGUI_VERSION)<<",\"font_width\":"<<fw<<",\"font_height\":"<<fh
        <<",\"verified_pixels\":"<<checked<<",\"gpu_submissions\":3,\"pipeline_creates\":"<<(bridgeMode?5:layered?4:2)<<",\"indexed_draws\":"<<draws<<",\"widget_draws\":"<<widgets<<",\"nonzero_base_vertex_draws\":"<<baseVertex<<",\"changed_widget_pixels\":"<<different
        <<",\"negative_tests\":"<<rejected<<",\"bridge_commands\":"<<bridgeCommands<<",\"transport\":"<<quote(bridgeMode?"in-process encoded Bridge request/response, NOT FD/vsock":"native direct")
        <<",\"passes_per_frame\":"<<(layered?2:1)<<",\"shared_color_targets\":1,\"intermediate_readbacks\":0,\"frames\":2,\"core_live_zero\":true,\"scope\":\"Original ImGui library/shaders, native draw packets. Layered mode preserves depth-rendered background color with LOAD on one target,one submit/final readback per frame. Analytic oracle covers probe rectangles,not every glyph. NOT complete upstream app/ICD/Surface/visible FPS or CTS PASS.\"}";return out.str();
}
#endif
