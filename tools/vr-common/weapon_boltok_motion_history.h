#pragma once
#include "controller_relative_motion.h"

// Original producer rows retained across consumer ticks. This is not a Frame,
// permit or renewed clock; controller_relative_motion::build still owns the
// held control/token and all geometry checks.
namespace gears_boltok_motion_history {
using Row=gears_hand_continuity::TrackedInput;
using Publisher=gears_hand_continuity::Publisher;
using History=gears_hand_continuity::MotionHistory;
namespace source=gears_hand_continuity;

class Buffer {
    History rows_{};Publisher publisher_{};bool bound_{};
    static bool valid_incoming(Publisher publisher,History const& incoming,Row const& current) noexcept {
        if(!publisher.pid||!publisher.start||incoming.reserved||!incoming.count||incoming.count>source::motion_history_capacity||
           !source::tracked_input_shape_valid(current)||
           !source::same_tracking(incoming.frames[incoming.count-1],current)||current.input.focused!=1)
            return false;
        for(unsigned i=0;i<incoming.count;++i)
            if(!source::tracked_input_shape_valid(incoming.frames[i]))return false;
        for(unsigned i=1;i<incoming.count;++i)
            if(!source::motion_pair(incoming.frames[i-1],incoming.frames[i],0,100000000)||
               !source::motion_pair(incoming.frames[i-1],incoming.frames[i],1,100000000))return false;
        return true;
    }
    static bool compose(History const& prior,History const& incoming,History& out) noexcept {
        // Merge by reference directly into the caller's bounded output. Do not
        // put another64 full controller frames on the game-thread stack.
        out={};unsigned a{},b{};Row const* last{};
        const auto current_sequence=incoming.frames[incoming.count-1].input.sequence;
        const auto first=current_sequence>=source::motion_history_capacity?
            current_sequence-source::motion_history_capacity+1:1;
        while(a<prior.count||b<incoming.count){
            Row const* next{};
            if(a>=prior.count)next=&incoming.frames[b++];
            else if(b>=incoming.count)next=&prior.frames[a++];
            else {
                auto const& x=prior.frames[a];auto const& y=incoming.frames[b];
                if(x.input.sequence<y.input.sequence)next=&x,++a;
                else if(y.input.sequence<x.input.sequence)next=&y,++b;
                else {if(!source::same_tracking(x,y))return false;next=&x;++a;++b;}
            }
            if(last&&(!source::motion_pair(*last,*next,0,100000000)||
                      !source::motion_pair(*last,*next,1,100000000)))return false;
            if(next->input.sequence>=first){
                if(out.count>=source::motion_history_capacity)return false;
                out.frames[out.count++]=*next;
            }
            last=next;
        }
        return out.count&&last&&source::same_tracking(out.frames[out.count-1],*last);
    }

public:
    void clear() noexcept {rows_={};publisher_={};bound_=false;}
    bool merge(Row const& current,Publisher publisher,History const& incoming,History& out) noexcept {
        out={};
        if(!valid_incoming(publisher,incoming,current)){clear();return false;}
        if(!bound_){
            unsigned first=incoming.count>source::motion_history_capacity?
                incoming.count-source::motion_history_capacity:0;
            rows_={};rows_.count=incoming.count-first;
            for(unsigned i=0;i<rows_.count;++i)rows_.frames[i]=incoming.frames[first+i];
            publisher_=publisher;bound_=true;out=rows_;return true;
        }
        if(publisher.pid!=publisher_.pid||publisher.start!=publisher_.start){
            clear();return false;
        }
        if(current.input.sequence<rows_.frames[rows_.count-1].input.sequence){
            clear();return false;
        }
        if(!compose(rows_,incoming,out)){clear();out={};return false;}
        rows_=out;return true;
    }
};
}
