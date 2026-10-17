#pragma once
// Included inside capture's anonymous namespace, after the pure header.
namespace dlss_output_control {
using namespace gears_dlss_output;
struct Output {
    Key key{};uintptr_t pass{},view{},renderer{},descriptor{};unsigned thread{},flags{},setters{},quads{},draws{};
    View before{};Descriptor expected{};bool valid{},site{},nested{},setter_proved{},quad_complete{},failed{};
    Eye proof{};
};
struct ConsumeScope {
    Key key{},previous{};int exceptions{};
    ConsumeScope(uint64_t world,uint64_t ticket,bool enabled);~ConsumeScope();
    ConsumeScope(ConsumeScope const&)=delete;ConsumeScope& operator=(ConsumeScope const&)=delete;
};
struct OutputScope {
    Output value;Output* previous{};int exceptions{};
    OutputScope(bool world,uintptr_t pass,uintptr_t view);~OutputScope();
    OutputScope(OutputScope const&)=delete;OutputScope& operator=(OutputScope const&)=delete;
};
struct SetterScope {
    Output* output{};bool tracking{};int exceptions{};
    SetterScope(bool world,uintptr_t caller,const uint16_t* descriptor,unsigned index);~SetterScope();
    SetterScope(SetterScope const&)=delete;SetterScope& operator=(SetterScope const&)=delete;
};
struct QuadScope {
    Output* output{};QuadScope* previous{};Args forwarded{};Dimensions dimensions{};bool changed{};int exceptions{};
    QuadScope(bool world,uintptr_t caller,Args const& args,Dimensions const& sizes);~QuadScope();
    bool candidate()const;
    void forward(float& x,float& y,float& w,float& h,float& u,float& v,float& uw,float& vh)const;
    QuadScope(QuadScope const&)=delete;QuadScope& operator=(QuadScope const&)=delete;
};
bool enable(FILE*,bool observer_attested);
bool scoped();
bool publication(uint64_t world,uint64_t ticket);
void retire();void progress(FILE*);void print(FILE*);
gears_final_transfer::EyeDrawEvidence draw(uintptr_t list,uintptr_t caller,unsigned kind,unsigned elements,unsigned instances,
    gears_viewport::TargetDraw const& binding,gears_frame_link::ListRef ref,gears_viewport::DrawState const& rectangles);
}
