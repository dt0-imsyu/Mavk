#pragma once
#include "triangle-renderer.h"
static AhbWayland *ahb_draw_owner;
static void ahb_wl_record_draw(VkCommandBuffer command,AhbPoolSlot *slot,unsigned frame){
    mavk_triangle_record(command,(unsigned)(slot-ahb_draw_owner->slots),frame);
}
static int ahb_wl_draw_setup(AhbWayland *owner){
    VkImage images[3];for(unsigned i=0;i<3;i++)images[i]=owner->slots[i].image;
    int result=mavk_triangle_setup(images,3,owner->width,owner->height,VK_FORMAT_R8G8B8A8_UNORM);
    if(!result){ahb_draw_owner=owner;pool_draw_hook=ahb_wl_record_draw;}
    return result;
}
static void ahb_wl_draw_destroy(void){
    pool_draw_hook=NULL;ahb_draw_owner=NULL;mavk_triangle_destroy();
}
