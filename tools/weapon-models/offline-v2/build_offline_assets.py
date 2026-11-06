"""Original offline VR authoring assets. Never loads or modifies the game.

Standard-library geometry and GLB export; Pillow/numpy only for preview rendering.
All sizes are authoring dimensions, not certified chamber fit or live game scale.
"""
from pathlib import Path
import argparse,collections,hashlib,json,math,struct

ROOT=Path(__file__).resolve().parents[3]
DEST=ROOT/'assets/weapon-6dof/offline-v2'
MATS=[('Aged brass',(0.63,.39,.14),.78,.32),('Copper jacket',(.64,.25,.10),.72,.30),
 ('Steel detail',(.22,.27,.31),.78,.34),('Red hull',(.48,.045,.055),.02,.46),
 ('Crimp',(.36,.025,.035),.03,.5),('Dark gunmetal',(.14,.18,.22),.72,.36),
 ('Chamber interior',(.075,.09,.11),.55,.49),('Yellow witness mark',(.72,.48,.08),.25,.4)]

def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def cross(a,b):return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def unit(v):
 d=math.sqrt(dot(v,v))
 if d<1e-14:raise ValueError('degenerate normal')
 return tuple(x/d for x in v)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()

class Mesh:
 def __init__(self):self.parts=collections.defaultdict(lambda:{'p':[],'n':[],'uv':[]})
 def tri(self,a,b,c,mat=0,normals=None,uvs=None):
  normal=unit(cross(sub(b,a),sub(c,a)));p=self.parts[mat]
  p['p'] += [a,b,c];p['n'] += list(normals or [normal]*3);p['uv'] += list(uvs or [(a[0]*10,a[1]*10),(b[0]*10,b[1]*10),(c[0]*10,c[1]*10)])
 def quad(self,a,b,c,d,mat=0,reverse=False):
  if reverse:a,b,c,d=d,c,b,a
  self.tri(a,b,c,mat);self.tri(a,c,d,mat)
 def triangles(self):
  for mat,p in self.parts.items():
   for i in range(0,len(p['p']),3):yield mat,p['p'][i:i+3]

def lathe(profile,segments):
 # Each tuple is radius, z, material for the segment starting at that point.
 mesh=Mesh();length=max(p[1] for p in profile)-min(p[1] for p in profile)
 for (r0,z0,mat),(r1,z1,_) in zip(profile,profile[1:]):
  for i in range(segments):
   a=2*math.pi*i/segments;b=2*math.pi*(i+1)/segments
   pts=[(r0*math.cos(a),r0*math.sin(a),z0),(r0*math.cos(b),r0*math.sin(b),z0),(r1*math.cos(b),r1*math.sin(b),z1),(r1*math.cos(a),r1*math.sin(a),z1)]
   na=unit(((z1-z0)*math.cos(a),(z1-z0)*math.sin(a),r0-r1));nb=unit(((z1-z0)*math.cos(b),(z1-z0)*math.sin(b),r0-r1))
   ns=[na,nb,nb,na];uv=[(i/segments,z0/length),((i+1)/segments,z0/length),((i+1)/segments,z1/length),(i/segments,z1/length)]
   ids=[(0,2,3)] if r0==0 else [(0,1,2)] if r1==0 else [(0,1,2),(0,2,3)]
   for ii in ids:mesh.tri(*[pts[j] for j in ii],mat,[ns[j] for j in ii],[uv[j] for j in ii])
 return mesh

