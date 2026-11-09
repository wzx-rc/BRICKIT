// BrickIt PC: the line pictograms of the library (Maquette 4.1, board Categories.dc.html of the Design canvas), exported as
// SVG path data for the native drawer: node tools/brickit/studio_icons.js -> src/brickit/BrickStudioIcons.inl
var C30 = 0.8660254, S30 = 0.5;
function f1(v){ return Math.round(v*10)/10; }
function Geo(u){ this.u = u; this.minx = 1e9; this.miny = 1e9; this.maxx = -1e9; this.maxy = -1e9; }
Geo.prototype.p = function(x,y,z){ var sx = (x-z)*C30*this.u, sy = (x+z)*S30*this.u - y*this.u; if (sx<this.minx) this.minx=sx; if (sx>this.maxx) this.maxx=sx; if (sy<this.miny) this.miny=sy; if (sy>this.maxy) this.maxy=sy; return [sx,sy]; };
Geo.prototype.poly = function(pts){ var g = this; return 'M' + pts.map(function(q){ var s = g.p(q[0],q[1],q[2]); return f1(s[0])+' '+f1(s[1]); }).join(' L') + ' Z'; };
Geo.prototype.ell = function(cx,cy,rx,ry){ return 'M'+f1(cx-rx)+' '+f1(cy)+' a'+f1(rx)+' '+f1(ry)+' 0 1 0 '+f1(2*rx)+' 0 a'+f1(rx)+' '+f1(ry)+' 0 1 0 '+f1(-2*rx)+' 0 Z'; };
Geo.prototype.cyl = function(cx,cz,r,y0,y1){
  var t = this.p(cx,y1,cz), b = this.p(cx,y0,cz), rx = r*C30*Math.SQRT2*this.u, ry = r*S30*Math.SQRT2*this.u;
  this.p(cx+r,y1,cz-r); this.p(cx-r,y0,cz+r);
  return {side:'M'+f1(t[0]-rx)+' '+f1(t[1])+' L'+f1(b[0]-rx)+' '+f1(b[1])+' A'+f1(rx)+' '+f1(ry)+' 0 0 0 '+f1(b[0]+rx)+' '+f1(b[1])+' L'+f1(t[0]+rx)+' '+f1(t[1])+' Z', top:this.ell(t[0],t[1],rx,ry)};
};
// ---- the same pictogram language as the 4.0 board: a thin rounded stroke, faces lightly tinted (trans-clear)
var PF = 'rgba(225,228,234,0.06)', NF = 'none';
function box(g,x0,y0,z0,x1,y1,z1){   // the three faces seen: front (z1), right (x1), top (y1)
  return [g.poly([[x0,y0,z1],[x1,y0,z1],[x1,y1,z1],[x0,y1,z1]]), g.poly([[x1,y0,z0],[x1,y1,z0],[x1,y1,z1],[x1,y0,z1]]), g.poly([[x0,y1,z0],[x1,y1,z0],[x1,y1,z1],[x0,y1,z1]])];
}
function studs(g, cells, y, r, h){ var ss = '', st = ''; cells.sort(function(p,q){ return (p[0]+p[1])-(q[0]+q[1]); }).forEach(function(c){ var k = g.cyl(c[0],c[1],r||0.3,y,y+(h||0.18)); ss += k.side; st += k.top; }); return [ss, st]; }
function grid(x0,z0,w,l){ var o = []; for (var a=0;a<w;a++) for (var b=0;b<l;b++) o.push([x0+a+0.5,z0+b+0.5]); return o; }
function prismZ(g, pts, z0, z1){   // a 2D profile (x, y) pushed along z: back, the side bands, front
  var L = [g.poly(pts.map(function(p){ return [p[0],p[1],z0]; }))], side = '';
  for (var i=0;i<pts.length;i++){ var a = pts[i], b = pts[(i+1)%pts.length]; side += g.poly([[a[0],a[1],z0],[b[0],b[1],z0],[b[0],b[1],z1],[a[0],a[1],z1]]); }
  L.push(side, g.poly(pts.map(function(p){ return [p[0],p[1],z1]; }))); return L;
}
function prismY(g, pts, y0, y1){   // a 2D outline (x, z) raised along y: the walls, the top
  var side = '';
  for (var i=0;i<pts.length;i++){ var a = pts[i], b = pts[(i+1)%pts.length]; side += g.poly([[a[0],y0,a[1]],[b[0],y0,b[1]],[b[0],y1,b[1]],[a[0],y1,a[1]]]); }
  return [side, g.poly(pts.map(function(p){ return [p[0],y1,p[1]]; }))];
}
var ISO = {
  brick: function(g){ var L = box(g,0,0,0,2,1.2,4); return L.concat(studs(g,grid(0,0,2,4),1.2)); },
  plate: function(g){ var L = box(g,0,0,0,2,0.4,4); return L.concat(studs(g,grid(0,0,2,4),0.4)); },
  tile: function(g){ return box(g,0,0,0,2,0.4,2); },
  baseplate: function(g){ var L = box(g,0,0,0,6,0.14,6); return L.concat(studs(g,grid(0,0,6,6),0.14,0.26,0.12)); },
  bracket: function(g){ var L = box(g,0,0,0,2,2,0.4).concat(box(g,0,0,0,2,0.4,2)); return L.concat(studs(g,[[0.5,1.2],[1.5,1.2]],0.4)); },
  panel: function(g){ var L = box(g,0,0,0,0.16,2.4,2).concat(box(g,0,0,0,1,0.4,2)); return L.concat(studs(g,[[0.6,0.5],[0.6,1.5]],0.4,0.26)); },
  slope: function(g){ var L = prismZ(g, [[0,0],[2,0],[2,0.35],[1,1.2],[0,1.2]], 0, 2); return L.concat(studs(g,[[0.5,0.5],[0.5,1.5]],1.2)); },
  wedge: function(g){ var L = prismY(g, [[0,0],[2,0],[2,2],[0,4]], 0, 0.4); return L.concat(studs(g,[[0.5,0.5],[1.5,0.5],[0.5,1.5],[1.5,1.5],[0.5,2.5]],0.4)); },
  arch: function(g){ var pts = [[0,0],[0,2.4],[4,2.4],[4,0],[3.3,0]]; for (var i=0;i<=10;i++){ var t = i/10*Math.PI; pts.push([2+1.3*Math.cos(t), 1.25*Math.sin(t)]); } pts.push([0.7,0]); var L = prismZ(g, pts, 0, 1); return L.concat(studs(g,grid(0,0,4,1),2.4)); },
  round: function(g){ var c = g.cyl(1,1,0.98,0,1.2), s = studs(g,[[1,1]],1.2); return [c.side, c.top].concat(s); },
  cone: function(g){ var b = g.cyl(1,1,0.98,0,0.14), t = g.cyl(1,1,0.42,1.1,1.2), A = g.p(1-0.98*0.7071,0.14,1+0.98*0.7071), B = g.p(1+0.98*0.7071,0.14,1-0.98*0.7071), C = g.p(1-0.42*0.7071,1.1,1+0.42*0.7071), D = g.p(1+0.42*0.7071,1.1,1-0.42*0.7071);
    return [b.side, b.top, 'M'+f1(A[0])+' '+f1(A[1])+' L'+f1(C[0])+' '+f1(C[1])+' M'+f1(B[0])+' '+f1(B[1])+' L'+f1(D[0])+' '+f1(D[1]), t.side, t.top]; },
  beam: function(g){ var L = box(g,0,0,0,1,1,5), h = ''; for (var i=0;i<5;i++){ var pts = []; for (var a=0;a<16;a++){ var t = a/16*Math.PI*2; pts.push([1, 0.5+0.3*Math.sin(t), i+0.5+0.3*Math.cos(t)]); } h += g.poly(pts); } return L.concat([h]); },
  tbrick: function(g){ var L = box(g,0,0,0,1,1.2,4), h = ''; for (var i=0;i<3;i++){ var pts = []; for (var a=0;a<16;a++){ var t = a/16*Math.PI*2; pts.push([1, 0.6+0.3*Math.sin(t), i+1+0.3*Math.cos(t)]); } h += g.poly(pts); } return L.concat([h], studs(g,grid(0,0,1,4),1.2)); },
  window: function(g){ var L = box(g,0,0,0,1,2.4,3); L.push(g.poly([[0.2,0.4,3.01],[0.8,0.4,3.01],[0.8,2.0,3.01],[0.2,2.0,3.01]].map(function(p){ return [p[0]*0+1.01, p[1], 0.3+p[0]*3.0]; }))); return L; },
  container: function(g){ var L = box(g,0,0,0,3,2,2), r = ''; for (var i=1;i<6;i++) r += g.poly([[i*0.5,0.2,2],[i*0.5,1.8,2]]); L.push(r); L.push(g.poly([[0.2,2,0.2],[2.8,2,0.2],[2.8,2,1.8],[0.2,2,1.8]])); return L; },
  duplo: function(g){ var L = box(g,0,0,0,2,1.6,2); return L.concat(studs(g,grid(0,0,2,2),1.6,0.36,0.3)); },
  stairs: function(g){ return prismZ(g, [[0,0],[3,0],[3,2.4],[2,2.4],[2,1.6],[1,1.6],[1,0.8],[0,0.8]], 0, 2); },
  platform: function(g){ var L = []; [[0.1,0.1],[2.6,0.1],[0.1,2.6],[2.6,2.6]].forEach(function(p){ L = L.concat(box(g,p[0],0,p[1],p[0]+0.3,1.6,p[1]+0.3)); }); L = L.concat(box(g,0,1.6,0,3,2,3)); return [L.join(' ')].concat(studs(g,grid(0,0,3,3),2)); },
  support: function(g){ var c = g.cyl(1,1,0.45,0.4,2.8); return box(g,0,0,0,2,0.4,2).concat([c.side, c.top]).concat(box(g,0,2.8,0,2,3.2,2)); },
  homemaker: function(g){ var L = box(g,0,0,0,2,3,1.2); L.push(g.poly([[1,0.2,1.2],[1,2.8,1.2]])+g.poly([[0,1.5,1.2],[2,1.5,1.2]])); return L; }
};
function circ(cx,cy,r){ return 'M'+f1(cx-r)+' '+f1(cy)+' a'+f1(r)+' '+f1(r)+' 0 1 0 '+f1(2*r)+' 0 a'+f1(r)+' '+f1(r)+' 0 1 0 '+f1(-2*r)+' 0 Z'; }
function ell(cx,cy,rx,ry){ return 'M'+f1(cx-rx)+' '+f1(cy)+' a'+f1(rx)+' '+f1(ry)+' 0 1 0 '+f1(2*rx)+' 0 a'+f1(rx)+' '+f1(ry)+' 0 1 0 '+f1(-2*rx)+' 0 Z'; }
function rotPath(pts, ang, cx, cy){ var c = Math.cos(ang), s = Math.sin(ang); return 'M' + pts.map(function(p){ var x = p[0]-cx, y = p[1]-cy; return f1(cx + x*c - y*s)+' '+f1(cy + x*s + y*c); }).join(' L') + ' Z'; }
function gear(n){ var ro = 40, ri = 32, d = ''; for (var j=0;j<n;j++){ var a0=j/n*Math.PI*2, a1=(j+0.2)/n*Math.PI*2, a2=(j+0.5)/n*Math.PI*2, a3=(j+0.7)/n*Math.PI*2; d += (j?' L':'M')+f1(50+Math.cos(a0)*ri)+' '+f1(50+Math.sin(a0)*ri)+' L'+f1(50+Math.cos(a1)*ro)+' '+f1(50+Math.sin(a1)*ro)+' L'+f1(50+Math.cos(a2)*ro)+' '+f1(50+Math.sin(a2)*ro)+' L'+f1(50+Math.cos(a3)*ri)+' '+f1(50+Math.sin(a3)*ri); } return [[d+' Z',1],[circ(50,50,13),1],['M44 50h12M50 44v12',0]]; }
function wheel(tyre){ var L = [[circ(50,50,40),1],[circ(50,50,tyre?30:25),1],[circ(50,50,7),1]]; var sp = ''; if (!tyre) for (var i=0;i<5;i++){ var t = -Math.PI/2 + i/5*Math.PI*2; sp += 'M'+f1(50+Math.cos(t)*7)+' '+f1(50+Math.sin(t)*7)+' L'+f1(50+Math.cos(t)*25)+' '+f1(50+Math.sin(t)*25); } else for (var k=0;k<24;k++){ var u = k/24*Math.PI*2; sp += 'M'+f1(50+Math.cos(u)*30)+' '+f1(50+Math.sin(u)*30)+' L'+f1(50+Math.cos(u)*40)+' '+f1(50+Math.sin(u)*40); } L.push([sp,0]); return L; }
function propeller(){ var L = [], blade = [[50,43],[45,30],[45,16],[50,9],[55,16],[55,30]]; for (var i=0;i<3;i++) L.push([rotPath(blade, i*Math.PI*2/3, 50, 50),1]); L.push([circ(50,50,7),1]); return L; }
function wheels(xs, y, r){ return xs.map(function(x){ return [circ(x,y,r),1]; }); }
// flat pictograms (100 x 100): [path, faces tinted?]
var FLAT = {
  clock: [[circ(50,50,38),1],['M50 26v24l16 10',0]],
  star: [['M50 12l11 23 25 3-18 17 5 25-23-12-23 12 5-25-18-17 25-3z',1]],
  dots: [[circ(26,50,5),1],[circ(50,50,5),1],[circ(74,50,5),1]],
  all: [['M18 18h26v26H18zM56 18h26v26H56zM18 56h26v26H18zM56 56h26v26H56z',1]],
  dish: [['M14 62c0-20 16-34 36-34s36 14 36 34',1],[ell(50,62,36,10),1],[ell(50,30,6,2.4),0]],
  sphere: [[circ(50,50,34),1],['M16 50a34 10 0 0 0 68 0',0],['M50 16c-14 10-14 58 0 68',0]],
  hinge: [['M10 58h34v14H10zM56 58h34v14H56z',1],[circ(50,65,8),1],[circ(50,65,2.5),0]],
  turntable: [['M14 56v10a36 13 0 0 0 72 0V56',1],[ell(50,56,36,13),1],[ell(50,52,20,7),1],[ell(50,49,5,2),0]],
  bar: [['M14 45h72a5 5 0 0 1 0 10H14a5 5 0 0 1 0-10z',1],[ell(14,50,3,5),0]],
  lever: [['M28 70h44v14H28z',1],['M50 70L68 26',0],[circ(69,23,6),1]],
  winch: [['M26 36h40v26H26z',1],[ell(26,49,5,13),1],['M66 49h12v14h6',0],['M46 62v18',0],['M46 80a6 6 0 1 0 6 6',0],['M14 30v40M78 30v20',0]],
  crane: [['M30 90V20h8v70',1],['M30 20l40 0l-32 12',0],['M38 20l-8-10h8z',1],['M70 20v32',0],['M70 52a5 5 0 1 0 5 5',0],['M22 90h24',0]],
  grab: [[circ(50,36,6),1],['M50 12v18',0],['M46 40L30 56l6 26M54 40l16 16-6 26',0]],
  magnet: [['M24 18v36a26 26 0 0 0 52 0V18H62v36a12 12 0 0 1-24 0V18z',1],['M24 30h14M62 30h14',0]],
  conveyor: [['M20 50h60a10 10 0 0 1 0 20H20a10 10 0 0 1 0-20z',1],[circ(20,60,5),0],[circ(80,60,5),0],['M34 34h16v16H34zM56 38h12v12H56z',1]],
  screw: [['M42 12h16v10H42z',1],['M45 22v58l5 8 5-8V22z',1],['M45 30l10 4M45 40l10 4M45 50l10 4M45 60l10 4M45 70l10 4',0]],
  rack: [['M10 58h80v14H10z',1],['M14 58v-8h6v8M28 58v-8h6v8M42 58v-8h6v8M56 58v-8h6v8M70 58v-8h6v8M84 58v-8h4',0]],
  hose: [['M14 64c12-34 24 26 36-4s24-30 36 2',0],[circ(14,66,5),1],[circ(86,62,5),1]],
  tap: [['M26 34h34v14H26z',1],['M60 40h12a10 10 0 0 1 10 10v8',0],['M38 34V24h10v10M32 24h22',0],['M82 66c-3 4-3 8 0 9 3-1 3-5 0-9z',1]],
  car: [['M12 64V52l14-5 10-15h30l12 15 10 5v12z',1],['M40 36h10v11H34zM54 36h10l8 11H54z',0]].concat(wheels([30,70],66,8)),
  vehicle: [['M10 30h48v34H10z',1],['M58 42h20l12 12v10H58z',1],['M64 46h12l6 8H64z',0]].concat(wheels([24,74],68,8)),
  train: [['M18 24h52a10 10 0 0 1 10 10v38H18z',1],['M26 32h18v14H26zM52 32h18v14H52z',0],['M10 88h80',0]].concat(wheels([30,52,72],78,6)),
  monorail: [['M8 82h84v8H8z',1],['M22 40h54a12 12 0 0 1 12 12v22H22z',1],['M30 48h14v12H30zM52 48h14v12H52z',0],['M40 74v8M68 74v8',0]],
  boat: [['M8 58h84l-14 24H22z',1],['M30 58V36h34v22',1],['M38 44h6v6h-6zM50 44h6v6h-6z',0],['M64 46h14',0]],
  plane: [['M50 10c4 0 6 6 6 14v18l32 16v8l-32-8v18l10 8v6l-16-4-16 4v-6l10-8V58l-32 8v-8l32-16V24c0-8 2-14 6-14z',1]],
  wing: [['M10 62L78 26l12 8-24 34z',1],['M34 50l38-18',0]],
  tail: [['M16 82h68L66 20H50z',1],['M58 30l8 42',0]],
  propeller: propeller(),
  cockpit: [['M12 72h76',0],['M18 72c4-26 22-40 44-40 12 0 20 10 24 40z',1],['M50 33v39',0]],
  windscreen: [['M16 76l12-46h44l12 46z',1],['M30 40l8-4',0]],
  exhaust: [['M18 74h34V34h12v52H18z',1],[circ(66,22,6),0],[circ(78,14,4),0]],
  tractor: [['M18 44h30V20h22v24h16v18H18z',1],['M54 26h10v14H54z',0],[circ(34,66,15),1],[circ(34,66,5),0],[circ(80,70,9),1]],
  trailer: [['M20 28h68v38H20z',1],['M8 58h12',0]].concat(wheels([62,78],72,7)),
  tipper: [['M16 56l46-30 22 22-50 12z',1],['M12 66h76',0]].concat(wheels([28,72],74,7)),
  forklift: [['M18 40h30v30H18z',1],['M26 40V22h22v18',0],['M54 82V16M54 76h32',0]].concat(wheels([26,46],76,7)),
  door: [['M28 8h44v86H28z',1],['M35 15h30v72H35z',1],['M50 15v72M35 51h30',0],['M60 58v4',0]],
  windowf: [['M24 14h52v72H24z',1],['M32 22h36v56H32z',1],['M50 22v56M32 50h36',0]],
  glass: [['M22 16h56v68H22z',1],['M32 36l16-12M32 54l30-22',0]],
  fence: [['M14 40h72M14 64h72',0],['M18 82V26l5-6 5 6v56zM38 82V26l5-6 5 6v56zM58 82V26l5-6 5 6v56zM78 82V26l5-6 5 6v56z',1]],
  ladder: [['M32 8h6v84h-6zM62 8h6v84h-6z',1],['M38 20h24M38 36h24M38 52h24M38 68h24M38 84h24',0]],
  garage: [['M10 44L50 16l40 28v42H10z',1],['M24 86V54h52v32',1],['M24 62h52M24 70h52M24 78h52',0]],
  roadsign: [['M50 10l26 26-26 26-26-26z',1],['M50 62v28M38 90h24',0],['M44 36h12',0]],
  flag: [['M26 92V10',0],['M26 14h50l-12 16 12 16H26z',1],[circ(26,10,3),1]],
  antenna: [['M50 92V32M50 32L28 10M50 32L72 10M38 92h24',0],[circ(50,32,5),1],['M36 22a20 20 0 0 0 28 0',0]],
  fig: [['M38 70h11v22H38zM51 70h11v22H51z',1],['M37 40h26l3 30H34z',1],['M31 42l6-2-2 20-6-1zM69 42l-6-2 2 20 6-1z',1],['M41 12h18a4 4 0 0 1 4 4v16a4 4 0 0 1-4 4H41a4 4 0 0 1-4-4V16a4 4 0 0 1 4-4zM45 6h10v6H45z',1],['M45 22v2M55 22v2M45 28q5 4 10 0',0]],
  sword: [['M46 12l4-6 4 6v56h-8z',1],['M36 68h28v6H36z',1],['M50 74v18',0],[circ(50,94,3),0]],
  footwear: [['M36 14h22v44c12 6 22 18 22 30H22c0-16 8-24 14-30z',1],['M36 26h22',0]],
  headwear: [['M20 64c0-24 14-40 30-40s30 16 30 40z',1],['M12 64h76v9H12z',1],['M50 24v10',0]],
  hipwear: [['M34 18h32l16 64H18z',1],['M34 28h32',0],['M42 28l-6 54M58 28l6 54',0]],
  neckwear: [['M30 18h40l16 74H14z',1],['M40 18c0 10 20 10 20 0',0]],
  figure: [[circ(50,22,12),1],['M36 38h28l6 30H30z',1],['M38 68v24M62 68v24',0],['M36 42l-10 22M64 42l10 22',0]],
  handbag: [['M26 40h48l6 44H20z',1],['M38 40c0-18 24-18 24 0',0],['M44 52h12',0]],
  arm: [['M36 16c12 0 20 8 20 20v28H42V36c0-4-2-6-6-6z',1],['M42 64h14v8a7 7 0 0 1-14 0z',1]],
  animal: [['M24 30l10-18 12 12h8l12-12 10 18v26c0 18-12 30-26 30S24 74 24 56z',1],['M40 48v4M60 48v4M46 62q4 4 8 0',0]],
  leaves: [['M50 92V48',0],['M50 60C34 60 24 48 26 34c13 1 22 11 24 26zM50 60c16 0 26-12 24-26-13 1-22 11-24 26z',1]],
  flower: [['M50 92V48',0],['M50 60C34 60 24 48 26 34c13 1 22 11 24 26zM50 60c16 0 26-12 24-26-13 1-22 11-24 26z',1],[circ(50,30,12),1],[circ(50,30,4),1]],
  tree: [['M50 80v12',0],['M50 10 70 38H30zM50 26 76 58H24zM50 44 82 80H18z',1]],
  rock: [['M12 78l10-32 22-16 22 6 18 22 4 20z',1],['M44 30l6 22 34 6M22 46l28 6',0]],
  axle: [['M10 45h70v10H10z',1],['M80 40h8v20h-8z',1],['M84 44v12M80 50h8',0]],
  pin: [['M12 42h28v16H12z',1],['M40 38h8v24h-8z',1],['M48 42h34a8 8 0 0 1 0 16H48z',1],['M62 42v16',0]],
  connector: [[rotPath([[46,52],[86,52],[86,72],[46,72]], -0.87, 50, 62),1],['M20 52h30v20H20a10 10 0 0 1 0-20z',1],['M16 62h8M20 58v8',0],[circ(73,35,5),0]],
  tpanel: [['M16 82C16 40 40 18 84 18v16C52 34 32 52 32 82z',1],[circ(70,26,3),0],[circ(24,70,3),0]],
  steering: [[circ(50,50,36),1],[circ(50,50,9),1],['M14 50h27M59 50h27M50 59v27',0]],
  pneumatic: [['M12 40h48v22H12z',1],['M60 46h26v10H60z',1],['M86 42v18',0],['M22 40v-8h10v8',0]],
  tread: [['M20 34h60a16 16 0 0 1 0 32H20a16 16 0 0 1 0-32z',1],[circ(20,50,10),1],[circ(80,50,10),1],['M34 34v-4M46 34v-4M58 34v-4M34 66v4M46 66v4M58 66v4',0]],
  motor: [['M14 28h52v44H14z',1],['M66 45h20v10H66z',1],['M24 38h32M24 50h32M24 62h32',0]],
  flex: [['M12 72C30 72 30 28 50 28s20 44 38 44',0],['M6 68h10v8H6zM84 68h10v8H84z',1]],
  bolt: [['M58 8L24 56h22l-8 36 38-50H54z',1]],
  znap: [[circ(50,52,9),1],['M50 43V14M42 57L18 76M58 57l24 19',0],[circ(50,12,6),1],[circ(16,78,6),1],[circ(84,78,6),1]],
  mask: [['M30 18h40c6 22 6 42-6 64H36c-12-22-12-42-6-64z',1],['M38 40h10v8H38zM52 40h10v8H52z',1],['M44 62h12',0]],
  heart: [['M50 82C20 62 12 46 18 32s24-16 32 0c8-16 26-14 32 0s-2 30-32 50z',1]],
  sticker: [['M18 20h64v42L64 80H18z',1],['M82 62H64v18',1]],
  cardboard: [['M16 40h68v44H16z',1],['M16 40l12-18h44l12 18M50 22v18',0]],
  fabric: [['M16 22c12 6 22-6 34 0s22 6 34 0v58c-12 6-22-6-34 0s-22-6-34 0z',1]],
  sheet: [['M22 14h40l18 18v54H22z',1],['M62 14v18h18',0]],
  obsolete: [['M16 30h68v52H16z',1],['M12 22h76v10H12z',1],['M40 46h20',0],['M36 58l28 16M64 58L36 74',0]],
  povray: [[circ(46,56,26),1],['M20 56a26 8 0 0 0 52 0',0],['M76 18l6-6M80 28h8M68 14V6',0],[circ(36,46,5),0]]
};
function slots(L, vb){ var o = {vb:vb}; for (var i=0;i<8;i++) o['p'+i] = L[i] ? {d:L[i][0], f:L[i][1] ? PF : NF} : {d:'', f:'none'}; return o; }
function icon(key){
  if (ISO[key]){
    var g = new Geo(10), L = ISO[key](g);
    var w = g.maxx-g.minx, h = g.maxy-g.miny, s = Math.max(w,h), m = s*0.12+2, cx = (g.minx+g.maxx)/2, cy = (g.miny+g.maxy)/2, half = s/2+m;
    return slots(L.map(function(d){ return [d,1]; }), f1(cx-half)+' '+f1(cy-half)+' '+f1(2*half)+' '+f1(2*half));
  }
  if (key==='gear') return slots(gear(12), '0 0 100 100');
  if (key==='gear8') return slots(gear(8), '0 0 100 100');
  if (key==='wheel') return slots(wheel(false), '0 0 100 100');
  if (key==='tyre') return slots(wheel(true), '0 0 100 100');
  return slots(FLAT[key] || FLAT.dots, '0 0 100 100');
}

