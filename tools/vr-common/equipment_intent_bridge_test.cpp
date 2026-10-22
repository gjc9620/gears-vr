#include "equipment_intent_bridge.h"
#include <cassert>
#include <cstdio>

int main(){
    gears_weapon::EquipmentIntentBridge publisher,reader,duplicate;
    assert(publisher.open(true));
    assert(!duplicate.open(true));
    assert(reader.open(false));
    std::uint64_t pose_cursor=UINT64_MAX;
    gears_weapon::HandWorldFrame world_pose{};
    assert(!reader.read_pose(pose_cursor,world_pose)&&pose_cursor==0);
    gears_weapon::HandWorldFrame published_pose{};
    published_pose.session=7;published_pose.sequence=12;published_pose.space_epoch=3;
    published_pose.world_generation=4;published_pose.controller=0x1000;
    published_pose.pawn=0x2000;published_pose.weapon=0x3000;
    published_pose.right={{10,20,30},{0,.5,0}};published_pose.valid_mask=2;
    assert(publisher.publish_pose(published_pose));
    assert(reader.read_pose(pose_cursor,world_pose));
    assert(world_pose.serial==1&&world_pose.right.position.x==10&&
           world_pose.right.rotation.yaw==.5);
    assert(!reader.read_pose(pose_cursor,world_pose));
    published_pose.valid_mask=0;
    assert(!publisher.publish_pose(published_pose));
    std::uint64_t cursor=UINT64_MAX;
    gears_weapon::EquipmentIntentEvent received{};
    assert(!reader.read_next(cursor,received)&&cursor==0);
    gears_weapon::EquipmentIntentEvent event{};
    event.input_session=7;event.input_sequence=12;event.space_epoch=3;
    event.world_generation=4;event.controller=0x1000;event.pawn=0x2000;
    event.weapon=0x3000;event.inventory_slot=2;
    assert(publisher.publish(event));
    assert(reader.read_next(cursor,received));
    assert(received.serial==1&&received.input_sequence==12&&
           received.controller==event.controller&&received.inventory_slot==2);
    assert(!reader.read_next(cursor,received));
    event.kind=gears_weapon::EquipmentIntentKind::grab_ammo;
    event.inventory_slot=4;event.input_sequence=13;
    assert(publisher.publish(event));
    assert(reader.read_next(cursor,received));
    assert(received.kind==gears_weapon::EquipmentIntentKind::grab_ammo&&
           received.inventory_slot==4);
    event.kind=gears_weapon::EquipmentIntentKind::slot;
    event.inventory_slot=4;
    assert(!publisher.publish(event));
    event.inventory_slot=3;
    for(unsigned i=0;i<gears_weapon::EquipmentIntentPacket::capacity+1;++i){
        event.input_sequence=14+i;
        assert(publisher.publish(event));
    }
    assert(!reader.read_next(cursor,received)); // no replay after overflow
    assert(cursor==gears_weapon::EquipmentIntentPacket::capacity+3);
    event.input_sequence=100;
    assert(publisher.publish(event));
    Sleep(260);
    assert(!reader.read_next(cursor,received)); // no stale grip actuation
    assert(cursor==gears_weapon::EquipmentIntentPacket::capacity+4);
    std::puts("equipment intent bridge passed");
}
