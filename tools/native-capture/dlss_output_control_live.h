#pragma once
// Included inside capture's anonymous namespace, after the pure header.
namespace dlss_output_control {
using namespace gears_dlss_output;
struct Output {
    Key key{};uintptr_t pass{},view{},renderer{},descriptor{};unsigned thread{},flags{},setters{},quads{},draws{},binds{};
    View before{};Descriptor expected{};bool valid{},site{},nested{},setter_proved{},quad_complete{},draw_complete{},failed{};
    bool binding_proved{},binding_failed{};gears_dlss_route::InputBind binding_capture{};
    Eye::BindingEvidence binding{};
    Eye proof{};
};
struct ListPath {uintptr_t owner{},raw{},table{},method{},canonical{};uint64_t generation{};bool valid{};};
struct BindingTrace {
    gears_dlss_route::InputSample sample{};ListPath before{},after{};uintptr_t vtable{};
    bool vtable_read{},chain{},named{},same{};
};
struct OutputExitReceipt {
    Key key{};uintptr_t pass{},view{},renderer{},descriptor{};unsigned thread{},flags{},fresh_flags{},setters{},quads{},draws{},binds{};
    bool valid{},failed{},nested{},exception{},setter_proved{},quad_complete{},draw_candidate{},draw_complete{};
    bool current{},thread_same{},flags_read{},stable{},descriptor_read{},descriptor_stable{},binding_snapshot{},binding_equal{},binding_current{},binding_proved{},binding_failed{};
    bool tail_transition_validated{},output_admitted{};
    View before{},fresh_view{};Descriptor expected{},fresh_descriptor{};
    Eye::BindingEvidence binding{};gears_dlss_route::InputSample input_before{};BindingTrace trace{},tail_trace{};
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
struct BindScope {
    Output* output{};bool tracking{};uintptr_t vtable{};int exceptions{};
    BindScope(bool world,uintptr_t caller,const uint16_t* descriptor,void* view,void* owner,uint32_t selection,int32_t alternate);~BindScope();
    BindScope(BindScope const&)=delete;BindScope& operator=(BindScope const&)=delete;
};
struct QuadScope {
    Output* output{};QuadScope* previous{};Args forwarded{};Dimensions dimensions{};bool changed{};int exceptions{};
    QuadScope(bool world,uintptr_t caller,Args const& args,Dimensions const& sizes);~QuadScope();
    bool candidate()const;
    void forward(float& x,float& y,float& w,float& h,float& u,float& v,float& uw,float& vh)const;
    QuadScope(QuadScope const&)=delete;QuadScope& operator=(QuadScope const&)=delete;
};
struct DrawCompletion {
    Output* output{};uintptr_t list{};int exceptions{};
    explicit DrawCompletion(uintptr_t raw);~DrawCompletion();
    DrawCompletion(DrawCompletion const&)=delete;DrawCompletion& operator=(DrawCompletion const&)=delete;
};
bool enable(FILE*,bool observer_attested);
bool scoped();
bool publication(uint64_t world,uint64_t ticket);
void retire();void progress(FILE*);void print(FILE*);
gears_final_transfer::EyeDrawEvidence draw(uintptr_t list,uintptr_t caller,unsigned kind,unsigned elements,unsigned instances,
    gears_viewport::TargetDraw const& binding,gears_frame_link::ListRef ref,gears_viewport::DrawState const& rectangles);
}
