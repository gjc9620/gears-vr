#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace gears_lancer_reload_display {
struct Layout {float x{},y{},scale{},width{},height{},resolution_scale{};};
inline bool layout(float clip_x,float clip_y,float texture_width,float texture_height,
    float resolution_scale,Layout& out,bool readable_marker=false) noexcept {
    if(!std::isfinite(clip_x)||!std::isfinite(clip_y)||clip_x<128||clip_x>16384||clip_y<128||clip_y>16384||
       !std::isfinite(texture_width)||texture_width<1||texture_width>4096||
       !std::isfinite(texture_height)||texture_height<1||texture_height>256||
       !std::isfinite(resolution_scale)||resolution_scale<=0||resolution_scale>16)return false;
    // Centre the original bar in the upper gameplay area. The private mirror
    //fits a narrower eye image inside the Canvas; the former12%-X coordinate
    //lies outside those fitted-eye bounds. Final projection is still checked
    //in the actual paired recording. A centred bar fits both regions.
    // Derive scale
    // from the real Canvas and original texture, not desktop/window pixels.
    // Retail's tick texture uses ResScaleSmall, without the bar's Scale.
    // Borrow a larger ResScaleSmall ONLY inside the owned bar invocation and
    // compensate Scale inversely. Original tile UVs, bar/region width, Canvas
    // scale, source image size and reward state stay unchanged. The caller
    // must journal/restore this HUD float before any unrelated drawing.
    Layout p{};p.resolution_scale=readable_marker?(std::min)(resolution_scale*2.f,16.f):resolution_scale;
    p.width=clip_x*.20f;p.scale=p.width/(texture_width*p.resolution_scale);
    // Retail uses the same Scale for width and height, making its245x7
    // background a hairline in the fitted stereo eye. Keep width independent
    // of the readable height; the original function's invocation-local
    // BarHeight is adapted AFTER its calculation, not the atlas UV rectangle.
    p.height=clip_y*.025f;p.x=(clip_x-p.width)*.5f;p.y=clip_y*.20f;
    if(!std::isfinite(p.scale)||p.scale<.01f||p.scale>64||p.height>clip_y*.12f||
       p.x+p.width>clip_x||p.y+p.height>clip_y)return false;
    out=p;return true;
}
// DrawActionReloadBar has its own automatic fade clock. A physical reload
// can outlive that clock. These values are scoped to that drawing invocation;
// neither the transaction/reward clock nor actual weapon readiness changes.
struct Plan {
    float draw_start{},marker{};
    std::array<float,4> window{};
    bool ordinary{};
};
inline bool plan(float now,float transaction_start,float duration,float fade,float flash,
    std::array<float,4> const& window,Plan& out) noexcept {
    if(!std::isfinite(now)||!std::isfinite(transaction_start)||transaction_start<0||now<transaction_start||
       !std::isfinite(duration)||duration<.1f||duration>60||!std::isfinite(fade)||fade<0||fade>60||
       !std::isfinite(flash)||flash<0||flash>60)return false;
    for(float v:window)if(!std::isfinite(v)||v<0||v>60)return false;
    if(window[0]+window[2]+window[3]>duration+.001f)return false;
    const float elapsed=now-transaction_start;
    Plan p{};p.window=window;p.marker=(std::min)(elapsed,duration);
    // Hold the display before its fade region while the actual transaction
    // remains open. The marker still represents real elapsed transaction time.
    // Retail takes the LOWER flash/fade alpha. FadeDuration alone leaves
    //half opacity when FlashDuration is longer (.5/1 in the current game).
    const float margin=(std::min)(duration,(std::max)((std::max)(fade,flash),.01f));
    const float shown=(std::min)(elapsed,duration-margin);
    p.draw_start=now-shown;
    const float perfect_end=window[0]+window[2];
    p.ordinary=window[2]<=0||elapsed>perfect_end;
    if(p.ordinary)p.window={{0,0,0,duration}};
    if(!std::isfinite(p.draw_start)||p.draw_start<0||p.draw_start>now)return false;
    out=p;return true;
}
// A confirmed original success may outlive the physical reload lease. Borrow
// only the exact HUD invocation's two presentation floats; never retain the
// weapon state, reward clock or ammo readiness. Retail takes the smaller
// flash/fade alpha, so both clocks must see this same completion-relative age.
inline bool result_plan(float now,float completed,float weapon_duration,float fade,float flash,Plan& out) noexcept {
    if(!std::isfinite(now)||!std::isfinite(completed)||completed<0||now<completed||
       !std::isfinite(weapon_duration)||weapon_duration<.1f||weapon_duration>60||
       !std::isfinite(fade)||fade<=0||fade>60||!std::isfinite(flash)||flash<=0||flash>60)return false;
    const float age=now-completed,remaining=flash-age;
    if(remaining<=0)return false;
    Plan p{};p.marker=(std::max)(0.f,weapon_duration-remaining);p.draw_start=now-p.marker;
    p.window={{0,0,weapon_duration,0}};
    if(!std::isfinite(p.draw_start)||p.draw_start<0)return false;
    out=p;return true;
}
}
