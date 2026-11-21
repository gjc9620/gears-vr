#include "original_input_cleanup_context_live.inl"
#include <cstdio>
int main(){
    namespace api=gears_weapon_original_input_cleanup_context;
    api::Request request{};
    if(original_input_cleanup_context_live::query(nullptr)!=unsigned(api::Result::unavailable)||
       original_input_cleanup_context_live::query(&request)!=unsigned(api::Result::unavailable))return 1;
    std::puts("Original input cleanup disabled path: unavailable without runtime/body declarations.");return 0;
}
