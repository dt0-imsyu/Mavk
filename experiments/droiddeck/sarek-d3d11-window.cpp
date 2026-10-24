// SPDX-License-Identifier: Apache-2.0
// Shared native/Win32 DXVK-Sarek consumer. Present calls are NOT visible FPS.
#include <d3d11.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <SDL3/SDL.h>
#include <dlfcn.h>
#endif
#include <cstdio>
#include <chrono>
#include <algorithm>
#include <cstdlib>
#include "sarek-frame-vs.h"
#include "sarek-filtered-ps.h"

int main(int argc,char** argv){
#ifdef _WIN32
    if(const char* log=std::getenv("MAVK_WINDOW_LOG")){if(!std::freopen(log,"w",stdout))return 15;}
#endif
    std::setvbuf(stdout,nullptr,_IONBF,0);std::setvbuf(stderr,nullptr,_IONBF,0);
    if(argc!=2)return 2;
    auto setting=[](const char* name,int fallback,int low,int high){const char* value=std::getenv(name);return value?std::clamp(std::atoi(value),low,high):fallback;};
    const int width=setting("MAVK_BENCH_WIDTH",640,320,3840),height=setting("MAVK_BENCH_HEIGHT",480,240,2160);
    const int duration=setting("MAVK_BENCH_SECONDS",20,5,60),interval=setting("MAVK_BENCH_VSYNC",1,0,1);
    std::printf("BENCH_CONFIG width=%d height=%d seconds=%d vsync=%d workload=textured-triangle\n",width,height,duration,interval);
#ifdef _WIN32
    WNDCLASSW wc{};wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"MavkD3D11Window";
    if(!RegisterClassW(&wc))return 3;
    HWND window=CreateWindowW(wc.lpszClassName,L"Mavk / Wine D3D11 / diagnostic",WS_POPUP|WS_VISIBLE,0,0,width,height,nullptr,nullptr,wc.hInstance,nullptr);
    if(!window)return 4;
    auto destroyWindow=[&]{DestroyWindow(window);UnregisterClassW(wc.lpszClassName,wc.hInstance);};
#if defined(__aarch64__)
    std::printf("WINDOW_CREATED backend=win32 processMachine=ARM64\n");
#elif defined(__x86_64__)
    std::printf("WINDOW_CREATED backend=win32 processMachine=AMD64\n");
#else
#error Unsupported test architecture
#endif
    auto library=LoadLibraryA(argv[1]);
    auto create=library?reinterpret_cast<decltype(&D3D11CreateDeviceAndSwapChain)>(GetProcAddress(library,"D3D11CreateDeviceAndSwapChain")):nullptr;
#else
    if(!SDL_Init(SDL_INIT_VIDEO)){std::fprintf(stderr,"SDL_INIT_FAIL %s\n",SDL_GetError());return 3;}
    SDL_Window* window=SDL_CreateWindow("Mavk / real DXVK-Sarek / diagnostic",width,height,SDL_WINDOW_VULKAN);
    if(!window){std::fprintf(stderr,"SDL_WINDOW_FAIL %s\n",SDL_GetError());SDL_Quit();return 4;}
    std::printf("WINDOW_CREATED backend=%s\n",SDL_GetCurrentVideoDriver());
    auto library=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
    auto create=library?reinterpret_cast<decltype(&D3D11CreateDeviceAndSwapChain)>(dlsym(library,"D3D11CreateDeviceAndSwapChain")):nullptr;
    auto destroyWindow=[&]{SDL_DestroyWindow(window);SDL_Quit();};
#endif
    if(!create){std::fprintf(stderr,"SAREK_ENTRY_FAIL\n");destroyWindow();return 5;}
    ID3D11Device* device=nullptr;ID3D11DeviceContext* context=nullptr;IDXGISwapChain* swapchain=nullptr;
    ID3D11Texture2D* backbuffer=nullptr;ID3D11RenderTargetView* target=nullptr;
    ID3D11VertexShader* vs=nullptr;ID3D11PixelShader* ps=nullptr;
    ID3D11Texture2D* texture=nullptr;ID3D11ShaderResourceView* image=nullptr;
    ID3D11SamplerState* sampler=nullptr;ID3D11RasterizerState* raster=nullptr;
    auto finish=[&](int status){if(context){context->ClearState();context->Flush();}if(raster)raster->Release();if(sampler)sampler->Release();if(image)image->Release();if(texture)texture->Release();if(ps)ps->Release();if(vs)vs->Release();if(target)target->Release();if(backbuffer)backbuffer->Release();if(swapchain)swapchain->Release();if(context)context->Release();if(device)device->Release();destroyWindow();return status;};
    DXGI_SWAP_CHAIN_DESC desc{};desc.BufferDesc.Width=width;desc.BufferDesc.Height=height;desc.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count=1;desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;desc.BufferCount=2;
    desc.OutputWindow=reinterpret_cast<HWND>(window);desc.Windowed=TRUE;desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_DISCARD;
    D3D_FEATURE_LEVEL requested=D3D_FEATURE_LEVEL_11_0,chosen{};
    HRESULT hr=create(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,&requested,1,D3D11_SDK_VERSION,&desc,&swapchain,&device,&chosen,&context);
    std::printf("WINDOW_DEVICE_HRESULT=0x%08x level=0x%x\n",unsigned(hr),unsigned(chosen));
    if(FAILED(hr)||!swapchain||!device||!context)return finish(6);
    hr=swapchain->GetBuffer(0,__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&backbuffer));if(FAILED(hr))return finish(7);
    hr=device->CreateRenderTargetView(backbuffer,nullptr,&target);if(FAILED(hr))return finish(8);
    if(FAILED(device->CreateVertexShader(mavkFrameVS,sizeof(mavkFrameVS),nullptr,&vs))||FAILED(device->CreatePixelShader(mavkFilteredPS,sizeof(mavkFilteredPS),nullptr,&ps)))return finish(9);
    unsigned char texels[64];for(unsigned y=0;y<4;y++)for(unsigned x=0;x<4;x++){auto p=texels+4*(y*4+x);p[0]=20+x*50;p[1]=30+y*50;p[2]=40+(x^y)*40;p[3]=255;}
    D3D11_TEXTURE2D_DESC td{};td.Width=td.Height=4;td.MipLevels=td.ArraySize=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.SampleDesc.Count=1;td.Usage=D3D11_USAGE_IMMUTABLE;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA data{texels,16,64};
    if(FAILED(device->CreateTexture2D(&td,&data,&texture))||FAILED(device->CreateShaderResourceView(texture,nullptr,&image)))return finish(10);
    D3D11_SAMPLER_DESC sd{};sd.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_WRAP;sd.MaxAnisotropy=1;sd.ComparisonFunc=D3D11_COMPARISON_NEVER;
    if(FAILED(device->CreateSamplerState(&sd,&sampler)))return finish(11);
    D3D11_RASTERIZER_DESC rs{};rs.FillMode=D3D11_FILL_SOLID;rs.CullMode=D3D11_CULL_NONE;rs.DepthClipEnable=TRUE;rs.ScissorEnable=TRUE;
    if(FAILED(device->CreateRasterizerState(&rs,&raster)))return finish(12);
    D3D11_VIEWPORT viewport{0,0,float(width),float(height),0,1};context->RSSetViewports(1,&viewport);context->RSSetState(raster);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);context->VSSetShader(vs,nullptr,0);context->PSSetShader(ps,nullptr,0);
    context->PSSetShaderResources(0,1,&image);context->PSSetSamplers(0,1,&sampler);
    unsigned presents=0;auto start=std::chrono::steady_clock::now();bool quit=false;
    double presentMs=0;unsigned measured=0;auto sampleStart=start;
    const bool resizeTest=setting("MAVK_BENCH_RESIZE",0,0,1)!=0;
    int resizePhase=0,frameWidth=width,frameHeight=height;
    while(!quit&&std::chrono::steady_clock::now()-start<std::chrono::seconds(duration+3)){
#ifdef _WIN32
        MSG event;while(PeekMessageW(&event,nullptr,0,0,PM_REMOVE)){if(event.message==WM_QUIT)quit=true;TranslateMessage(&event);DispatchMessageW(&event);}
        if(!IsWindow(window))quit=true;
#else
        SDL_Event event;while(SDL_PollEvent(&event)){
            if(event.type==SDL_EVENT_QUIT)quit=true;
            if(event.type==SDL_EVENT_MOUSE_BUTTON_DOWN||event.type==SDL_EVENT_KEY_DOWN){
                std::printf("WINDOW_INPUT type=%s\n",event.type==SDL_EVENT_MOUSE_BUTTON_DOWN?"mouse-button":"key");
                std::fflush(stdout);
            }
        }
#endif
        const auto elapsed=std::chrono::steady_clock::now()-start;
        if(resizeTest&&resizePhase<2&&elapsed>=std::chrono::seconds(resizePhase?10:5)){
            frameWidth=resizePhase?width:1280;frameHeight=resizePhase?height:800;
            context->OMSetRenderTargets(0,nullptr,nullptr);
            target->Release();target=nullptr;backbuffer->Release();backbuffer=nullptr;context->Flush();
            hr=swapchain->ResizeBuffers(0,frameWidth,frameHeight,DXGI_FORMAT_UNKNOWN,0);
            if(FAILED(hr)){std::fprintf(stderr,"WINDOW_RESIZE_FAIL=0x%08x phase=%d\n",unsigned(hr),resizePhase);return finish(16);}
            if(FAILED(swapchain->GetBuffer(0,__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&backbuffer)))||
               FAILED(device->CreateRenderTargetView(backbuffer,nullptr,&target)))return finish(17);
            viewport.Width=float(frameWidth);viewport.Height=float(frameHeight);context->RSSetViewports(1,&viewport);
            std::printf("WINDOW_RESIZE_PASS width=%d height=%d\n",frameWidth,frameHeight);resizePhase++;
        }
        context->OMSetRenderTargets(1,&target,nullptr);const float clear[]={0.025f,0.05f,0.08f,1};context->ClearRenderTargetView(target,clear);
        LONG x=LONG((presents*7)%std::max(1,frameWidth/2));D3D11_RECT rect{x,0,x+frameWidth/2,frameHeight};context->RSSetScissorRects(1,&rect);context->Draw(3,0);
        auto before=std::chrono::steady_clock::now();
        hr=swapchain->Present(interval,0);if(FAILED(hr)){std::fprintf(stderr,"WINDOW_PRESENT_FAIL=0x%08x frame=%u\n",unsigned(hr),presents);return finish(13);}presents++;
        auto after=std::chrono::steady_clock::now();
        if(before-start>=std::chrono::seconds(3)){if(!measured)sampleStart=before;measured++;presentMs+=std::chrono::duration<double,std::milli>(after-before).count();}
        if(presents==1)std::printf("WINDOW_PRESENT_RETURNED frames=%u\n",presents);
    }
    double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    std::printf("WINDOW_STREAM_END presents=%u seconds=%.3f calls_per_second=%.3f visible_fps=UNMEASURED\n",presents,seconds,presents/seconds);
    const double measuredSeconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-sampleStart).count();
    std::printf("BENCH_RESULT measured=%u seconds=%.3f present_calls_per_second=%.3f mean_present_ms=%.3f warmup_seconds=3 visible_fps=UNMEASURED\n",measured,measuredSeconds,measured/measuredSeconds,presentMs/std::max(1u,measured));
    return finish(presents?0:14);
}
