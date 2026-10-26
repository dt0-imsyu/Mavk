// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <chrono>
#include <cstdio>
#include <fstream>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

namespace mavk::diagnostics {
// CPU-side causality only. Never adds queries, commands, waits or GPU signals.
// Serialized by the existing device mutex. Buffer during capture, print AFTER it.
class SyncTrace {
    struct Event {char kind;uint64_t time,id,a,b,c,d,thread;};
    std::string arm_,tag_;std::vector<Event> events_;
    uint64_t poll_=0,until_=0,sequence_=0;bool recording_=false;
    void flush(bool truncated=false) noexcept {
        if(!recording_)return;recording_=false;
        std::fprintf(stderr,"MAVK_SYNC_BEGIN tag=%s events=%zu truncated=%u\n",tag_.c_str(),events_.size(),unsigned(truncated));
        for(const auto& e:events_)std::fprintf(stderr,"MAVK_SYNC %c %llu %llu %llu %llu %llu %llu %llu\n",e.kind,(unsigned long long)e.time,(unsigned long long)e.id,(unsigned long long)e.a,(unsigned long long)e.b,(unsigned long long)e.c,(unsigned long long)e.d,(unsigned long long)e.thread);
        std::fprintf(stderr,"MAVK_SYNC_END tag=%s\n",tag_.c_str());events_.clear();
    }
public:
    explicit SyncTrace(std::string arm):arm_(std::move(arm)){events_.reserve(8192);}
    ~SyncTrace(){flush();}
    static uint64_t now(){return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
    template<class T>static uint64_t bits(T value){if constexpr(std::is_pointer_v<T>)return uint64_t(reinterpret_cast<uintptr_t>(value));else return uint64_t(value);}
    bool active() noexcept {
        const auto t=now();if(recording_&&t>=until_)flush();
        if(!recording_&&t>=poll_)try{
            poll_=t+1000000000ull;std::ifstream file(arm_);std::string tag;file>>tag;
            bool valid=!tag.empty()&&tag.size()<=32;for(char c:tag)valid=valid&&((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='-'||c=='_');
            if(valid&&tag!=tag_){tag_=std::move(tag);events_.clear();until_=t+2000000000ull;recording_=true;}
        }catch(...){poll_=t+1000000000ull;}
        return recording_;
    }
    void add(char kind,uint64_t id,uint64_t a=0,uint64_t b=0,uint64_t c=0,uint64_t d=0,uint64_t time=0) noexcept {
        if(!id||!active())return;if(events_.size()==8192){flush(true);return;}
        static thread_local const auto thread=uint64_t(std::hash<std::thread::id>{}(std::this_thread::get_id()));
        events_.push_back({kind,time?time:now(),id,a,b,c,d,thread});
    }
    uint64_t begin(char kind,uint64_t a=0,uint64_t b=0,uint64_t c=0,uint64_t d=0,uint64_t time=0) noexcept {
        if(!active())return 0;const auto id=++sequence_;add(kind,id,a,b,c,d,time);return id;
    }
    uint64_t submit(VkQueue queue,uint32_t count,const VkSubmitInfo* submits,uint64_t entry) noexcept {
        if(!active()||count>256)return 0;uint64_t commands=0;for(uint32_t i=0;i<count;i++)commands+=submits[i].commandBufferCount;
        auto id=begin('S',bits(queue),commands,entry,now());
        for(uint32_t i=0;i<count;i++){
            const auto& s=submits[i];const VkTimelineSemaphoreSubmitInfo* timeline=nullptr;
            for(auto p=static_cast<const VkBaseInStructure*>(s.pNext);p;p=p->pNext)if(p->sType==VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO)timeline=reinterpret_cast<const VkTimelineSemaphoreSubmitInfo*>(p);
            for(uint32_t j=0;j<s.waitSemaphoreCount&&j<128;j++)add('W',id,bits(s.pWaitSemaphores[j]),timeline&&j<timeline->waitSemaphoreValueCount?timeline->pWaitSemaphoreValues[j]:0,s.pWaitDstStageMask[j]);
            for(uint32_t j=0;j<s.signalSemaphoreCount&&j<128;j++)add('G',id,bits(s.pSignalSemaphores[j]),timeline&&j<timeline->signalSemaphoreValueCount?timeline->pSignalSemaphoreValues[j]:0);
        }return id;
    }
    uint64_t submit(VkQueue queue,uint32_t count,const VkSubmitInfo2* submits,uint64_t entry) noexcept {
        if(!active()||count>256)return 0;uint64_t commands=0;for(uint32_t i=0;i<count;i++)commands+=submits[i].commandBufferInfoCount;
        auto id=begin('S',bits(queue),commands,entry,now());
        for(uint32_t i=0;i<count;i++){
            const auto& s=submits[i];
            for(uint32_t j=0;j<s.waitSemaphoreInfoCount&&j<128;j++)add('W',id,bits(s.pWaitSemaphoreInfos[j].semaphore),s.pWaitSemaphoreInfos[j].value,s.pWaitSemaphoreInfos[j].stageMask);
            for(uint32_t j=0;j<s.signalSemaphoreInfoCount&&j<128;j++)add('G',id,bits(s.pSignalSemaphoreInfos[j].semaphore),s.pSignalSemaphoreInfos[j].value);
        }return id;
    }
};
}
