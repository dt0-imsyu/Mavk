// Host-only Microsoft D3D11 WARP texture sampling oracle. No window, VM or ICD.
#include <mavk/compat/bc.hpp>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>
template<class T>struct Com {T* p=nullptr;~Com(){if(p)p->Release();}T* operator->()const{return p;}T** address(){return &p;}};
static void check(HRESULT r,const char* operation){if(FAILED(r)){std::fprintf(stderr,"%s HRESULT=%08lx\n",operation,static_cast<unsigned long>(r));throw std::runtime_error(operation);}}
int main(){try{
    Com<ID3D11Device> device;Com<ID3D11DeviceContext> context;D3D_FEATURE_LEVEL level;
    D3D_FEATURE_LEVEL requested=D3D_FEATURE_LEVEL_11_0;
    check(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,&requested,1,D3D11_SDK_VERSION,device.address(),&level,context.address()),"Create Microsoft WARP");
    const char* hlsl="Texture2D<float4> image:register(t0);RWStructuredBuffer<float4> output:register(u0);[numthreads(8,8,1)] void main(uint3 p:SV_DispatchThreadID){uint w,h;image.GetDimensions(w,h);if(p.x<w&&p.y<h)output[p.y*w+p.x]=image.Load(int3(p.xy,0));}";
    Com<ID3DBlob> code,errors;check(D3DCompile(hlsl,std::strlen(hlsl),nullptr,nullptr,nullptr,"main","cs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,code.address(),errors.address()),"Compile sampling shader");
    Com<ID3D11ComputeShader> shader;check(device->CreateComputeShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,shader.address()),"Compute shader");
    uint64_t checked=0;uint32_t rng=0x67426742;unsigned jobs=0;
    const unsigned modes[]={0,1,2,6,10,14,18,22,26,30,3,7,11,15};
    for(unsigned variant=0;variant<3;variant++){
        bool bc7=variant==0,sign=variant==2;unsigned count=(bc7?8:14)*256,width=256,height=count/64*4;
        std::vector<uint8_t> bytes(size_t(count)*16);
        for(unsigned i=0;i<count;i++){
            auto* p=bytes.data()+i*16;for(unsigned b=0;b<16;b++){rng=rng*1664525+1013904223;p[b]=uint8_t(rng>>24);}
            unsigned mode=i/256;
            if(bc7)p[0]=uint8_t((p[0]&~((1u<<(mode+1))-1u))|(1u<<mode));
            else{unsigned m=modes[mode];p[0]=uint8_t((p[0]&~(m<2?3u:31u))|m);}
        }
        D3D11_TEXTURE2D_DESC td{};td.Width=width;td.Height=height;td.MipLevels=1;td.ArraySize=1;td.Format=bc7?DXGI_FORMAT_BC7_UNORM:sign?DXGI_FORMAT_BC6H_SF16:DXGI_FORMAT_BC6H_UF16;td.SampleDesc.Count=1;td.Usage=D3D11_USAGE_DEFAULT;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA initial{};initial.pSysMem=bytes.data();initial.SysMemPitch=64*16;
        Com<ID3D11Texture2D> texture;check(device->CreateTexture2D(&td,&initial,texture.address()),"Create compressed oracle texture");
        Com<ID3D11ShaderResourceView> view;check(device->CreateShaderResourceView(texture.p,nullptr,view.address()),"Oracle sampled view");
        D3D11_BUFFER_DESC bd{};bd.ByteWidth=width*height*16;bd.Usage=D3D11_USAGE_DEFAULT;bd.BindFlags=D3D11_BIND_UNORDERED_ACCESS;bd.MiscFlags=D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;bd.StructureByteStride=16;
        Com<ID3D11Buffer> output;check(device->CreateBuffer(&bd,nullptr,output.address()),"Oracle result buffer");
        Com<ID3D11UnorderedAccessView> uav;check(device->CreateUnorderedAccessView(output.p,nullptr,uav.address()),"Oracle result UAV");
        bd.Usage=D3D11_USAGE_STAGING;bd.BindFlags=0;bd.CPUAccessFlags=D3D11_CPU_ACCESS_READ;bd.MiscFlags=0;bd.StructureByteStride=0;
        Com<ID3D11Buffer> staging;check(device->CreateBuffer(&bd,nullptr,staging.address()),"Oracle readback buffer");
        context->CSSetShader(shader.p,nullptr,0);context->CSSetShaderResources(0,1,&view.p);context->CSSetUnorderedAccessViews(0,1,&uav.p,nullptr);
        context->Dispatch((width+7)/8,(height+7)/8,1);ID3D11UnorderedAccessView* none=nullptr;ID3D11ShaderResourceView* noView=nullptr;
        context->CSSetUnorderedAccessViews(0,1,&none,nullptr);context->CSSetShaderResources(0,1,&noView);
        context->CopyResource(staging.p,output.p);D3D11_MAPPED_SUBRESOURCE mapped{};check(context->Map(staging.p,0,D3D11_MAP_READ,0,&mapped),"Oracle readback map");
        std::vector<float> actual(size_t(width)*height*4);std::memcpy(actual.data(),mapped.pData,actual.size()*sizeof(float));context->Unmap(staging.p,0);
        for(unsigned i=0;i<count;i++){
            const auto* block=bytes.data()+i*16;
            auto u=bc7?mavk::compat::decodeBc7(4,4,block,16):mavk::compat::DecodedBc{};
            auto f=bc7?mavk::compat::DecodedBcChannels{}:mavk::compat::decodeBc6h(sign,4,4,block,16);
            for(unsigned y=0;y<4;y++)for(unsigned x=0;x<4;x++)for(unsigned c=0;c<4;c++){
                float value=actual[(((i/64)*4+y)*width+(i%64)*4+x)*4+c];
                float expected=bc7?float(u.rgba[(y*4+x)*4+c])/255.0f:f.rgba[(y*4+x)*4+c];
                if(!std::isfinite(value)||std::abs(value-expected)>std::max(1e-9f,std::abs(expected)*1e-6f)){
                    std::fprintf(stderr,"WARP mismatch variant=%u block=%u mode=%u pixel=%u ch=%u actual=%.9g expected=%.9g input=",variant,i,i/256,y*4+x,c,value,expected);for(unsigned b=0;b<16;b++)std::fprintf(stderr,"%02x",block[b]);std::fputc('\n',stderr);return 2;
                }
                checked++;
            }
        }
        std::printf("MICROSOFT_WARP_BC_ORACLE_SLICE_PASS variant=%u blocks=%u\n",variant,count);jobs++;
    }
    std::printf("BC67_MICROSOFT_WARP_ORACLE_PASS components=%llu jobs=%u NOT_ANDROID_GPU_OR_FULL_BC_API_PASS\n",static_cast<unsigned long long>(checked),jobs);return 0;
}catch(const std::exception& e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
