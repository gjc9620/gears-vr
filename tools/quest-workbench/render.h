#pragma once
#include "core.h"
#include <GLES3/gl3.h>
#include <map>
#include <functional>

namespace workbench {
struct M4 {float a[16]{};};
inline M4 identity(){M4 m{};m.a[0]=m.a[5]=m.a[10]=m.a[15]=1;return m;}
inline M4 multiply(M4 a,M4 b){M4 m{};for(int c=0;c<4;++c)for(int r=0;r<4;++r)for(int k=0;k<4;++k)m.a[c*4+r]+=a.a[k*4+r]*b.a[c*4+k];return m;}
inline M4 matrix(P p,double scale=1){M4 m=identity();auto r=v::rotation_matrix(p.orientation);for(int c=0;c<3;++c)for(int i=0;i<3;++i)m.a[c*4+i]=float(r[i][c]*scale);m.a[12]=float(p.position.x);m.a[13]=float(p.position.y);m.a[14]=float(p.position.z);return m;}
inline M4 projection(float left,float right,float down,float up,float near=.03f,float far=50){M4 m{};const float l=std::tan(left),r=std::tan(right),d=std::tan(down),u=std::tan(up);m.a[0]=2/(r-l);m.a[5]=2/(u-d);m.a[8]=(r+l)/(r-l);m.a[9]=(u+d)/(u-d);m.a[10]=-(far+near)/(far-near);m.a[11]=-1;m.a[14]=-2*far*near/(far-near);return m;}
struct Color {float r{},g{},b{},a{1};};
struct Vertex {float x{},y{},z{},nx{},ny{},nz{};};
struct Mesh {std::vector<Vertex> vertices;std::vector<std::uint32_t> indices;GLuint vao{},vbo{},ebo{};};
struct HandMesh {Rig rig{};std::vector<V> bind;std::vector<std::array<unsigned,4>> bones;std::vector<std::array<double,4>> weights;std::array<P,16> inverse_bind{};Mesh mesh;};
struct Glyph {float u{},v{},w{},h{},advance{};};
struct Button {std::string text;P pose{};double width{},height{};Color color;std::function<void()> action;std::function<void(double)> change;double value{-1},font_size{.023};};
class Renderer {
    struct Instance {M4 model;Color color;};
    struct TextVertex {float x,y,z,u,v;Color color;};
    GLuint program_{},text_program_{},text_vao_{},text_buffer_{},font_{};GLint mvp_{},model_{},color_{},text_mvp_{},text_color_{};
    GLuint instance_program_{},instance_buffer_{};GLint instance_vp_{};bool collecting_{true};
    std::vector<Instance> boxes_,balls_,lines_;std::vector<TextVertex> text_vertices_;
    void instances(Mesh const& mesh,std::vector<Instance> const& instances,M4 vp);
    Mesh cube_,sphere_,cylinder_;std::map<unsigned,Glyph> glyphs_;
public:
    std::function<std::vector<unsigned char>(std::string const&)> read;
    void initialize();void shutdown();void upload(Mesh& mesh);void update(Mesh& mesh);void destroy(Mesh& mesh);
    void begin_scene(){boxes_.clear();balls_.clear();lines_.clear();text_vertices_.clear();collecting_=true;}
    void replay_scene(){collecting_=false;}
    void flush(M4 vp);
    void normals(Mesh& mesh);
    Mesh load_glb(std::string const& name);
    std::array<HandMesh,2> load_hands();
    void skin(HandMesh& h,std::array<Q,15> const& local);
    void mesh(Mesh const& mesh,P pose,Color color,M4 vp,double scale=1);
    void box(P pose,V scale,Color color,M4 vp);
    void ball(V position,double radius,Color color,M4 vp);
    void line(V from,V to,double radius,Color color,M4 vp);
    void text(std::string const& text,P pose,double size,Color color,M4 vp);
    void button(Button const& b,M4 vp,bool hover);
    void hand(Hand const& h,Color color,M4 vp);
    void volume(Volume const& volume,P object,Color color,M4 vp);
};
}