def round_mesh(kind,n):
 if kind=='longshot-round':
  profile=[(0,.00025,2),(.003,.00025,2),(.0035,0,0),(.00937,0,0),(.00937,.0025,0),(.0085,.003,0),(.0085,.005,0),(.009,.006,0),(.0088,.100,0),(.0065,.112,0),(.0065,.1199,1),(.0064,.125,1),(.0057,.134,1),(.0037,.143,1),(0,.150,1)]
 elif kind=='boltok-round':
  profile=[(0,.00025,2),(.0025,.00025,2),(.003,0,0),(.0086,0,0),(.0086,.002,0),(.0078,.003,0),(.0078,.048,0),(.0076,.050,1),(.0075,.058,1),(.0066,.064,1),(.0055,.068,1),(0,.068,1)]
 else:
  profile=[(0,.0002,2),(.0045,.0002,2),(.005,0,0),(.01685,0,0),(.01685,.0025,0),(.01565,.0036,0),(.01565,.0205,3),(.01565,.1005,3),(.01535,.105,4)]
  mesh=lathe(profile,n)
  # Eight folded ridges, sealed at the centre. No floating primer plane.
  for i in range(n):
   a=2*math.pi*i/n;b=2*math.pi*(i+1)/n
   def v(r,t,z):return (r*math.cos(t),r*math.sin(t),z)
   outer=[v(.01535,t,.105) for t in [a,b]]
   fold=[v(.0105,t,.1129+.00055*math.cos(8*t)) for t in [a,b]]
   inner=[v(.0031,t,.1102+.00025*math.cos(8*t)) for t in [a,b]]
   mesh.quad(outer[0],outer[1],fold[1],fold[0],4);mesh.quad(fold[0],fold[1],inner[1],inner[0],4);mesh.tri(inner[0],inner[1],(0,0,.109),4)
  return mesh
 return lathe(profile,n)

def cylinder_mesh(n):
 # Six identical sectors tile a circular face around six real through-holes.
 R=.03033;face_R=R-.0006;C=.0192;hole=.0087;inside=.0082;L=.07474
 vertex_angle=math.atan2(face_R*.5,face_R*math.sqrt(3)/2-C)
 angles=sorted(set([round(2*math.pi*i/n,12) for i in range(n)]+[round(vertex_angle,12),round(2*math.pi-vertex_angle,12),round(math.pi,12)]))
 def boundary(phi):
  dx,dy=math.cos(phi),math.sin(phi);candidates=[]
  t=-C*dx+math.sqrt((C*dx)**2+face_R**2-C*C);candidates.append((t,(C+t*dx,t*dy)))
  for sign in [-1,1]:
   ex,ey=math.sqrt(3)/2,sign*.5;den=dx*ey-dy*ex
   if abs(den)>1e-10:
    t=-C*ey/den;q=(C+t*dx,t*dy);along=q[0]*ex+q[1]*ey
    if t>0 and -.000000001<=along<=face_R+.000000001:candidates.append((t,q))
  return min(candidates,key=lambda v:v[0])[1]
 outer=[boundary(phi) for phi in angles];mesh=Mesh()
 def rot(p,t):return (p[0]*math.cos(t)-p[1]*math.sin(t),p[0]*math.sin(t)+p[1]*math.cos(t))
 def p3(p,z,t):x,y=rot(p,t);return (x,y,z)
 for k in range(6):
  theta=k*math.pi/3
  for i,phi in enumerate(angles):
   j=(i+1)%len(angles);phj=angles[j];aa=outer[i];bb=outer[j]
   hi=(C+hole*math.cos(phi),hole*math.sin(phi));hj=(C+hole*math.cos(phj),hole*math.sin(phj))
   mesh.quad(p3(aa,0,theta),p3(bb,0,theta),p3(hj,0,theta),p3(hi,0,theta),5,True)
   mesh.quad(p3(aa,L,theta),p3(bb,L,theta),p3(hj,L,theta),p3(hi,L,theta),5)
   profile=[(hole,0),(inside,.0015),(inside,L-.0015),(hole,L)]
   for (r0,z0),(r1,z1) in zip(profile,profile[1:]):
    points=[(C+r0*math.cos(phi),r0*math.sin(phi)),(C+r0*math.cos(phj),r0*math.sin(phj)),(C+r1*math.cos(phj),r1*math.sin(phj)),(C+r1*math.cos(phi),r1*math.sin(phi))]
    mesh.quad(*[p3(p,z,theta) for p,z in zip(points,[z0,z0,z1,z1])],6,True)
   if abs(math.hypot(*aa)-face_R)<1e-7 and abs(math.hypot(*bb)-face_R)<1e-7:
    profile=[(1,0),(R/face_R,.0015),(R/face_R,L-.0015),(1,L)]
    for (r0,z0),(r1,z1) in zip(profile,profile[1:]):
     mesh.quad(p3((aa[0]*r0,aa[1]*r0),z0,theta),p3((bb[0]*r0,bb[1]*r0),z0,theta),p3((bb[0]*r1,bb[1]*r1),z1,theta),p3((aa[0]*r1,aa[1]*r1),z1,theta),5)
 return mesh

