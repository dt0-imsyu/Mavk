#pragma once
#include <cstdint>
#include <map>
#include <stdexcept>
#include <vector>

inline std::vector<uint32_t> groupDescriptorFixture(const std::vector<uint32_t>& code,size_t* count=nullptr){
    if(code.size()<5||code.size()>262144||code[0]!=0x07230203)throw std::invalid_argument("bounded group fixture module");
    std::map<std::pair<uint32_t,uint32_t>,std::vector<uint32_t>> targets;
    for(size_t p=5;p<code.size();){const auto n=code[p]>>16,op=code[p]&65535;if(!n||n>code.size()-p)throw std::invalid_argument("malformed fixture");
        if(op==73||op==74||op==75)throw std::invalid_argument("fixture already grouped");
        if(op==71&&n==4&&(code[p+2]==33||code[p+2]==34))targets[{code[p+2],code[p+3]}].push_back(code[p+1]);p+=n;}
    if(targets.empty()||code[3]>UINT32_MAX-targets.size())throw std::invalid_argument("fixture descriptor groups");
    std::vector<uint32_t> out(code.begin(),code.begin()+5);bool inserted=false;uint32_t next=code[3];
    for(size_t p=5;p<code.size();p+=code[p]>>16){const bool descriptor=(code[p]&65535)==71&&(code[p]>>16)==4&&(code[p+2]==33||code[p+2]==34);
        if(descriptor&&!inserted){for(const auto& [key,ids]:targets){const auto group=next++;out.insert(out.end(),{(4u<<16)|71,group,key.first,key.second,(2u<<16)|73,group,(uint32_t(ids.size()+2)<<16)|74,group});out.insert(out.end(),ids.begin(),ids.end());}inserted=true;}
        if(!descriptor)out.insert(out.end(),code.begin()+p,code.begin()+p+(code[p]>>16));}
    out[3]=next;if(count)*count=targets.size();return out;
}
