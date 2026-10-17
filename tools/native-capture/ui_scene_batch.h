#pragma once
#include "ui_canvas_batch.h"
#include "ui_material_batch.h"
#include <vector>
#include <algorithm>
#include <new>
#include <utility>

// Bounded read-only producer evidence. Equal captures are not an atomic engine
// lock, allocation/ABA identity, flush-before-return proof or GPU ownership.
namespace gears_ui_scene_batch {
using Config=gears_ui_batch::Config;
using Vertex=gears_ui_batch::Vertex;
namespace d=gears_ui_batch::detail;
inline constexpr unsigned key_limit=64,item_limit=128,vertex_limit=1024;
enum class Kind {simple,material};
struct Item {
    std::uintptr_t address{},payload{};Kind kind=Kind::simple;
    d::Array vertex_array{};
    std::array<unsigned char,80> transform{};
    std::vector<Vertex> vertices;
    gears_ui_material::Receipt material{};
    std::array<unsigned char,112> material_header{};
};
struct Key {std::int32_t value{};d::Array item_array{};std::vector<Item> items;};
struct Inventory {
    std::uintptr_t fcanvas{};d::Array stack_array{},element_array{};
    std::vector<std::int32_t> stack;
    std::vector<Key> keys;
};
struct Range {
    std::int32_t key{};std::uintptr_t item{},payload{};
    unsigned begin{},end{};std::array<unsigned char,80> transform{};
    std::vector<Vertex> vertices;
};
struct Delta {
    std::uintptr_t fcanvas{};std::vector<Range> simple;
    std::vector<gears_ui_material::Receipt> material;
};
namespace detail {
inline bool same_material(Item const& a,Item const& b){return a.material_header==b.material_header&&
    a.material.proxy==b.material.proxy&&a.material.transform==b.material.transform&&a.material.record==b.material.record;}
inline bool same(Inventory const& a,Inventory const& b){
    if(a.fcanvas!=b.fcanvas||!d::same(a.stack_array,b.stack_array)||!d::same(a.element_array,b.element_array)||
       a.stack!=b.stack||a.keys.size()!=b.keys.size())return false;
    for(std::size_t k=0;k<a.keys.size();++k){auto const& x=a.keys[k];auto const& y=b.keys[k];
        if(x.value!=y.value||!d::same(x.item_array,y.item_array)||x.items.size()!=y.items.size())return false;
        for(std::size_t i=0;i<x.items.size();++i){auto const& p=x.items[i];auto const& q=y.items[i];
            if(p.address!=q.address||p.payload!=q.payload||p.kind!=q.kind)return false;
            if(p.kind==Kind::simple){if(!d::same(p.vertex_array,q.vertex_array)||p.transform!=q.transform||p.vertices!=q.vertices)return false;}
            else if(!same_material(p,q))return false;
        }
    }return true;
}
inline bool bounded(Inventory const& value){
    if(!value.fcanvas||value.stack.empty()||value.stack.size()>key_limit||value.keys.size()>key_limit||
       !d::valid(value.stack_array,key_limit,4)||std::size_t(value.stack_array.count)!=value.stack.size()||
       !d::valid(value.element_array,key_limit,20)||std::size_t(value.element_array.count)!=value.keys.size())return false;
    std::vector<std::uintptr_t> items,payloads;unsigned vertices=0;
    for(std::size_t k=0;k<value.keys.size();++k){auto const& key=value.keys[k];
        for(std::size_t j=0;j<k;++j)if(value.keys[j].value==key.value)return false;
        if(key.items.size()>item_limit-items.size()||!d::valid(key.item_array,item_limit,8)||
           std::size_t(key.item_array.count)!=key.items.size())return false;
        for(auto const& item:key.items){
            if(!item.address||!item.payload||std::find(items.begin(),items.end(),item.address)!=items.end()||
               std::find(payloads.begin(),payloads.end(),item.payload)!=payloads.end())return false;
            items.push_back(item.address);payloads.push_back(item.payload);
            if(item.kind==Kind::simple){if(item.vertices.size()>vertex_limit-vertices||item.vertices.size()%4||
                   !d::valid(item.vertex_array,1048576,gears_ui_batch::vertex_stride)||
                   std::size_t(item.vertex_array.count)!=item.vertices.size())return false;
                vertices+=unsigned(item.vertices.size());}
            else if(item.kind==Kind::material){auto const& m=item.material;d::Array records{};std::uintptr_t proxy{};
                std::array<unsigned char,80> transform{};
                std::memcpy(&records,item.material_header.data()+0x60,16);
                std::memcpy(&proxy,item.material_header.data(),8);
                std::memcpy(transform.data(),item.material_header.data()+0x10,80);
                if(m.canvas||m.fcanvas!=value.fcanvas||m.key!=key.value||m.item!=item.address||m.payload!=item.payload||
                   m.kind||!proxy||m.proxy!=proxy||m.transform!=transform||!item.vertices.empty()||
                   !d::valid(records,128,36)||records.count!=1)return false;
                for(unsigned j=0;j<8;++j){float f{};std::memcpy(&f,m.record.data()+j*4,4);if(!std::isfinite(f))return false;}
            }else return false;
        }
    }return true;
}
template<class Read> bool capture(Config c,Read read,std::uintptr_t fcanvas,Inventory& out){
    d::Reader<Read> r{read};Inventory result{};result.fcanvas=fcanvas;
    if(!fcanvas||!gears_ui_batch::validate_binding(c,read)||!gears_ui_material::validate_runtime_binding({c.base},read)||
       !r.field(result.stack_array,fcanvas,0x28)||!d::valid(result.stack_array,key_limit,4)||!result.stack_array.count||
       !r.field(result.element_array,fcanvas,0x48)||!d::valid(result.element_array,key_limit,20))return false;
    result.stack.resize(result.stack_array.count);
    if(!r.bytes(result.stack.data(),result.stack_array.data,result.stack.size()*4))return false;
    unsigned item_count=0,vertex_count=0;
    for(int k=0;k<result.element_array.count;++k){Key key{};std::uintptr_t at{};
        if(!d::add(result.element_array.data,std::size_t(k)*20,at)||!r.field(key.value,at,0)||
           !r.field(key.item_array,at,4)||!d::valid(key.item_array,item_limit,8)||
           unsigned(key.item_array.count)>item_limit-item_count)return false;
        for(auto const& old:result.keys)if(old.value==key.value)return false;
        for(int i=0;i<key.item_array.count;++i){Item item{};std::uintptr_t table{};
            if(!r.field(item.address,key.item_array.data,std::size_t(i)*8)||!item.address||
               !r.field(table,item.address,0)||!r.field(item.payload,item.address,8)||!item.payload)return false;
            for(auto const& old_key:result.keys)for(auto const& old:old_key.items)
                if(old.address==item.address||old.payload==item.payload)return false;
            for(auto const& old:key.items)if(old.address==item.address||old.payload==item.payload)return false;
            if(table==c.base+0x133cc98){
                if(!r.field(item.vertex_array,item.payload,0x48)||!d::valid(item.vertex_array,1048576,gears_ui_batch::vertex_stride)||
                   unsigned(item.vertex_array.count)>vertex_limit-vertex_count||item.vertex_array.count%4||
                   !r.field(item.transform,item.payload,0x80))return false;
                item.vertices.resize(item.vertex_array.count);
                for(std::size_t v=0;v<item.vertices.size();++v){std::uintptr_t address{};
                    if(!d::add(item.vertex_array.data,v*gears_ui_batch::vertex_stride,address)||
                       !r.bytes(item.vertices[v].data(),address,item.vertices[v].size()))return false;}
                vertex_count+=unsigned(item.vertices.size());
            }else if(table==c.base+0x136c5f0){unsigned flags{};item.kind=Kind::material;
                item.material.fcanvas=fcanvas;item.material.item=item.address;item.material.payload=item.payload;item.material.key=key.value;
                if(!r.field(flags,item.address,0x10)||flags||!gears_ui_material::read_payload(read,item.payload,item.material)||
                   !r.bytes(item.material_header.data(),item.payload,item.material_header.size()))return false;
            }else return false;
            key.items.push_back(std::move(item));++item_count;
        }result.keys.push_back(std::move(key));
    }
    out=std::move(result);return true;
}
}
template<class Read> bool inventory(Config c,Read read,std::uintptr_t fcanvas,Inventory& out){
    try{Inventory first{},second{};
        if(!detail::capture(c,read,fcanvas,first)||!detail::capture(c,read,fcanvas,second)||
           !detail::same(first,second)||!detail::bounded(second))return false;
        out=std::move(second);return true;
    }catch(std::bad_alloc const&){return false;}
}
template<class Read> bool delta(Config c,Read read,Inventory const& before,std::uintptr_t fcanvas,Delta& out){
    try{Inventory after{};
        if(before.fcanvas!=fcanvas||!detail::bounded(before)||!inventory(c,read,fcanvas,after)||before.stack!=after.stack||
           !d::same(before.stack_array,after.stack_array))return false;
        for(auto const& key:before.keys){auto found=std::find_if(after.keys.begin(),after.keys.end(),[&](Key const& k){return k.value==key.value;});
            if(found==after.keys.end()||found->items.size()<key.items.size())return false;
            for(std::size_t i=0;i<key.items.size();++i){auto const& old=key.items[i];auto const& now=found->items[i];
                if(old.address!=now.address||old.payload!=now.payload||old.kind!=now.kind)return false;
                if(old.kind==Kind::material){if(!detail::same_material(old,now))return false;}
                else if(old.transform!=now.transform||old.vertices.size()>now.vertices.size()||
                        !std::equal(old.vertices.begin(),old.vertices.end(),now.vertices.begin()))return false;
            }
        }
        Delta result{};result.fcanvas=fcanvas;
        for(auto const& key:after.keys){auto old=std::find_if(before.keys.begin(),before.keys.end(),[&](Key const& k){return k.value==key.value;});
            for(std::size_t i=0;i<key.items.size();++i){auto const& item=key.items[i];
                const bool existing=old!=before.keys.end()&&i<old->items.size();
                if(item.kind==Kind::material){if(!existing)result.material.push_back(item.material);}
                else {const auto begin=existing?old->items[i].vertices.size():0;
                    if(begin==item.vertices.size())continue;
                    Range range{key.value,item.address,item.payload,unsigned(begin),unsigned(item.vertices.size()),item.transform,{}};
                    range.vertices.assign(item.vertices.begin()+begin,item.vertices.end());result.simple.push_back(std::move(range));}
            }
        }out=std::move(result);return true;
    }catch(std::bad_alloc const&){return false;}
}
}
