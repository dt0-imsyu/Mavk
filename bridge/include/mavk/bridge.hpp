#pragma once
#include <mavk/core.hpp>
#include <memory>
#include <vector>
#include <cstdint>

namespace mavk::bridge {
constexpr uint32_t Magic=0x314b564d,HeaderBytes=32,MaxPayload=1024*1024;
enum class Op:uint32_t {Hello=1,CreateBuffer=2,DestroyBuffer=3,DispatchVerify=4,ReadBuffer=5,DrawTriangle=6,DeviceIdentity=7,DrawOffscreen=8,DrawVertices=10,DrawIndexed=11,DrawIndexedPush=12,DrawScene=13,RenderSnapshot=14,ReadSnapshot=15,DestroySnapshot=16,RenderUniformSnapshot=17,DispatchStorage=18,CreateSampled=19,WriteSampled=20,CommitSampled=21,DestroySampled=22,RenderSampledSnapshot=23,RenderLayersSnapshot=24};
enum class Status:uint32_t {Ok=0,BadHeader=1,BadPayload=2,Unsupported=3,NotFound=4,Limit=5,GpuFailure=6,BadSequence=7,NotReady=8};
// No Vulkan handles, addresses, native struct layouts or OS handles on the wire.
// Session is single-threaded; bounded resources are private to one connection.
class Session {
public:
    Session(DeviceContext&,VkQueue,const uint32_t* trustedShader,size_t bytes);
    ~Session();Session(const Session&)=delete;Session& operator=(const Session&)=delete;
    std::vector<uint8_t> exchange(const std::vector<uint8_t>& request);
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
