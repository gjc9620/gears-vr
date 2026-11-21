#include "render.h"
#define CGLTF_IMPLEMENTATION
#include "third_party/cgltf.h"
#include <stdexcept>
#include <cstring>
namespace workbench {
namespace {
GLuint shader(GLenum kind,char const* source){GLuint s=glCreateShader(kind);glShaderSource(s,1,&source,nullptr);glCompileShader(s);GLint ok{};glGetShaderiv(s,GL_COMPILE_STATUS,&ok);if(!ok){char message[2048]{};glGetShaderInfoLog(s,sizeof message,nullptr,message);throw std::runtime_error(message);}return s;}
GLuint program(char const* vertex,char const* fragment){auto a=shader(GL_VERTEX_SHADER,vertex),b=shader(GL_FRAGMENT_SHADER,fragment);auto p=glCreateProgram();glAttachShader(p,a);glAttachShader(p,b);glLinkProgram(p);glDeleteShader(a);glDeleteShader(b);GLint ok{};glGetProgramiv(p,GL_LINK_STATUS,&ok);if(!ok)throw std::runtime_error("GL program link failed");return p;}
void component(V& v,unsigned axis,double value){if(axis==0)v.x=value;else if(axis==1)v.y=value;else v.z=value;}
unsigned utf8(std::string const& s,std::size_t& i){unsigned a=static_cast<unsigned char>(s[i++]);if(a<128)return a;unsigned n=a<224?1:a<240?2:3;unsigned r=a&((1u<<(6-n))-1);while(n--&&i<s.size())r=(r<<6)|(static_cast<unsigned char>(s[i++])&63);return r;}
}
void Renderer::initialize(){
    program_=program(R"(#version 300 es
layout(location=0) in vec3 position;layout(location=1) in vec3 normal;uniform mat4 mvp;uniform mat4 model;out vec3 n;out vec3 world;void main(){gl_Position=mvp*vec4(position,1);world=(model*vec4(position,1)).xyz;n=mat3(model)*normal;})",
R"(#version 300 es
precision highp float;in vec3 n;in vec3 world;uniform vec4 color;out vec4 result;void main(){vec3 normal=normalize(n);float light=.34+.49*abs(dot(normal,normalize(vec3(-.3,.8,.5))))+.17*abs(dot(normal,normalize(vec3(.7,.1,-.5))));result=vec4(color.rgb*light,color.a);})");
    mvp_=glGetUniformLocation(program_,"mvp");model_=glGetUniformLocation(program_,"model");color_=glGetUniformLocation(program_,"color");
    instance_program_=program(R"(#version 300 es
layout(location=0) in vec3 position;layout(location=1) in vec3 normal;
layout(location=2) in mat4 model;layout(location=6) in vec4 color;
uniform mat4 vp;out vec3 n;out vec4 tint;
void main(){gl_Position=vp*model*vec4(position,1);n=mat3(model)*normal;tint=color;})",R"(#version 300 es
precision highp float;in vec3 n;in vec4 tint;out vec4 result;
void main(){float light=.45+.55*abs(dot(normalize(n),normalize(vec3(-.3,.8,.5))));result=vec4(tint.rgb*light,tint.a);})");
    instance_vp_=glGetUniformLocation(instance_program_,"vp");glGenBuffers(1,&instance_buffer_);
    text_program_=program(R"(#version 300 es
layout(location=0) in vec3 position;layout(location=1) in vec2 tex;layout(location=2) in vec4 color;uniform mat4 mvp;out vec2 uv;out vec4 tint;void main(){gl_Position=mvp*vec4(position,1);uv=tex;tint=color;})",R"(#version 300 es
precision highp float;in vec2 uv;in vec4 tint;uniform sampler2D atlas;out vec4 result;void main(){float a=texture(atlas,uv).r;if(a<.03)discard;result=vec4(tint.rgb,tint.a*a);})");
    text_mvp_=glGetUniformLocation(text_program_,"mvp");text_color_=glGetUniformLocation(text_program_,"color");
    glGenVertexArrays(1,&text_vao_);glGenBuffers(1,&text_buffer_);glBindVertexArray(text_vao_);glBindBuffer(GL_ARRAY_BUFFER,text_buffer_);glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(TextVertex),nullptr);glEnableVertexAttribArray(1);glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(TextVertex),reinterpret_cast<void*>(12));glEnableVertexAttribArray(2);glVertexAttribPointer(2,4,GL_FLOAT,GL_FALSE,sizeof(TextVertex),reinterpret_cast<void*>(20));glBindVertexArray(0);
    auto bytes=read("font.json");auto font=Json::parse(bytes);auto raw=read("font.raw");unsigned w=font["width"],h=font["height"];if(raw.size()!=w*h)throw std::runtime_error("Invalid glyph atlas");
    for(auto it=font["glyphs"].begin();it!=font["glyphs"].end();++it){auto a=it.value();glyphs_[unsigned(std::stoul(it.key()))]={a[0],a[1],a[2],a[3],a[4]};}
    glGenTextures(1,&font_);glBindTexture(GL_TEXTURE_2D,font_);glTexImage2D(GL_TEXTURE_2D,0,GL_R8,w,h,0,GL_RED,GL_UNSIGNED_BYTE,raw.data());glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    for(auto p:{V{-1,-1,-1},V{1,-1,-1},V{1,1,-1},V{-1,1,-1},V{-1,-1,1},V{1,-1,1},V{1,1,1},V{-1,1,1}})cube_.vertices.push_back({float(p.x*.5),float(p.y*.5),float(p.z*.5)});
    cube_.indices={0,2,1,0,3,2,4,5,6,4,6,7,0,1,5,0,5,4,3,7,6,3,6,2,1,2,6,1,6,5,0,4,7,0,7,3};normals(cube_);upload(cube_);
    constexpr unsigned rows=8,cols=12;for(unsigned y=0;y<=rows;++y)for(unsigned x=0;x<=cols;++x){double p=y*3.141592653589793/rows,t=x*6.283185307179586/cols;float a=float(std::sin(p)*std::cos(t)),b=float(std::cos(p)),c=float(std::sin(p)*std::sin(t));sphere_.vertices.push_back({a,b,c,a,b,c});}
    for(unsigned y=0;y<rows;++y)for(unsigned x=0;x<cols;++x){auto a=y*(cols+1)+x,b=a+cols+1;for(auto n:{a,b,a+1,a+1,b,b+1})sphere_.indices.push_back(n);}upload(sphere_);
    for(unsigned y=0;y<2;++y)for(unsigned i=0;i<12;++i){float a=float(i*6.283185307179586/12);cylinder_.vertices.push_back({std::cos(a),float(y),std::sin(a),std::cos(a),0,std::sin(a)});}for(unsigned i=0;i<12;++i){unsigned n=(i+1)%12;for(auto j:{i,n,i+12,n,n+12,i+12})cylinder_.indices.push_back(j);}upload(cylinder_);
    glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);glDisable(GL_CULL_FACE);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
}
void Renderer::destroy(Mesh& m){if(m.ebo)glDeleteBuffers(1,&m.ebo);if(m.vbo)glDeleteBuffers(1,&m.vbo);if(m.vao)glDeleteVertexArrays(1,&m.vao);m.ebo=m.vbo=m.vao=0;}
void Renderer::shutdown(){destroy(cube_);destroy(sphere_);destroy(cylinder_);glDeleteProgram(program_);glDeleteProgram(instance_program_);glDeleteBuffers(1,&instance_buffer_);glDeleteProgram(text_program_);glDeleteTextures(1,&font_);glDeleteVertexArrays(1,&text_vao_);glDeleteBuffers(1,&text_buffer_);}
void Renderer::upload(Mesh& m){glGenVertexArrays(1,&m.vao);glGenBuffers(1,&m.vbo);glGenBuffers(1,&m.ebo);glBindVertexArray(m.vao);glBindBuffer(GL_ARRAY_BUFFER,m.vbo);glBufferData(GL_ARRAY_BUFFER,m.vertices.size()*sizeof(Vertex),m.vertices.data(),GL_DYNAMIC_DRAW);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,m.ebo);glBufferData(GL_ELEMENT_ARRAY_BUFFER,m.indices.size()*4,m.indices.data(),GL_STATIC_DRAW);glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);glEnableVertexAttribArray(1);glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(12));glBindVertexArray(0);}
void Renderer::update(Mesh& m){glBindBuffer(GL_ARRAY_BUFFER,m.vbo);glBufferSubData(GL_ARRAY_BUFFER,0,m.vertices.size()*sizeof(Vertex),m.vertices.data());}
void Renderer::normals(Mesh& m){for(auto& a:m.vertices)a.nx=a.ny=a.nz=0;for(std::size_t i=0;i+2<m.indices.size();i+=3){auto& a=m.vertices.at(m.indices[i]);auto& b=m.vertices.at(m.indices[i+1]);auto& c=m.vertices.at(m.indices[i+2]);V n=cross({b.x-a.x,b.y-a.y,b.z-a.z},{c.x-a.x,c.y-a.y,c.z-a.z});for(auto j:{m.indices[i],m.indices[i+1],m.indices[i+2]}){auto& p=m.vertices[j];p.nx+=float(n.x);p.ny+=float(n.y);p.nz+=float(n.z);}}for(auto& a:m.vertices){V n=unit({a.nx,a.ny,a.nz});a.nx=float(n.x);a.ny=float(n.y);a.nz=float(n.z);}}
Mesh Renderer::load_glb(std::string const& name){
    auto raw=read(name);if(raw.size()>64*1024*1024)throw std::runtime_error("Model exceeds64MiB");cgltf_options options{};cgltf_data* data{};if(cgltf_parse(&options,raw.data(),raw.size(),&data)!=cgltf_result_success)throw std::runtime_error("Invalid GLB");
    struct Guard{cgltf_data* p;~Guard(){cgltf_free(p);}}guard{data};
    // Self-contained GLB only: never resolve arbitrary external buffer paths/URLs.
    if(data->buffers_count!=1||!data->bin||data->buffers[0].uri)throw std::runtime_error("Use a self-contained GLB");
    data->buffers[0].data=const_cast<void*>(data->bin);if(cgltf_validate(data)!=cgltf_result_success)throw std::runtime_error("Invalid GLB accessors");Mesh mesh;
    for(cgltf_size ni=0;ni<data->nodes_count;++ni){auto const& node=data->nodes[ni];if(!node.mesh)continue;float transform[16];cgltf_node_transform_world(&node,transform);
        for(cgltf_size pi=0;pi<node.mesh->primitives_count;++pi){auto const& p=node.mesh->primitives[pi];if(p.type!=cgltf_primitive_type_triangles)continue;auto* positions=cgltf_find_accessor(&p,cgltf_attribute_type_position,0);if(!positions)continue;if(positions->count>500000||mesh.vertices.size()+positions->count>500000)throw std::runtime_error("Too many vertices");auto base=std::uint32_t(mesh.vertices.size());
            for(cgltf_size i=0;i<positions->count;++i){float q[3]{};if(!cgltf_accessor_read_float(positions,i,q,3))throw std::runtime_error("Invalid position");Vertex a{};a.x=transform[0]*q[0]+transform[4]*q[1]+transform[8]*q[2]+transform[12];a.y=transform[1]*q[0]+transform[5]*q[1]+transform[9]*q[2]+transform[13];a.z=transform[2]*q[0]+transform[6]*q[1]+transform[10]*q[2]+transform[14];if(!std::isfinite(a.x)||!std::isfinite(a.y)||!std::isfinite(a.z)||std::max({std::abs(a.x),std::abs(a.y),std::abs(a.z)})>20)throw std::runtime_error("Use metre-scale model");mesh.vertices.push_back(a);}
            const auto count=p.indices?p.indices->count:positions->count;if(count>1500000)throw std::runtime_error("Too many triangles");for(cgltf_size i=0;i<count;++i){auto index=p.indices?cgltf_accessor_read_index(p.indices,i):i;if(index>=positions->count)throw std::runtime_error("Index out of bounds");mesh.indices.push_back(base+std::uint32_t(index));}
        }
    }if(mesh.vertices.empty()||mesh.indices.empty())throw std::runtime_error("No triangle mesh");normals(mesh);upload(mesh);return mesh;
}
std::array<HandMesh,2> Renderer::load_hands(){auto bytes=read("marcus.json");auto j=Json::parse(bytes);std::array<HandMesh,2> result;
    for(unsigned side=0;side<2;++side){auto& h=result[side];auto const& a=j.at("hands").at(side);for(unsigned i=0;i<16;++i){auto const& r=a.at("reference").at(i);h.rig.reference[i]={vector(r.at("position")),quaternion(r.at("orientation"))};h.rig.parent[i]=r.at("parent");}
        std::array<Q,15> refs{};for(unsigned i=0;i<15;++i)refs[i]=h.rig.reference[i+1].orientation;auto bind=fk(h.rig,refs);for(unsigned i=0;i<16;++i)h.inverse_bind[i]=inverse(bind[i]);
        for(auto const& p:a.at("vertices"))h.bind.push_back(vector(p));for(auto const& p:a.at("bones"))h.bones.push_back(p.get<std::array<unsigned,4>>());for(auto const& p:a.at("weights"))h.weights.push_back(p.get<std::array<double,4>>());for(auto const& t:a.at("triangles"))for(auto i:t)h.mesh.indices.push_back(i.get<std::uint32_t>());
        if(h.bind.size()!=h.bones.size()||h.bind.size()!=h.weights.size())throw std::runtime_error("Invalid skin");h.mesh.vertices.resize(h.bind.size());for(std::size_t i=0;i<h.bind.size();++i){auto& p=h.mesh.vertices[i];p.x=float(h.bind[i].x);p.y=float(h.bind[i].y);p.z=float(h.bind[i].z);}normals(h.mesh);upload(h.mesh);
    }return result;
}
void Renderer::skin(HandMesh& h,std::array<Q,15> const& local){auto world=fk(h.rig,local);std::array<P,16> delta;for(unsigned i=0;i<16;++i)delta[i]=compose(world[i],h.inverse_bind[i]);for(std::size_t i=0;i<h.bind.size();++i){V p{};for(unsigned k=0;k<4;++k)if(h.weights[i][k]>0){if(h.bones[i][k]>=16)throw std::runtime_error("Invalid skin bone");p=add(p,mul(compose(delta[h.bones[i][k]],{h.bind[i],{}}).position,h.weights[i][k]));}auto& a=h.mesh.vertices[i];a.x=float(p.x);a.y=float(p.y);a.z=float(p.z);}normals(h.mesh);update(h.mesh);}
void Renderer::mesh(Mesh const& m,P p,Color color,M4 vp,double scale){auto model=matrix(p,scale);auto mvp=multiply(vp,model);glUseProgram(program_);glUniformMatrix4fv(mvp_,1,GL_FALSE,mvp.a);glUniformMatrix4fv(model_,1,GL_FALSE,model.a);glUniform4f(color_,color.r,color.g,color.b,color.a);glBindVertexArray(m.vao);glDrawElements(GL_TRIANGLES,GLsizei(m.indices.size()),GL_UNSIGNED_INT,nullptr);glBindVertexArray(0);}
void Renderer::box(P p,V scale,Color color,M4){if(!collecting_)return;auto model=matrix(p);for(unsigned i=0;i<3;++i){model.a[i]*=float(scale.x);model.a[4+i]*=float(scale.y);model.a[8+i]*=float(scale.z);}boxes_.push_back({model,color});}
void Renderer::ball(V p,double radius,Color color,M4){if(collecting_)balls_.push_back({matrix({p,{}},radius),color});}
void Renderer::line(V a,V b,double radius,Color color,M4){if(!collecting_)return;const auto d=sub(b,a);if(length(d)<1e-6)return;auto model=matrix({a,arc({0,1,0},d)});for(unsigned i=0;i<3;++i){model.a[i]*=float(radius);model.a[4+i]*=float(length(d));model.a[8+i]*=float(radius);}lines_.push_back({model,color});}
void Renderer::text(std::string const& text,P pose,double size,Color color,M4){if(!collecting_)return;float x=0;for(std::size_t i=0;i<text.size();){unsigned c=utf8(text,i);auto it=glyphs_.find(c);if(it==glyphs_.end())it=glyphs_.find('?');if(it==glyphs_.end())continue;auto g=it->second;float s=float(size),l=x,r=x+s,t=0,b=-s;float u=g.u,v=g.v,w=g.w,h=g.h;for(auto a:{std::array<float,5>{l,b,0,u,v+h},{r,b,0,u+w,v+h},{r,t,0,u+w,v},{l,b,0,u,v+h},{r,t,0,u+w,v},{l,t,0,u,v}}){auto p=compose(pose,{{a[0],a[1],a[2]},{}}).position;text_vertices_.push_back({float(p.x),float(p.y),float(p.z),a[3],a[4],color});}x+=s*std::max(g.advance,.24f);}}
void Renderer::instances(Mesh const& m,std::vector<Instance> const& instances,M4 vp){
    if(instances.empty())return;glUseProgram(instance_program_);glUniformMatrix4fv(instance_vp_,1,GL_FALSE,vp.a);glBindVertexArray(m.vao);glBindBuffer(GL_ARRAY_BUFFER,instance_buffer_);glBufferData(GL_ARRAY_BUFFER,instances.size()*sizeof(Instance),instances.data(),GL_STREAM_DRAW);
    for(unsigned i=0;i<4;++i){glEnableVertexAttribArray(2+i);glVertexAttribPointer(2+i,4,GL_FLOAT,GL_FALSE,sizeof(Instance),reinterpret_cast<void*>(i*16));glVertexAttribDivisor(2+i,1);}glEnableVertexAttribArray(6);glVertexAttribPointer(6,4,GL_FLOAT,GL_FALSE,sizeof(Instance),reinterpret_cast<void*>(64));glVertexAttribDivisor(6,1);
    glDrawElementsInstanced(GL_TRIANGLES,GLsizei(m.indices.size()),GL_UNSIGNED_INT,nullptr,GLsizei(instances.size()));glBindVertexArray(0);
}
void Renderer::flush(M4 vp){
    instances(cube_,boxes_,vp);instances(sphere_,balls_,vp);instances(cylinder_,lines_,vp);
    if(!text_vertices_.empty()){glUseProgram(text_program_);glUniformMatrix4fv(text_mvp_,1,GL_FALSE,vp.a);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,font_);glBindVertexArray(text_vao_);glBindBuffer(GL_ARRAY_BUFFER,text_buffer_);glBufferData(GL_ARRAY_BUFFER,text_vertices_.size()*sizeof(TextVertex),text_vertices_.data(),GL_STREAM_DRAW);glDrawArrays(GL_TRIANGLES,0,GLsizei(text_vertices_.size()));glBindVertexArray(0);}
}
void Renderer::button(Button const& b,M4 vp,bool hover){if(!collecting_)return;auto c=b.color;if(hover){c.r=std::min(1.f,c.r+.2f);c.g=std::min(1.f,c.g+.2f);c.b=std::min(1.f,c.b+.2f);}box(b.pose,{b.width,b.height,.008},c,vp);auto textpose=compose(b.pose,{{-b.width*.45,b.height*.28,.006},{}});if(b.font_size>0)text(b.text,textpose,b.font_size,{.96,.98,1,1},vp);if(b.value>=0){auto from=compose(b.pose,{{-b.width*.43,-b.height*.25,.01},{}}).position,to=compose(b.pose,{{b.width*.43,-b.height*.25,.01},{}}).position;line(from,to,.0015,{.4f,.55f,.65f,1},vp);auto knob=compose(b.pose,{{(std::clamp(b.value,0.,1.)-.5)*b.width*.86,-b.height*.25,.012},{}}).position;ball(knob,.0055,hover?Color{1,.85f,.3f,1}:Color{.65f,.88f,1,1},vp);}}
void Renderer::hand(Hand const& h,Color color,M4 vp){if(!collecting_||!valid(h))return;static constexpr int parent[26]={1,-1,1,2,3,4,1,6,7,8,9,1,11,12,13,14,1,16,17,18,19,1,21,22,23,24};for(unsigned i=0;i<26;++i){auto const& j=h.joints[i];auto c=(j.flags&12)==12?color:Color{1,.55f,.15f,.7f};ball(j.pose.position,std::max(.003,std::min(j.radius,.012))*.65,c,vp);if(parent[i]>=0)line(h.joints[parent[i]].pose.position,j.pose.position,.0025,c,vp);}}
void Renderer::volume(Volume const& a,P object,Color color,M4 vp){if(!collecting_)return;P p=compose(object,a.local);if(a.shape=="box"){V h=a.half;for(unsigned axis=0;axis<3;++axis)for(int s:{-1,1})for(int t:{-1,1}){V x{},y{};const double e[3]={h.x,h.y,h.z};component(x,axis,-e[axis]);component(y,axis,e[axis]);component(x,(axis+1)%3,s*e[(axis+1)%3]);component(y,(axis+1)%3,s*e[(axis+1)%3]);component(x,(axis+2)%3,t*e[(axis+2)%3]);component(y,(axis+2)%3,t*e[(axis+2)%3]);line(compose(p,{x,{}}).position,compose(p,{y,{}}).position,.0013,color,vp);}return;}
    const auto point=[&](V v){return compose(p,{v,{}}).position;};for(unsigned plane=0;plane<3;++plane)for(unsigned i=0;i<48;++i){double a0=i*6.283185307179586/48,a1=(i+1)*6.283185307179586/48;V u{},w{};component(u,plane,std::cos(a0)*a.radius);component(u,(plane+1)%3,std::sin(a0)*a.radius);component(w,plane,std::cos(a1)*a.radius);component(w,(plane+1)%3,std::sin(a1)*a.radius);if(a.shape=="capsule"){for(int sign:{-1,1})line(point(add(u,{0,sign*a.half_length,0})),point(add(w,{0,sign*a.half_length,0})),.001,color,vp);}else line(point(u),point(w),.001,color,vp);}if(a.shape=="capsule")for(V u:{V{a.radius,0,0},V{-a.radius,0,0},V{0,0,a.radius},V{0,0,-a.radius}})line(point(add(u,{0,-a.half_length,0})),point(add(u,{0,a.half_length,0})),.001,color,vp);
}
}
