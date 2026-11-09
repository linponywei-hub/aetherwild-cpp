#!/usr/bin/env python3
"""Build UV-preserving seam repair sidecars. Requires numpy/scipy; originals stay intact.
Only boundary vertices can move, by <= 0.2% of source diagonal. Fill small
planar holes or narrow slits, never large door/window loops. No decimation.
"""
import collections, hashlib, json, math, struct
from pathlib import Path
import numpy as np
from scipy.spatial import cKDTree
ROOT=Path(__file__).resolve().parents[1]/'web/assets/models/buildings/village'
def load(path):
 raw=path.read_bytes();n=struct.unpack_from('<I',raw,12)[0];j=json.loads(raw[20:20+n]);binary=raw[28+n:]
 def accessor(k):
  a=j['accessors'][k];v=j['bufferViews'][a['bufferView']];width={'VEC3':3,'VEC2':2,'SCALAR':1}[a['type']];dtype={5126:'<f4',5125:'<u4',5123:'<u2'}[a['componentType']]
  return np.ndarray((a['count'],width),dtype=dtype,buffer=binary,offset=v.get('byteOffset',0)+a.get('byteOffset',0),strides=(v.get('byteStride',np.dtype(dtype).itemsize*width),np.dtype(dtype).itemsize)).copy()
 p=j['meshes'][0]['primitives'][0];pos=accessor(p['attributes']['POSITION']);faces=accessor(p['indices']).reshape(-1,3)
 if 'NORMAL' in p['attributes']:normals=accessor(p['attributes']['NORMAL'])
 else:
  normals=np.zeros_like(pos);face_normals=np.cross(pos[faces[:,1]]-pos[faces[:,0]],pos[faces[:,2]]-pos[faces[:,0]])
  for i in range(3):np.add.at(normals,faces[:,i],face_normals)
  normals/=np.maximum(np.linalg.norm(normals,axis=1)[:,None],1e-20)
 return raw,pos,faces,normals
def topology(pos,faces):
 u,inv=np.unique(np.round(pos,7),axis=0,return_inverse=True);f=inv[faces];valid=(f[:,0]!=f[:,1])&(f[:,1]!=f[:,2])&(f[:,2]!=f[:,0]);f=f[valid]
 edges=np.concatenate([f[:,[0,1]],f[:,[1,2]],f[:,[2,0]]]);e,cnt=np.unique(np.sort(edges,axis=1),axis=0,return_counts=True)
 return u,inv,f,e,cnt

def triangulate(points):
 # Ear clipping preserves concave boundaries; no fan across a concave opening.
 out=[];remaining=list(range(len(points)));area=np.sum(points[:,0]*np.roll(points[:,1],-1)-points[:,1]*np.roll(points[:,0],-1));sign=1 if area>0 else -1
 cross=lambda a,b:float(a[0]*b[1]-a[1]*b[0])
 while len(remaining)>3:
  found=False
  for k,b in enumerate(remaining):
   a,c=remaining[k-1],remaining[(k+1)%len(remaining)];pa,pb,pc=points[[a,b,c]]
   if cross(pb-pa,pc-pb)*sign<=1e-14:continue
   others=[q for q in remaining if q not in (a,b,c)]
   if any(all(v>=-1e-12 for v in [cross(pb-pa,points[q]-pa)*sign,cross(pc-pb,points[q]-pb)*sign,cross(pa-pc,points[q]-pc)*sign]) for q in others):continue
   out.append((a,b,c));remaining.pop(k);found=True;break
  if not found:return []
 if len(remaining)==3:out.append(tuple(remaining))
 return out

