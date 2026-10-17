#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <stdint.h>
#include <stddef.h>

// Disposable guest diagnostic, not a Vulkan ICD. No CRT, driver install,
// registry/boot changes or display access. Only the existing signed viosock WSP.
static HANDLE deadlineEvent;
static uint8_t* response;
static uint64_t sequence;
static uint32_t responseBytes;
void* memset(void* dst,int v,size_t n){volatile unsigned char* p=dst;while(n--)*p++=(unsigned char)v;return dst;}
void* memcpy(void* dst,const void* src,size_t n){volatile unsigned char* p=dst;const volatile unsigned char* q=src;while(n--)*p++=*q++;return dst;}
static void log_text(const char* text){DWORD n=0;while(text[n])n++;DWORD done;WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),text,n,&done,NULL);}
static void log_number(const char* key,uint64_t value){char text[32];unsigned i=0;do{text[i++]=(char)('0'+value%10);value/=10;}while(value);log_text(key);while(i){char one[2]={text[--i],0};log_text(one);}log_text("\r\n");}
static uint32_t get32(const uint8_t* p){return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static uint64_t get64(const uint8_t* p){return get32(p)|((uint64_t)get32(p+4)<<32);}
static void put32(uint8_t* p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8));}
static void put64(uint8_t* p,uint64_t v){put32(p,(uint32_t)v);put32(p+4,(uint32_t)(v>>32));}
static void header(uint8_t* p,uint32_t op,uint64_t id,uint32_t bytes){put32(p,0x314b564d);put32(p+4,0x00200001);put32(p+8,op);put32(p+12,0);put64(p+16,id);put32(p+24,bytes);put32(p+28,0);}
static int wire_selftest(void){uint8_t h[32];header(h,4,0x0102030405060708ull,12);if(get32(h)!=0x314b564d||get32(h+4)!=0x00200001||get64(h+16)!=0x0102030405060708ull||h[16]!=8||h[23]!=1||get32(h+24)!=12||get32(h+28))return 10;uint32_t seed=0xfedcba98u;put32(h,((65535u*1664525u+1013904223u)^seed));if(get32(h)!=((65535u*1664525u+1013904223u)^seed))return 11;log_text("MAVK_ACTUAL_CLIENT_WIRE_SELFTEST_PASS\r\n");return 0;}
static int send_all(SOCKET s,const uint8_t* p,uint32_t n){while(n){int k=send(s,(const char*)p,(int)n,0);if(k<=0)return 0;p+=k;n-=(uint32_t)k;}return 1;}
static int recv_all(SOCKET s,uint8_t* p,uint32_t n){while(n){int k=recv(s,(char*)p,(int)n,0);if(k<=0)return 0;p+=k;n-=(uint32_t)k;}return 1;}
static int exchange(SOCKET s,uint32_t op,const uint8_t* data,uint32_t bytes,uint32_t status){uint8_t h[32];header(h,op,++sequence,bytes);if(!send_all(s,h,32)||(bytes&&!send_all(s,data,bytes))||!recv_all(s,response,32))return 0;responseBytes=get32(response+24);if(get32(response)!=0x314b564d||get32(response+4)!=0x00200001||get32(response+8)!=op||get32(response+12)!=1||get64(response+16)!=sequence||responseBytes>1048576||get32(response+28)!=status)return 0;return !responseBytes||recv_all(s,response+32,responseBytes);}
static DWORD WINAPI deadline(void* ignored){(void)ignored;if(WaitForSingleObject(deadlineEvent,60000)==WAIT_TIMEOUT){log_text("MAVK_PROBE_DEADLINE_60S\r\n");ExitProcess(124);}return 0;}
typedef struct {ADDRESS_FAMILY family;USHORT reserved;UINT port,cid;} VsockAddress;
_Static_assert(sizeof(VsockAddress)==12,"viosock ABI");
static int run_gpu(void){
    int result=1,started=0;HANDLE device=INVALID_HANDLE_VALUE,thread=NULL;HMODULE wsp=NULL;SOCKET listener=INVALID_SOCKET,peer=INVALID_SOCKET;WSADATA wsa;DWORD af=0,n=0;
    log_text("MAVK_WINDOWS_COMPUTE_PROBE_V1; NOT_VULKAN_ICD_OR_DISPLAY_PASS\r\n");
    device=CreateFileW(L"\\??\\Viosock",GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(device==INVALID_HANDLE_VALUE){log_number("VIOSOCK_OPEN_ERROR=",GetLastError());goto done;}
    if(!DeviceIoControl(device,0x0801300C,NULL,0,&af,4,&n,NULL)||n!=4||!af){log_number("VIOSOCK_AF_ERROR=",GetLastError());goto done;}
    wsp=LoadLibraryW(L"viosocklib.dll");if(!wsp){log_number("WSP_LOAD_ERROR=",GetLastError());goto done;}
    if(WSAStartup(MAKEWORD(2,2),&wsa))goto done;started=1;
    listener=socket((int)af,SOCK_STREAM,0);if(listener==INVALID_SOCKET){log_number("SOCKET_ERROR_NO_PROTOCOL_INSTALL=",WSAGetLastError());goto done;}
    VsockAddress address={(ADDRESS_FAMILY)af,0,4057,UINT32_MAX};if(bind(listener,(struct sockaddr*)&address,sizeof(address))||listen(listener,1)){log_number("LISTEN_ERROR=",WSAGetLastError());goto done;}
    deadlineEvent=CreateEventW(NULL,TRUE,FALSE,NULL);if(!deadlineEvent)goto done;thread=CreateThread(NULL,0,deadline,NULL,0,NULL);if(!thread)goto done;log_text("LISTEN_READY=4057\r\n");
    // The installed WSP dereferences accept's peer length unconditionally.
    // Match the known-good display client's valid output buffers.
    VsockAddress remote={0};int remoteBytes=sizeof(remote);
    peer=accept(listener,(struct sockaddr*)&remote,&remoteBytes);if(peer==INVALID_SOCKET)goto done;
    // WSP options are optional; the whole-process deadline already bounds I/O.
    DWORD timeout=10000;if(setsockopt(peer,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout)))log_number("RECV_TIMEOUT_OPTION_ERROR=",WSAGetLastError());
    if(setsockopt(peer,SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeout,sizeof(timeout)))log_number("SEND_TIMEOUT_OPTION_ERROR=",WSAGetLastError());
    response=HeapAlloc(GetProcessHeap(),0,1048576+32);if(!response)goto done;
#ifndef MAVK_BUNDLE_FETCH
    uint8_t payload[16];
#endif
    if(!exchange(peer,1,NULL,0,0)||responseBytes!=16||get32(response+32)!=1||get32(response+36)!=1048576)goto done;
#if defined(MAVK_BUNDLE_FETCH)
#ifdef MAVK_BUNDLE_CHUNKED
    uint8_t slice[8]={0};
    if(!exchange(peer,9,slice,8,0)||responseBytes!=4)goto done;
    uint32_t bundleBytes=get32(response+32);if(!bundleBytes||bundleBytes>4194304)goto done;
#else
    if(!exchange(peer,9,NULL,0,0)||!responseBytes)goto done;
#endif
    wchar_t path[MAX_PATH];DWORD len=GetTempPathW(MAX_PATH,path);const wchar_t tail[]=L"MavkIcdDevelopment1.zip";
    if(!len||len>=MAX_PATH-sizeof(tail)/sizeof(wchar_t))goto done;
    memcpy(path+len,tail,sizeof(tail));
    HANDLE file=CreateFileW(path,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);if(file==INVALID_HANDLE_VALUE){log_number("BUNDLE_CREATE_NEW_ERROR=",GetLastError());goto done;}
    DWORD written=0;BOOL saved=TRUE;
#ifdef MAVK_BUNDLE_CHUNKED
    for(uint32_t offset=0;offset<bundleBytes;){uint32_t bytes=bundleBytes-offset;if(bytes>1048576)bytes=1048576;put32(slice,offset);put32(slice+4,bytes);
        if(!exchange(peer,9,slice,8,0)||responseBytes!=bytes||!WriteFile(file,response+32,bytes,&written,NULL)||written!=bytes){saved=FALSE;break;}offset+=bytes;}
    saved=saved&&FlushFileBuffers(file);
#else
    saved=WriteFile(file,response+32,responseBytes,&written,NULL)&&written==responseBytes&&FlushFileBuffers(file);
#endif
    CloseHandle(file);
    if(!saved){DeleteFileW(path);goto done;}
#ifdef MAVK_BUNDLE_CHUNKED
    log_number("BUNDLE_BYTES=",bundleBytes);
#else
    log_number("BUNDLE_BYTES=",responseBytes);
#endif
    log_text("FIXED_DEVELOPMENT_BUNDLE_RECEIVED_NOT_EXECUTED\r\n");result=0;
#elif defined(MAVK_GRAPHICS_PROBE)
    put32(payload,0);put32(payload+4,64);if(!exchange(peer,6,payload,8,5))goto done;
    put32(payload,64);if(!exchange(peer,6,payload,8,0)||responseBytes!=64*64*4)goto done;
    uint32_t checked=0;
    for(int y=0;y<64;y++)for(int x=0;x<64;x++){
        float nx=2*(x+.5f)/64-1,ny=2*(y+.5f)/64-1,absnx=nx<0?-nx:nx;
        float side=.375f-.5f*ny-absnx;
        int inside=ny>-.67f&&ny<.67f&&side>.08f,outside=ny<-.83f||ny>.83f||side<-.08f;
        if(!inside&&!outside)continue;
        const uint8_t* pixel=response+32+(y*64+x)*4;
        int expected[4]={inside?255:0,inside?64:0,0,255};
        for(int k=0;k<4;k++){int delta=(int)pixel[k]-expected[k];if(delta < -1||delta > 1){log_number("PIXEL_MISMATCH_INDEX=",(uint32_t)(y*64+x));goto done;}}
        checked++;
    }
    if(checked!=3330)goto done;
    log_number("GPU_VERIFIED_PIXELS=",checked);log_number("COMPLETED_COMMANDS=",sequence);log_text("MAVK_WINDOWS_VSOCK_GRAPHICS_PASS\r\n");result=0;
#else
    if(!exchange(peer,999,NULL,0,3))goto done;
    put32(payload,65536);if(!exchange(peer,2,payload,4,0)||responseBytes!=8)goto done;uint64_t handle=get64(response+32);put64(payload,handle);put32(payload+8,0);put32(payload+12,65536);if(!exchange(peer,5,payload,16,8))goto done;
    uint64_t words=0;for(uint32_t round=0;round<8;round++){
        uint32_t seed=round*747796405u+2891336453u;put64(payload,handle);put32(payload+8,seed);if(!exchange(peer,4,payload,12,0))goto done;
        put32(payload+8,0);put32(payload+12,65536);if(!exchange(peer,5,payload,16,0)||responseBytes!=65536)goto done;
        for(uint32_t i=0;i<16384;i++){if(get32(response+32+i*4)!=((i*1664525u+1013904223u)^seed)){log_number("GPU_MISMATCH_WORD=",i);goto done;}words++;}
    }
    put64(payload,handle);if(!exchange(peer,3,payload,8,0)||!exchange(peer,3,payload,8,4))goto done;
    put32(payload,1024);if(!exchange(peer,2,payload,4,0))goto done;
    log_number("GPU_VERIFIED_WORDS=",words);log_number("COMPLETED_COMMANDS=",sequence);log_text("MAVK_WINDOWS_VSOCK_COMPUTE_PASS\r\n");result=0;
#endif
done:
    if(deadlineEvent)SetEvent(deadlineEvent);if(thread){WaitForSingleObject(thread,1000);CloseHandle(thread);}if(deadlineEvent)CloseHandle(deadlineEvent);
    if(peer!=INVALID_SOCKET){shutdown(peer,SD_BOTH);closesocket(peer);}if(listener!=INVALID_SOCKET)closesocket(listener);if(started)WSACleanup();if(wsp)FreeLibrary(wsp);if(device!=INVALID_HANDLE_VALUE)CloseHandle(device);if(response)HeapFree(GetProcessHeap(),0,response);log_number("PROBE_EXIT=",(uint64_t)result);return result;
}
void mavk_entry(void){const wchar_t* cmd=GetCommandLineW();int selftest=0;for(unsigned i=0;cmd[i];i++)if(cmd[i]==L'-'&&cmd[i+1]==L'-'&&cmd[i+2]==L'w'){selftest=1;break;}ExitProcess((UINT)(selftest?wire_selftest():run_gpu()));}