def validate_mesh(mesh,expected_closed=True):
 edges=collections.Counter();directed=collections.Counter();vol=0.;pts=[];tri_count=0
 def key(p):return tuple(round(x,9) for x in p)
 for _,tri in mesh.triangles():
  a,b,c=tri;assert all(math.isfinite(x) for p in tri for x in p)
  area=math.sqrt(dot(cross(sub(b,a),sub(c,a)),cross(sub(b,a),sub(c,a))))/2;assert area>1e-14
  vol+=dot(a,cross(b,c))/6;pts+=tri;tri_count+=1
  for p,q in zip(tri,[b,c,a]):
   p,q=key(p),key(q);edges[tuple(sorted([p,q]))]+=1;directed[(p,q)]+=1
 bad=sum(v!=2 for v in edges.values());winding=sum(directed[(a,b)]!=directed[(b,a)] for a,b in edges)
 if expected_closed:assert bad==0 and winding==0,(bad,winding)
 assert vol>0,vol
 return {'triangles':tri_count,'unique_positions':len({key(p) for p in pts}),'boundary_or_nonmanifold_edges':bad,'winding_mismatches':winding,'signed_volume_m3':vol,'bounds_min':[min(p[i] for p in pts) for i in range(3)],'bounds_max':[max(p[i] for p in pts) for i in range(3)]}

def glb(kind,meshes,sockets):
 binary=bytearray();views=[];access=[];gmeshes=[]
 def attr(values,ctype,atype,target):
  fmt='f' if ctype==5126 else 'I';flat=[x for p in values for x in p] if atype!='SCALAR' else values
  raw=struct.pack('<'+fmt*len(flat),*flat);offset=len(binary);binary.extend(raw);binary.extend(b'\0'*(-len(binary)%4));vi=len(views);views.append({'buffer':0,'byteOffset':offset,'byteLength':len(raw),'target':target})
  a={'bufferView':vi,'componentType':ctype,'count':len(values),'type':atype}
  if atype=='VEC3':a.update(min=[min(p[i] for p in values) for i in range(3)],max=[max(p[i] for p in values) for i in range(3)])
  access.append(a);return len(access)-1
 for mi,mesh in enumerate(meshes):
  prim=[]
  for mat,p in sorted(mesh.parts.items()):
   pos=attr(p['p'],5126,'VEC3',34962);norm=attr(p['n'],5126,'VEC3',34962);uv=attr(p['uv'],5126,'VEC2',34962);idx=attr(list(range(len(p['p']))),5125,'SCALAR',34963)
   prim.append({'attributes':{'POSITION':pos,'NORMAL':norm,'TEXCOORD_0':uv},'indices':idx,'material':mat})
  gmeshes.append({'name':kind+'_LOD'+str(mi),'primitives':prim})
 nodes=[{'name':'LOD'+str(i),'mesh':i,'extras':{'lod':i}} for i in range(len(meshes))]+sockets
 markers=list(range(len(meshes),len(nodes)))
 doc={'asset':{'version':'2.0','generator':'GearsVR offline authoring v2'},'scene':0,'scenes':[{'name':'LOD'+str(i),'nodes':[i]+markers} for i in range(len(meshes))],'nodes':nodes,'meshes':gmeshes,'materials':[{'name':name,'pbrMetallicRoughness':{'baseColorFactor':[*color,1],'metallicFactor':metal,'roughnessFactor':rough}} for name,color,metal,rough in MATS],'buffers':[{'byteLength':len(binary)}],'bufferViews':views,'accessors':access,'extras':{'units':'metres','sourceMeshCopied':False,'runtimeIntegrated':False,'gameFitVerified':False,'axis':'+Z from base toward tip or cylinder front','lodSelection':'separate scenes, not simultaneous geometry'}}
 j=json.dumps(doc,separators=(',',':'),sort_keys=True).encode();j+=b' '*(-len(j)%4)
 return struct.pack('<4sII',b'glTF',2,28+len(j)+len(binary))+struct.pack('<I4s',len(j),b'JSON')+j+struct.pack('<I4s',len(binary),b'BIN\0')+binary

