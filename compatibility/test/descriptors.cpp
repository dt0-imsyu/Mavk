#include <mavk/compat/descriptors.hpp>
#include <iostream>
using namespace mavk::compat;
unsigned assertions=0,negative=0;
void check(bool pass){assertions++;if(!pass)throw std::runtime_error("descriptor assertion");}
template<class F>void reject(F f){try{f();}catch(const std::exception&){negative++;return;}throw std::runtime_error("invalid descriptor accepted");}
uint32_t value(const DescriptorSnapshot& s,uint32_t set,uint32_t binding,uint32_t i){auto& d=s.metadata;uint32_t table=s.abi==GuardMetadataAbi::BufferRanges?d[set+1]:d[d[0]+set];return d[d[table+binding]+i];}
int main(){try{
    using K=DescriptorKind;using A=GuardMetadataAbi;
    auto owner=std::make_shared<int>(1);
    DescriptorState state({{2,9,3,K::StorageBuffer},{0,4,1,K::CombinedImageSampler},{2,1,1,K::UniformBuffer}}, {16,8});
    reject([&]{state.snapshot(A::IndexAndInit);});state.bindImage(0,4,0,owner);state.bindBuffer(2,1,0,owner,1024,16,32);
    state.setNull(2,9,0);state.bindBuffer(2,9,1,owner,1024,8,256);state.setNull(2,9,2);
    auto gen=state.generation();
    for(auto abi:{A::BufferRanges,A::IndexAndInit}){
        auto snap=state.snapshot(abi);check(snap->generation==gen);check(snap==state.snapshot(abi));
        check(value(*snap,0,4,0)==1);check(value(*snap,2,1,0)==32);
        check(value(*snap,2,9,0)==0);check(value(*snap,2,9,1)==256);check(value(*snap,2,9,2)==0);
        if(abi==A::IndexAndInit)check(snap->metadata[snap->metadata[3]+9]==3);
        check(snap->at(2,9,1).offset==8);check(snap->at(2,9,0).status==DescriptorStatus::Null);
    }
    auto before=state.snapshot(A::IndexAndInit);state.copyFrom(state,2,9,0,2,9,1,2);auto after=state.snapshot(A::IndexAndInit);
    check(before!=after);check(value(*before,2,9,1)==256);check(value(*after,2,9,1)==0);check(value(*after,2,9,2)==256);
    state.setNull(2,9,0);check(state.snapshot(A::IndexAndInit)==after);
    state.bindBuffer(2,9,0,owner,1024,8,UINT64_MAX);check(value(*state.snapshot(A::IndexAndInit),2,9,0)==1016);
    auto stable=state.snapshot(A::IndexAndInit);auto unchanged=state.generation();
    reject([&]{state.bindBuffer(2,9,0,owner,1024,3,16);});reject([&]{state.bindBuffer(2,1,0,owner,1024,8,16);});
    reject([&]{state.bindBuffer(2,9,0,owner,1024,8,0);});reject([&]{state.bindBuffer(2,9,0,owner,1024,8,UINT64_MAX-1);});
    reject([&]{state.bindBuffer(2,9,0,{},1024,0,16);});reject([&]{state.bindBuffer(2,9,0,owner,1024,1024,16);});
    reject([&]{state.bindBuffer(2,9,0,owner,UINT64_MAX,0,uint64_t(UINT32_MAX)+1);});
    reject([&]{state.bindImage(2,9,0,owner);});reject([&]{state.bindBuffer(0,4,0,owner,1024,0,16);});reject([&]{state.bindImage(0,4,0,{});});
    reject([&]{state.setNull(2,9,3);});reject([&]{state.setNull(0,5,0);});reject([&]{state.copyFrom(state,2,9,0,2,9,2,2);});
    reject([&]{state.copyFrom(state,2,1,0,2,9,0,1);});reject([&]{state.snapshot(static_cast<A>(999));});
    check(state.generation()==unchanged);check(state.snapshot(A::IndexAndInit)==stable);
    DescriptorState blank({{0,0,1,K::StorageBuffer}});reject([&]{state.copyFrom(blank,0,0,0,2,9,0,1);});
    reject([&]{DescriptorState bad({{0,0,0,K::StorageBuffer}});});reject([&]{DescriptorState bad({{8,0,1,K::StorageBuffer}});});
    reject([&]{DescriptorState bad({{0,1024,1,K::StorageBuffer}});});reject([&]{DescriptorState bad({{0,0,1,K::StorageBuffer},{0,0,1,K::StorageBuffer}});});
    reject([&]{DescriptorState bad({{0,0,1,static_cast<K>(999)}});});reject([&]{DescriptorState bad({{0,0,1,K::StorageBuffer}}, {0,1});});
    bool retired=false;std::shared_ptr<const DescriptorSnapshot> inFlight;
    {DescriptorState life({{0,0,1,K::StorageBuffer}});auto resource=std::shared_ptr<int>(new int(7),[&](int* p){retired=true;delete p;});
     life.bindBuffer(0,0,0,resource,64,0,64);inFlight=life.snapshot(A::IndexAndInit);resource.reset();life.setNull(0,0,0);check(!retired);}
    check(!retired);inFlight.reset();check(retired);
    {int object=0;auto first=std::shared_ptr<int>(&object,[](int*){}),second=std::shared_ptr<int>(&object,[](int*){});
     DescriptorState identity({{0,0,1,K::StorageBuffer}});identity.bindBuffer(0,0,0,first,16,0,16);auto snapshot=identity.snapshot(A::IndexAndInit);auto old=identity.generation();
     identity.bindBuffer(0,0,0,second,16,0,16);check(identity.generation()==old+1);check(identity.snapshot(A::IndexAndInit)!=snapshot);}
    {DescriptorState source({{0,0,2,K::StorageBuffer}}),destination({{0,0,2,K::StorageBuffer}});
     source.bindBuffer(0,0,0,owner,16,0,16);destination.setNull(0,0,0);destination.setNull(0,0,1);auto before=destination.snapshot(A::IndexAndInit);
     reject([&]{destination.copyFrom(source,0,0,0,0,0,0,2);});check(destination.snapshot(A::IndexAndInit)==before);}
    DescriptorState strict({{0,0,1,K::StorageBuffer}}, {1,64});DescriptorState lax({{0,0,1,K::StorageBuffer}});
    lax.bindBuffer(0,0,0,owner,128,8,32);reject([&]{strict.copyFrom(lax,0,0,0,0,0,0,1);});check(strict.generation()==0);
    std::cout<<"DESCRIPTOR_STATE_PASS assertions="<<assertions<<" negatives="<<negative<<"\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