def repair(path):
 raw,pos,faces,source_normals=load(path);original=pos.copy();diag=float(np.linalg.norm(np.ptp(pos,axis=0)));tol=diag*.004
 u,inv,f,e,cnt=topology(pos,faces);before=int(np.sum(cnt==1));boundary=e[cnt==1];ids=np.unique(boundary);adj={tuple(edge) for edge in boundary};parent=np.arange(len(u));members={}
 def find(i):
  while parent[i]!=i:parent[i]=parent[parent[i]];i=parent[i]
  return i
 # Never collapse a boundary edge itself, or grow a transitive cluster beyond tolerance.
 for ia,ib in sorted(cKDTree(u[ids]).query_pairs(tol)):
  a,b=int(ids[ia]),int(ids[ib]);ra,rb=find(a),find(b)
  if ra==rb or tuple(sorted((a,b))) in adj:continue
  aa=members.get(ra,[ra]);bb=members.get(rb,[rb]);combined=aa+bb;cloud=u[combined]
  if len(combined)>8 or np.max(np.linalg.norm(cloud-cloud.mean(axis=0),axis=1))>tol*.5:continue
  if any(tuple(sorted((x,y))) in adj for x in aa for y in bb):continue
  parent[rb]=ra;members[ra]=combined;members.pop(rb,None)
 new=u.copy()
 for cluster in members.values():new[cluster]=u[cluster].mean(axis=0)
 cluster_ids=[i for cluster in members.values() for i in cluster]
 moved=np.isin(inv,cluster_ids)&(np.linalg.norm(new[inv]-pos,axis=1)>1e-8);pos[moved]=new[inv][moved]
 u,inv,f,e,cnt=topology(pos,faces);boundary=e[cnt==1];adj=collections.defaultdict(list)
 for a,b in boundary:adj[int(a)].append(int(b));adj[int(b)].append(int(a))
 representatives={}
 for vertex,uid in enumerate(inv):representatives.setdefault(int(uid),vertex)
 edge_normals=collections.defaultdict(list)
 for tri in f:
  a,b,c=u[tri];normal=np.cross(b-a,c-a)
  for edge in [(tri[0],tri[1]),(tri[1],tri[2]),(tri[2],tri[0])]:
   edge_normals[tuple(sorted(edge))].append(normal)
 visited=set();patches=[];loops=0;skipped=0
 for seed in adj:
  if seed in visited:continue
  component=set();stack=[seed]
  while stack:
   q=stack.pop()
   if q in component:continue
   component.add(q);stack.extend(adj[q])
  visited.update(component)
  if not 3<=len(component)<=48 or any(len(adj[q])!=2 for q in component):skipped+=1;continue
  loop=[seed];previous=None;current=seed
  while True:
   nxt=next(q for q in adj[current] if q!=previous)
   if nxt==seed:break
   loop.append(nxt);previous,current=current,nxt
   if len(loop)>len(component):break
  if len(loop)!=len(component):skipped+=1;continue
  cloud=u[loop];center=cloud.mean(axis=0);_,sv,vh=np.linalg.svd(cloud-center,full_matrices=False);diam=float(np.max(np.linalg.norm(cloud[:,None]-cloud[None,:],axis=2)));flat=float(np.max(np.abs((cloud-center)@vh[-1])))
  projected=(cloud-center)@vh[:2].T;area=abs(float(np.sum(projected[:,0]*np.roll(projected[:,1],-1)-projected[:,1]*np.roll(projected[:,0],-1))))*.5
  narrow=sv[1]/max(sv[0],1e-12)<.12
  if flat>diag*.0015 or not ((diam<diag*.035 and area<diag*diag*.0002) or (narrow and diam<diag*.12 and area<diag*diag*.001)):skipped+=1;continue
  triangles=triangulate(projected)
  if not triangles:skipped+=1;continue
  normals=[n for i,q in enumerate(loop) for n in edge_normals[tuple(sorted((q,loop[(i+1)%len(loop)])))]];normal=np.sum(normals,axis=0)
  for triangle in triangles:
   ids=[loop[k] for k in triangle];a,b,c=u[ids]
   if np.dot(np.cross(b-a,c-a),normal)<0:ids[1],ids[2]=ids[2],ids[1]
   patches.extend(representatives[q] for q in ids)
  loops+=1
 # Bridge matching, nearly coplanar opposing boundary edges across narrow tears.
 # A boundary participates at most once; never bridge across a door-sized gap.
 mid=u[boundary].mean(axis=1);tree=cKDTree(mid);used=set();bridges=0;candidates=[]
 for ia,ib in tree.query_pairs(diag*.006):
  a,b=map(int,boundary[ia]);c,d=map(int,boundary[ib]);
  if len({a,b,c,d})<4:continue
  va=u[b]-u[a];vb=u[d]-u[c];la=np.linalg.norm(va);lb=np.linalg.norm(vb)
  if min(la,lb)<1e-9 or abs(np.dot(va,vb)/(la*lb))<.85:continue
  if np.linalg.norm(u[a]-u[d])+np.linalg.norm(u[b]-u[c])<np.linalg.norm(u[a]-u[c])+np.linalg.norm(u[b]-u[d]):c,d=d,c
  gap=max(np.linalg.norm(u[a]-u[c]),np.linalg.norm(u[b]-u[d]))
  if gap>diag*.006 or gap<tol*.25:continue
  na=np.sum(edge_normals[tuple(sorted((a,b)))],axis=0);nb=np.sum(edge_normals[tuple(sorted((c,d)))],axis=0);nl=np.linalg.norm(na)*np.linalg.norm(nb)
  if nl<1e-20 or abs(np.dot(na,nb)/nl)<.9:continue
  normal=na/max(np.linalg.norm(na),1e-20)
  if max(abs(np.dot(u[c]-u[a],normal)),abs(np.dot(u[d]-u[a],normal)))>diag*.0015:continue
  candidates.append((gap,ia,ib,a,b,c,d,normal))
 for _,ia,ib,a,b,c,d,normal in sorted(candidates,key=lambda q:q[0]):
  if ia in used or ib in used:continue
  for ids in [[a,b,c],[b,d,c]]:
   xyz=u[ids];n=np.cross(xyz[1]-xyz[0],xyz[2]-xyz[0])
   if np.linalg.norm(n)<1e-14:continue
   if np.dot(n,normal)<0:ids[1],ids[2]=ids[2],ids[1]
   patches.extend(representatives[q] for q in ids)
  used.update([ia,ib]);bridges+=1
 new_area=np.linalg.norm(np.cross(pos[faces[:,1]]-pos[faces[:,0]],pos[faces[:,2]]-pos[faces[:,0]]),axis=1)
 removed=np.flatnonzero(new_area<1e-14).tolist()
 repaired_faces=np.concatenate([faces[new_area>=1e-14],np.asarray(patches,dtype=np.int64).reshape(-1,3)])
 _,_,_,_,after_count=topology(pos,repaired_faces);after=int(np.sum(after_count==1))
 updates=np.column_stack([np.flatnonzero(moved),pos[moved]]).reshape(-1).tolist()
 cross=np.cross(pos[repaired_faces[:,1]]-pos[repaired_faces[:,0]],pos[repaired_faces[:,2]]-pos[repaired_faces[:,0]]);flipped=int(np.sum(np.einsum('ij,ij->i',cross,source_normals[repaired_faces].sum(axis=1))<0))
 report={'source':path.name,'sourceSha256':hashlib.sha256(raw).hexdigest(),'vertices':len(pos),'sourceTriangles':len(faces),'removedDegenerates':len(removed),'reorientedTriangles':flipped,'movedVertices':int(moved.sum()),'maxShift':float(np.max(np.linalg.norm(pos-original,axis=1))),'maxAllowedShift':tol,'filledLoops':loops,'bridgedSeams':bridges,'addedTriangles':len(patches)//3,'boundaryBefore':before,'boundaryAfter':after,'skippedOpenings':skipped,'method':'boundary-only bounded weld + planar micro-hole / narrow-slit ear clipping; original UVs and baked texture maps retained; winding aligned to original normal field; missing vertex normals rebuilt on load'}
 payload={'version':1,'sourceSha256':report['sourceSha256'],'vertices':len(pos),'updates':[round(x,9) if k%4 else int(x) for k,x in enumerate(updates)],'triangles':patches,'removeTriangles':removed}
 dest=path.with_suffix('.repair.json');dest.write_text(json.dumps(payload,separators=(',',':'))+'\n');report['patchSha256']=hashlib.sha256(dest.read_bytes()).hexdigest();report['patchBytes']=dest.stat().st_size
 assert after<before and report['maxShift']<=tol and report['addedTriangles']>0
 print(json.dumps(report),flush=True);return report
if __name__=='__main__':
 reports=[repair(p) for p in sorted(ROOT.glob('village_building_*_lod0.glb'))]
 (ROOT/'repair-report.json').write_text(json.dumps({'version':1,'houses':reports},ensure_ascii=False,indent=2)+'\n')