def render_preview(models,path):
 import numpy as np
 from PIL import Image,ImageDraw,ImageFont
 im=Image.new('RGB',(1800,1200),(18,24,32));d=ImageDraw.Draw(im);f=ImageFont.truetype('C:/Windows/Fonts/arial.ttf',26);small=ImageFont.truetype('C:/Windows/Fonts/arial.ttf',17)
 light=np.array([-.35,-.45,.82]);light/=np.linalg.norm(light)
 panels=[('gnasher-round','Gnasher complete round',(.9,-1.3,.65)),('longshot-round','Longshot complete round',(.8,-1.3,.55)),('boltok-round','Boltok complete round',(.8,-1.3,.8)),('boltok-cylinder','Boltok six-hole cylinder',(.6,-1,-1.4)),('boltok-cylinder','Cylinder / loading face',(0,0,-1))]
 for k,(kind,title,eye) in enumerate(panels):
  x=(k%3)*600;y=(k//3)*570;mesh=models[kind][0];points=np.array([p for _,tri in mesh.triangles() for p in tri]);centre=(points.min(0)+points.max(0))/2
  eye=np.array(eye,dtype=float);eye/=np.linalg.norm(eye);right=np.cross([0,0,1] if abs(eye[2])<.98 else [0,1,0],eye);right/=np.linalg.norm(right);up=np.cross(eye,right);px=(points-centre)@right;py=(points-centre)@up;scale=min(430/max(np.ptp(px),.001),390/max(np.ptp(py),.001))
  # Per-pixel depth avoids painter-sort artifacts on long chamber walls.
  zbuffer=np.full((im.height,im.width),-np.inf,dtype=np.float32)
  for mat,tri in mesh.triangles():
   tri=np.array(tri);n=np.cross(tri[1]-tri[0],tri[2]-tri[0]);n/=np.linalg.norm(n)
   if n@eye<=0:continue
   shading=.36+.64*max(0,float(n@light));color=tuple(round((max(0,min(1,c*shading)))**(1/2.2)*255) for c in MATS[mat][1]);coords=np.array([(x+300+(v-centre)@right*scale,y+285-(v-centre)@up*scale) for v in tri]);zs=tri@eye
   xmin=max(x,int(np.floor(coords[:,0].min())));xmax=min(x+599,int(np.ceil(coords[:,0].max())));ymin=max(y+60,int(np.floor(coords[:,1].min())));ymax=min(y+480,int(np.ceil(coords[:,1].max())))
   if xmin>xmax or ymin>ymax:continue
   xx,yy=np.meshgrid(np.arange(xmin,xmax+1)+.5,np.arange(ymin,ymax+1)+.5)
   a,b,c=coords;den=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1])
   if abs(den)<1e-10:continue
   wa=((b[1]-c[1])*(xx-c[0])+(c[0]-b[0])*(yy-c[1]))/den;wb=((c[1]-a[1])*(xx-c[0])+(a[0]-c[0])*(yy-c[1]))/den;wc=1-wa-wb;depth=wa*zs[0]+wb*zs[1]+wc*zs[2]
   view=zbuffer[ymin:ymax+1,xmin:xmax+1];mask=(wa>=-1e-7)&(wb>=-1e-7)&(wc>=-1e-7)&(depth>view);view[mask]=depth[mask]
   if mask.any():
    patch=np.array(im.crop((xmin,ymin,xmax+1,ymax+1)));patch[mask]=color;im.paste(Image.fromarray(patch),(xmin,ymin))
  d.text((x+25,y+20),title,font=f,fill='#f2f4f7');d.text((x+25,y+505),f'{sum(1 for _ in mesh.triangles())} triangles / LOD0',font=small,fill='#b4c3d4')
  d.text((x+25,y+532),'Original geometry; authoring scale, game fit pending',font=small,fill='#b4c3d4')
 d.text((1225,610),'Offline model set',font=f,fill='#f2f4f7');d.text((1225,660),'Three complete rounds + six-hole cylinder',font=small,fill='#b4c3d4');d.text((1225,690),'Metres / +Z insertion / LOD0 and LOD1',font=small,fill='#b4c3d4');d.text((1225,720),'Grip and insertion markers included',font=small,fill='#b4c3d4');d.text((1225,750),'No game loader or runtime wiring',font=small,fill='#b4c3d4')
 d.text((25,1160),'Different panel scales. Inspect the GLB and dimensions in manifest.json; the preview is not a size comparison.',font=small,fill='#cdbb91');im.save(path)