// ---- export: every pictogram as its view box (square) and its paths (a tinted face, or a line only)
function exportIcon(key){
  if (ISO[key]){
    var g = new Geo(10), L = ISO[key](g);
    var w = g.maxx-g.minx, h = g.maxy-g.miny, s = Math.max(w,h), m = s*0.12+2, cx = (g.minx+g.maxx)/2, cy = (g.miny+g.maxy)/2, half = s/2+m;
    return {vb:[f1(cx-half), f1(cy-half), f1(2*half)], paths: L.filter(function(d){ return d; }).map(function(d){ return [d,1]; })};
  }
  var P = key==='gear' ? gear(12) : key==='gear8' ? gear(8) : key==='wheel' ? wheel(false) : key==='tyre' ? wheel(true) : FLAT[key];
  return {vb:[0,0,100], paths: P.filter(function(p){ return p[0]; })};
}
var keys = Object.keys(ISO).concat(Object.keys(FLAT), ['gear','gear8','wheel','tyre']);
var NL = String.fromCharCode(10), Q = String.fromCharCode(34);
var out = '// Generated by tools/brickit/studio_icons.js from the Maquette 4.1 (do not edit): the line pictograms of the library.' + NL;
out += 'struct StIcoPath { const char* d; bool fill; };' + NL + 'struct StIco { const char* key; float vx, vy, vw; std::vector<StIcoPath> paths; };' + NL;
out += 'static const std::vector<StIco>& stIcons() {' + NL + '    static const std::vector<StIco> k = {' + NL;
keys.forEach(function(key){
  var e = exportIcon(key);
  out += '        {' + Q + key + Q + ', ' + e.vb.map(function(v){ return Number(v).toFixed(1) + 'f'; }).join(', ') + ', {' + e.paths.map(function(p){ return '{' + Q + p[0].replace(/\s+/g,' ').trim() + Q + ', ' + (p[1] ? 'true' : 'false') + '}'; }).join(', ') + '}},' + NL;
});
out += '    };' + NL + '    return k;' + NL + '}' + NL;
require('fs').writeFileSync(require('path').join(__dirname, '..', '..', 'src', 'brickit', 'BrickStudioIcons.inl'), out);
console.log(keys.length + ' pictograms');

