#include <mavk/sampled.hpp>
#include <cassert>
#include <cstdio>
int main(){
    auto a=mavk::rgbaMipLayout(1024,1024,11);assert(a.size()==11&&a.back().offset+a.back().bytes==5592404&&a.back().width==1&&a.back().height==1);
    auto b=mavk::rgbaMipLayout(7,5,3);assert(b[0].bytes==140&&b[1].offset==140&&b[1].bytes==24&&b[2].offset==164&&b[2].bytes==4);
    auto c=mavk::rgbaMipLayout(1,1024,11);assert(c.back().width==1&&c.back().height==1&&c.back().offset+c.back().bytes==8188);
    unsigned rejected=0;
    for(auto bad:std::vector<std::vector<uint32_t>>{{0,1,1},{1,0,1},{1,1,0},{1025,1,1},{1,1025,1},{1,1,2},{8,8,5},{1024,1024,12},{UINT32_MAX,1,1}}){
        bool caught=false;try{(void)mavk::rgbaMipLayout(bad[0],bad[1],bad[2]);}catch(const std::invalid_argument&){caught=true;}assert(caught);rejected++;
    }
    mavk::Counters counters;assert(counters.empty());counters.samplers=1;assert(!counters.empty());counters.samplers=0;counters.imageViews=1;assert(!counters.empty());
    counters.imageViews=0;counters.timelineSemaphores=1;assert(!counters.empty());counters.timelineSemaphores=0;assert(counters.empty());
    std::printf("RGBA_MIP_LAYOUT_CPU_PASS rejects=%u largest_bytes=5592404 NOT_GPU_PASS\n",rejected);
}