def write_viewer(models,path):
 packed={}
 for name,lods in models.items():
  packed[name]=[[[mat,[[round(v,7) for v in p] for p in tri]] for mat,tri in mesh.triangles()] for mesh in lods]
 template='''<!doctype html><meta charset="utf-8"><title>GearsVR offline model review</title><style>body{margin:0;background:#121820;color:#e9eef5;font:16px system-ui}header{padding:20px;display:flex;gap:15px;align-items:center}select,button{background:#263444;color:white;border:1px solid #567;padding:9px;border-radius:6px}canvas{display:block;width:100vw;height:78vh;touch-action:none}footer{padding:16px;color:#adbed1}</style><header><b>离线模型检查</b><select id="model"></select><select id="lod"><option value="0">LOD0</option><option value="1">LOD1</option></select><button id="reset">重置视角</button><span id="info"></span></header><canvas id="c"></canvas><footer>拖动旋转，滚轮缩放。原创模型；尺寸为制作值，尚未接入游戏。六孔弹巢需要替换原封闭弹巢，不能叠在封闭端面上。</footer><script>const DATA=__DATA__, COLORS=__COLORS__;const c=document.querySelector('#c'),ctx=c.getContext('2d'),select=document.querySelector('#model'),lod=document.querySelector('#lod');Object.keys(DATA).forEach(k=>select.add(new Option(k,k)));let yaw=.5,pitch=-.4,zoom=1,last=null;const sub=(a,b)=>a.map((v,i)=>v-b[i]),dot=(a,b)=>a.reduce((s,v,i)=>s+v*b[i],0),cross=(a,b)=>[a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]],unit=a=>{let n=Math.hypot(...a);return a.map(x=>x/n)};function render(){c.width=c.clientWidth*devicePixelRatio;c.height=c.clientHeight*devicePixelRatio;ctx.fillStyle='#121820';ctx.fillRect(0,0,c.width,c.height);let tris=DATA[select.value][+lod.value],pts=tris.flatMap(t=>t[1]),lo=[0,1,2].map(i=>Math.min(...pts.map(p=>p[i]))),hi=[0,1,2].map(i=>Math.max(...pts.map(p=>p[i]))),ctr=lo.map((v,i)=>(v+hi[i])/2),scale=Math.min(c.width,c.height)*.72/Math.hypot(...sub(hi,lo))*zoom;let eye=unit([Math.cos(pitch)*Math.sin(yaw),-Math.cos(pitch)*Math.cos(yaw),Math.sin(pitch)]),right=unit(cross([0,0,1],eye)),up=cross(eye,right),faces=[];for(let [m,t] of tris){let n=unit(cross(sub(t[1],t[0]),sub(t[2],t[0])));if(dot(n,eye)<=0)continue;let light=.35+.65*Math.max(0,dot(n,unit([-.35,-.45,.82]))),rgb=COLORS[m].map(v=>Math.round(Math.pow(Math.min(1,v*light),1/2.2)*255));faces.push({z:t.reduce((s,p)=>s+dot(p,eye),0)/3,p:t.map(v=>[c.width/2+dot(sub(v,ctr),right)*scale,c.height/2-dot(sub(v,ctr),up)*scale]),color:'rgb('+rgb.join(',')+')'})}faces.sort((a,b)=>a.z-b.z);for(let f of faces){ctx.fillStyle=f.color;ctx.beginPath();f.p.forEach((p,i)=>i?ctx.lineTo(...p):ctx.moveTo(...p));ctx.closePath();ctx.fill()}document.querySelector('#info').textContent=tris.length+' triangles · '+sub(hi,lo).map(v=>(v*1000).toFixed(1)).join(' × ')+' mm'}c.onpointerdown=e=>{last=[e.clientX,e.clientY];c.setPointerCapture(e.pointerId)};c.onpointerup=()=>last=null;c.onpointermove=e=>{if(!last)return;yaw+=(e.clientX-last[0])*.007;pitch=Math.max(-1.5,Math.min(1.5,pitch+(e.clientY-last[1])*.007));last=[e.clientX,e.clientY];render()};c.onwheel=e=>{e.preventDefault();zoom=Math.max(.4,Math.min(3,zoom*Math.exp(-e.deltaY*.001)));render()};select.onchange=()=>{zoom=1;render()};lod.onchange=render;document.querySelector('#reset').onclick=()=>{yaw=.5;pitch=-.4;zoom=1;render()};window.onresize=render;render();</script>'''
 # Use real depth testing for the interactive view, too.
 start=template.index("const c=document")
 end=template.index('</script>',start)
 webgl='''const c=document.querySelector('#c'),gl=c.getContext('webgl',{antialias:true}),select=document.querySelector('#model'),lod=document.querySelector('#lod');Object.keys(DATA).forEach(k=>select.add(new Option(k,k)));const sub=(a,b)=>a.map((v,i)=>v-b[i]),dot=(a,b)=>a.reduce((s,v,i)=>s+v*b[i],0),cross=(a,b)=>[a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]],unit=a=>{let n=Math.hypot(...a);return a.map(x=>x/n)};let yaw=.5,pitch=-.4,zoom=1,last=null,centre=[0,0,0],radius=1,count=0;function shader(type,src){let s=gl.createShader(type);gl.shaderSource(s,src);gl.compileShader(s);if(!gl.getShaderParameter(s,gl.COMPILE_STATUS))throw Error(gl.getShaderInfoLog(s));return s}let program=gl.createProgram();gl.attachShader(program,shader(gl.VERTEX_SHADER,'attribute vec3 p;attribute vec3 n;attribute vec3 col;uniform vec3 center,right,up,eye,scale;varying vec3 v;void main(){vec3 q=p-center;gl_Position=vec4(dot(q,right)*scale.x,dot(q,up)*scale.y,-dot(q,eye)*scale.z,1.);float light=.35+.65*max(0.,dot(normalize(n),normalize(vec3(-.35,-.45,.82))));v=pow(col*light,vec3(1./2.2));}'));gl.attachShader(program,shader(gl.FRAGMENT_SHADER,'precision mediump float;varying vec3 v;void main(){gl_FragColor=vec4(v,1.);}'));gl.linkProgram(program);if(!gl.getProgramParameter(program,gl.LINK_STATUS))throw Error(gl.getProgramInfoLog(program));gl.useProgram(program);gl.enable(gl.DEPTH_TEST);gl.enable(gl.CULL_FACE);let buffer=gl.createBuffer();gl.bindBuffer(gl.ARRAY_BUFFER,buffer);['p','n','col'].forEach((name,i)=>{let a=gl.getAttribLocation(program,name);gl.enableVertexAttribArray(a);gl.vertexAttribPointer(a,3,gl.FLOAT,false,36,i*12)});const u=name=>gl.getUniformLocation(program,name);function load(){let tris=DATA[select.value][+lod.value],flat=[],lo=[Infinity,Infinity,Infinity],hi=[-Infinity,-Infinity,-Infinity];for(let [m,t] of tris){let n=unit(cross(sub(t[1],t[0]),sub(t[2],t[0])));for(let p of t){flat.push(...p,...n,...COLORS[m]);p.forEach((v,i)=>{lo[i]=Math.min(lo[i],v);hi[i]=Math.max(hi[i],v)})}}centre=lo.map((v,i)=>(v+hi[i])/2);radius=Math.hypot(...sub(hi,lo));count=flat.length/9;gl.bufferData(gl.ARRAY_BUFFER,new Float32Array(flat),gl.STATIC_DRAW);document.querySelector('#info').textContent=tris.length+' triangles · '+sub(hi,lo).map(v=>(v*1000).toFixed(1)).join(' × ')+' mm';render()}function render(){c.width=c.clientWidth*devicePixelRatio;c.height=c.clientHeight*devicePixelRatio;gl.viewport(0,0,c.width,c.height);gl.clearColor(.071,.094,.125,1);gl.clear(gl.COLOR_BUFFER_BIT|gl.DEPTH_BUFFER_BIT);let eye=unit([Math.cos(pitch)*Math.sin(yaw),-Math.cos(pitch)*Math.cos(yaw),Math.sin(pitch)]),right=unit(cross([0,0,1],eye)),up=cross(eye,right),s=Math.min(c.width,c.height)*.72/radius*zoom;gl.uniform3fv(u('center'),centre);gl.uniform3fv(u('right'),right);gl.uniform3fv(u('up'),up);gl.uniform3fv(u('eye'),eye);gl.uniform3fv(u('scale'),[2*s/c.width,2*s/c.height,1/radius]);gl.drawArrays(gl.TRIANGLES,0,count)}c.onpointerdown=e=>{last=[e.clientX,e.clientY];c.setPointerCapture(e.pointerId)};c.onpointerup=()=>last=null;c.onpointermove=e=>{if(!last)return;yaw+=(e.clientX-last[0])*.007;pitch=Math.max(-1.5,Math.min(1.5,pitch+(e.clientY-last[1])*.007));last=[e.clientX,e.clientY];render()};c.onwheel=e=>{e.preventDefault();zoom=Math.max(.4,Math.min(3,zoom*Math.exp(-e.deltaY*.001)));render()};select.onchange=()=>{zoom=1;load()};lod.onchange=load;document.querySelector('#reset').onclick=()=>{yaw=.5;pitch=-.4;zoom=1;render()};window.onresize=render;load();'''
 template=template[:start]+webgl+template[end:]
 path.write_text(template.replace('__DATA__',json.dumps(packed,separators=(',',':'))).replace('__COLORS__',json.dumps([m[1] for m in MATS])),encoding='utf-8')

