// Clip a triangle in view space (positive Z forward); keep geometry crossing the near plane.
export function clipNear(triangle,near=.09){const polygon=[];for(let i=0;i<3;i++){const a=triangle[i],b=triangle[(i+1)%3],inside=a[2]>=near,next=b[2]>=near;if(inside)polygon.push(a);if(inside!==next){const t=(near-a[2])/(b[2]-a[2]);polygon.push([a[0]+(b[0]-a[0])*t,a[1]+(b[1]-a[1])*t,near]);}}const result=[];for(let i=1;i+1<polygon.length;i++)result.push([polygon[0],polygon[i],polygon[i+1]]);return result;}
export const UNCLIPPED=[null];
