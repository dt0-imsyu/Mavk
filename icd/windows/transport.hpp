#pragma once
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <mutex>

namespace mavk::icd {
inline void put32(std::vector<uint8_t>& b,uint32_t v){for(unsigned i=0;i<4;i++)b.push_back(uint8_t(v>>(8*i)));}
inline uint32_t get32(const uint8_t* p){return p[0]|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;}
// Same installed WSP, port and stream protocol as the verified guest probe.
// No driver/protocol installation, registry, input or authentication changes.
// Initial prototype is bounded by the external finite test-process deadline;
// it is not yet a cancellable production loader transport.
class Transport {
    HMODULE wsp{};SOCKET listener=INVALID_SOCKET,peer=INVALID_SOCKET;bool started=false;uint64_t sequence=0;
    std::mutex wireMutex;
    void transfer(uint8_t* p,uint32_t bytes,bool write){while(bytes){int n=write?send(peer,reinterpret_cast<char*>(p),int(bytes),0):recv(peer,reinterpret_cast<char*>(p),int(bytes),0);if(n<=0)throw std::runtime_error("Mavk transport closed");p+=n;bytes-=uint32_t(n);}}
public:
    ~Transport(){if(peer!=INVALID_SOCKET){shutdown(peer,SD_BOTH);closesocket(peer);}if(listener!=INVALID_SOCKET)closesocket(listener);if(started)WSACleanup();if(wsp)FreeLibrary(wsp);}
    void open(){
        struct Address{ADDRESS_FAMILY family;USHORT reserved;UINT port,cid;};static_assert(sizeof(Address)==12);
        HANDLE d=CreateFileW(L"\\??\\Viosock",GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(d==INVALID_HANDLE_VALUE)throw std::runtime_error("Viosock access");DWORD af=0,n=0;BOOL ok=DeviceIoControl(d,0x0801300C,nullptr,0,&af,4,&n,nullptr);CloseHandle(d);if(!ok||n!=4||!af)throw std::runtime_error("Viosock family");
        wsp=LoadLibraryW(L"viosocklib.dll");if(!wsp)throw std::runtime_error("Installed WSP unavailable");WSADATA w{};if(WSAStartup(MAKEWORD(2,2),&w))throw std::runtime_error("Winsock initialization");started=true;
        listener=socket(int(af),SOCK_STREAM,0);Address address{ADDRESS_FAMILY(af),0,4057,UINT32_MAX};if(listener==INVALID_SOCKET||bind(listener,reinterpret_cast<sockaddr*>(&address),sizeof(address))||listen(listener,1))throw std::runtime_error("Mavk listener");
        const char ready[]="MAVK_ICD_LISTEN_READY=4057\r\n";DWORD written;WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),ready,sizeof(ready)-1,&written,nullptr);
        Address remote{};int len=sizeof(remote);peer=accept(listener,reinterpret_cast<sockaddr*>(&remote),&len);if(peer==INVALID_SOCKET)throw std::runtime_error("Mavk accept");
        auto hello=exchange(1,{});if(hello.size()!=16||get32(hello.data())!=1||get32(hello.data()+4)!=1048576)throw std::runtime_error("Mavk protocol mismatch");
    }
    std::vector<uint8_t> exchange(uint32_t op,const std::vector<uint8_t>& payload){
        std::lock_guard<std::mutex> lock(wireMutex);
        if(payload.size()>1048576)throw std::runtime_error("Mavk request limit");std::vector<uint8_t> frame;put32(frame,0x314b564d);put32(frame,0x00200001);put32(frame,op);put32(frame,0);++sequence;put32(frame,uint32_t(sequence));put32(frame,uint32_t(sequence>>32));put32(frame,uint32_t(payload.size()));put32(frame,0);frame.insert(frame.end(),payload.begin(),payload.end());transfer(frame.data(),uint32_t(frame.size()),true);
        uint8_t h[32];transfer(h,32,false);uint32_t bytes=get32(h+24);if(get32(h)!=0x314b564d||get32(h+4)!=0x00200001||get32(h+8)!=op||get32(h+12)!=1||get32(h+16)!=uint32_t(sequence)||get32(h+20)!=uint32_t(sequence>>32)||get32(h+28)||bytes>1048576)throw std::runtime_error("Mavk response rejected");std::vector<uint8_t> out(bytes);if(bytes)transfer(out.data(),bytes,false);return out;
    }
};
}