def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output-dir',type=Path,default=DEST);parser.add_argument('--check',action='store_true');args=parser.parse_args();out=args.output_dir.resolve()
 if out!=DEST.resolve():raise ValueError('This authoring build only writes its dedicated offline-v2 folder')
 out.mkdir(parents=True,exist_ok=True);models={};records=[]
 for kind in ['gnasher-round','longshot-round','boltok-round','boltok-cylinder']:
  meshes=[cylinder_mesh(n) if kind=='boltok-cylinder' else round_mesh(kind,n) for n in [48,16]];models[kind]=meshes;reports=[validate_mesh(m) for m in meshes]
  height=reports[0]['bounds_max'][2];radius=max(abs(reports[0]['bounds_min'][0]),reports[0]['bounds_max'][0])
  sockets=[{'name':'grip_primary','translation':[0,0,height*.45],'extras':{'role':'authoring grip, not calibrated'}},{'name':'insert_base','translation':[0,0,0]},{'name':'insert_tip','translation':[0,0,height]}]
  if kind=='boltok-cylinder':
   for i in range(6):sockets.append({'name':'chamber_'+str(i),'translation':[.0192*math.cos(i*math.pi/3),.0192*math.sin(i*math.pi/3),0],'extras':{'index':i,'axis':'+Z','round_base_seated_offset_m':[0,0,-.003]}})
  raw=glb(kind,meshes,sockets);path=out/(kind+'.glb')
  if args.check:assert path.read_bytes()==raw,path
  else:path.write_bytes(raw)
  records.append({'id':kind,'file':path.name,'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw),'lods':reports,'sourceMeshCopied':False,'units':'metres','gameFitVerified':False,'runtimeIntegrated':False,'collision_authoring':{'shape':'convex hull' if kind!='boltok-cylinder' else 'compound hulls around six ports; do not use one convex hull to detect insertion','envelope_radius':radius,'envelope_length':height},'markers':[s['name'] for s in sockets]})
 # Reuse the existing original starter without touching its source or mainline asset.
 source=ROOT/'assets/weapon-6dof/lancer-starter.glb';assert source.exists();target=out/'lancer-starter.glb'
 if args.check:assert target.read_bytes()==source.read_bytes()
 else:target.write_bytes(source.read_bytes())
 records.append({'id':'lancer-starter','file':target.name,'sha256':sha(target),'bytes':target.stat().st_size,'reuse_of':'../lancer-starter.glb','sourceMeshCopied':False,'runtimeIntegrated':False,'gameFitVerified':False})
 assembly={'schema':'gearsvr.offline-boltok-assembly.v1','units':'metres','runtimeIntegrated':False,'gameFitVerified':False,'cylinder':'boltok-cylinder.glb','round':'boltok-round.glb','roundBaseOffsetFromChamber_m':[0,0,-.003],'reason':'Keep the larger rim behind the loading face, outside the narrower chamber throat.','roundInstances':[{'chamber':i,'translation':[.0192*math.cos(i*math.pi/3),.0192*math.sin(i*math.pi/3),-.003],'rotation_xyzw':[0,0,0,1]} for i in range(6)]}
 manifest={'schema':'gearsvr.offline-original-assets.v2','date':'2026-10-28','scope':'Only original authoring assets. No runtime loader, integration or physical cartridge specification.','dimensions':'Provisional authoring size; cylinder uses 1 raw mesh unit = .01 m for offline design only.','sourceGeometryCopied':False,'assembly':'boltok-assembly.json','assets':records}
 if not args.check:
  (out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8');(out/'boltok-assembly.json').write_text(json.dumps(assembly,indent=2)+'\n',encoding='utf-8');render_preview(models,out/'preview.png');write_viewer(models,out/'preview.html')
 else:
  assert json.loads((out/'manifest.json').read_text())==manifest
  assert json.loads((out/'boltok-assembly.json').read_text())==assembly
 print(json.dumps({'assets':len(records),'closed_mesh_lods':8,'triangles_lod0':{r['id']:r['lods'][0]['triangles'] for r in records if 'lods' in r},'checks':'finite geometry, welded manifold edges, winding, volume, deterministic GLB bytes'}))

if __name__=='__main__':main()
