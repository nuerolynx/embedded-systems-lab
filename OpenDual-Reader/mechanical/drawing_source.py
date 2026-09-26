"""Generate the dimensioned, editable SVG fit-study drawing (stdlib only).
This is a dimension-table illustration, not a render of the OpenSCAD solid.
Run with Python 3 from any directory. Outputs stay adjacent to this source.
"""
from pathlib import Path
import html

out = Path(__file__).resolve().parent
svg = []
def emit(s): svg.append(s)
def text(x,y,s,size=13,fill='#d8e6ed',weight='normal',anchor='start'):
    emit(f'<text x="{x}" y="{y}" fill="{fill}" font-family="Segoe UI,Arial,sans-serif" font-size="{size}" font-weight="{weight}" text-anchor="{anchor}">{html.escape(s)}</text>')
def rect(x,y,w,h,fill,stroke='none',radius=0,dash=None):
    emit(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{radius}" fill="{fill}" stroke="{stroke}" stroke-width="1.2"'+(f' stroke-dasharray="{dash}"' if dash else '')+'/>')
def line(x1,y1,x2,y2,stroke='#66848e',width=1,dash=None):
    emit(f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="{stroke}" stroke-width="{width}"'+(f' stroke-dasharray="{dash}"' if dash else '')+'/>')
def circle(x,y,r,fill,stroke='#f5be68',dash=None):
    emit(f'<circle cx="{x}" cy="{y}" r="{r}" fill="{fill}" stroke="{stroke}" stroke-width="1.2"'+(f' stroke-dasharray="{dash}"' if dash else '')+'/>')
def dim(x1,y1,x2,y2,label,tx,ty):
    line(x1,y1,x2,y2,'#b7c8ce')
    if y1==y2:
        for x in [x1,x2]: line(x,y1-4,x,y1+4,'#b7c8ce')
    else:
        for y in [y1,y2]: line(x1-4,y,x1+4,y,'#b7c8ce')
    text(tx,ty,label,13,'#e7f0f4',anchor='middle')

W,H,D=40.7,131.2,17.6
scale=4.0
emit('<svg xmlns="http://www.w3.org/2000/svg" width="1500" height="1110" viewBox="0 0 1500 1110">')
emit('<title>OpenDual Reader dimensioned mechanical fit study A0</title>')
emit('<desc>Original 131.2 by 40.7 by 17.6 millimetre enclosure proposal with notched PCB, component envelopes and unresolved mechanical fit constraints.</desc>')
rect(0,0,1500,1110,'#101b24')
text(45,47,'OPENDUAL / MECHANICAL FIT STUDY',27,'#edf5f7','bold')
text(46,75,'A0 · ORIGINAL DESIGN · ALL DIMENSIONS IN mm · NOT RELEASED FOR FABRICATION',14,'#ffc073')
text(46,103,'External envelope remains 131.2 H × 40.7 W × 17.6 D. Body proxies and clearances are nominal.',15)

# Front/envelope and board overlay.
x0,y0=104,173
def X(x): return x0+x*scale
def Y(y): return y0+y*scale
text(103,131,'ENCLOSURE + PCB / FRONT VIEW',14,'#a2c5d7','bold')
rect(X(0),Y(0),W*scale,H*scale,'#233644','#7792a0',12)
rect(X(1.8),Y(1.8),(W-3.6)*scale,(H-3.6)*scale,'#16232d','#485c67',5)
board_pts=[(2.35,3.6),(38.35,3.6),(38.35,127.6),(26.35,127.6),(26.35,117.6),(14.35,117.6),(14.35,127.6),(2.35,127.6)]
emit('<polygon points="'+' '.join(f'{X(x)},{Y(y)}' for x,y in board_pts)+'" fill="#17483e" stroke="#67b598" stroke-width="1.5"/>')
def body(kx,ky,w,h,color,label,small=None):
    lx=kx-100+2.35; ly=ky-50+3.6
    rect(X(lx-w/2),Y(ly-h/2),w*scale,h*scale,color,'#bfd0d6',2)
    text(X(lx),Y(ly)+1,label,11,'#ffffff','bold','middle')
    if small: text(X(lx),Y(ly)+16,small,10,'#e2e9ee',anchor='middle')
body(118,62.75,18,25.5,'#697581','ESP32','18 × 25.5')
rect(X(11.35),Y(3.6),18*scale,6.5*scale,'#998250','#cfb967',2)
text(X(20.35),Y(8.2),'2.4 GHz',10,'#fff7cc',anchor='middle')
body(118,88,25,10,'#774c31','HF FERRITE')
body(118,116,27.1,25.9,'#24282b','LF ID-12LA-HE','MAX 27.1 × 25.9')
body(118,138,25,16.4,'#315d82','PN532 MINI','25 × 16.4')
body(108.55,153.7,12.2,8.1,'#48515a','3.3 V')
body(125.55,153.7,12.2,8.1,'#48515a','5 V')
rect(X(3.55),Y(91.1),9*scale,9*scale,'none','#e7bc7f',2,'4 3')
circle(X(20.35),Y(33.6),2*scale,'#d8f5de','#d8f5de')
for hx in [5.35,35.35]:
    for hy in [52.6,122.6]:
        circle(X(hx),Y(hy),3*scale,'none','#e2aa64','3 3')
        circle(X(hx),Y(hy),1.1*scale,'#101b24','#e4cbaa')
rect(X(14.35),Y(117.5),12*scale,5*scale,'#c68d45','#f7c682',10)
text(X(20.35),Y(125.3),'CABLE',9,'#fff0d0',anchor='middle')
dim(X(0),Y(-4),X(W),Y(-4),'40.7',X(W/2),Y(-6))
dim(X(-8),Y(0),X(-8),Y(H),'131.2',X(-13),Y(H/2))
dim(X(0),Y(H+6),X(W),Y(H+6),'External',X(W/2),Y(H+12))
line(X(W),Y(33.6),336,Y(33.6),'#b8d7c2')
text(345,Y(33.6)+4,'Ø4 diffuser / LED at PCB (118,80)',13)
line(X(W),Y(43.6),336,Y(43.6),'#c98b5b')
text(345,Y(43.6)+4,'25 × 10 antenna; thickness unverified',13)
line(X(W),Y(53.3),336,Y(53.3),'#e8b76e')
text(345,Y(53.3)+4,'4 × Ø2.2 NPTH, Ø6 component reserve',13)
line(X(W),Y(71),336,Y(71),'#9eaeb6')
text(345,Y(71)+4,'LF maximum body 6.6 high',13)
line(X(W),Y(108.6),336,Y(108.6),'#cbbcaf')
text(345,Y(108.6)+4,'MAX regulator 10.6 → 0.4 pocket clearance',13,'#ffb771')
line(X(W),Y(122),336,Y(122),'#e8b76e')
text(345,Y(122)+4,'12 × 10 PCB notch + rear cable exit',13)

# Side thickness section schematic, not a cut through every component.
sx,sy=783,174
text(sx,141,'DEPTH STACK / SCHEMATIC SECTION',14,'#a2c5d7','bold')
sz=10
depth_y=265
rect(sx,depth_y,17.6*sz,200,'#233644','#7792a0')
rect(sx,depth_y,2*sz,200,'#516575','#9aacb5')
rect(sx+4*sz,depth_y,1.6*sz,200,'#277a65','#7fc4a7')
rect(sx+16.2*sz,depth_y,1.4*sz,200,'#516575','#9aacb5')
rect(sx+16.2*sz,depth_y+28,0.4*sz,74,'#16232d','#ffc073')
rect(sx+5.6*sz,depth_y+38,10.6*sz,54,'#53595e','#d7c8ac')
line(sx+16.4*sz,depth_y+50,sx+225,depth_y+50,'#ffb251')
text(sx+235,depth_y+54,'0.4 AFTER POCKET',14,'#ffbd68','bold')
text(sx+235,depth_y+76,'10.6 max regulator; roof locally 1.0',13)
text(sx+235,depth_y+98,'Remaining tolerance stack unresolved',13,'#ffc58f')
rect(sx+1.5*sz,depth_y+124,0.5*sz,50,'#16232d','#d3b47b')
rect(sx+2.1*sz,depth_y+133,1.9*sz,30,'#a99570','#d8c08f')
line(sx+4*sz,depth_y+148,sx+225,depth_y+148,'#d3b47b')
text(sx+235,depth_y+152,'Rear piezo: 0.6 nominal clearance',13)
text(sx+235,depth_y+174,'0.5 rear pocket; remaining wall 1.5',13)
dim(sx,depth_y-22,sx+17.6*sz,depth_y-22,'17.6 external depth',sx+88,depth_y-34)
for z,label,ty in [(0,'z0 rear',495),(2,'z2 inside rear',525),(4,'z4 PCB back',555),(5.6,'z5.6 component face',585),(16.2,'z16.2 inner roof',615),(17.6,'z17.6 front',645)]:
    line(sx+z*sz,depth_y+200,sx+z*sz,ty-7,'#5d7480',1,'3 3')
    line(sx+z*sz,ty-7,sx+211,ty-7,'#5d7480')
    text(sx+221,ty-3,label,13)
text(sx,194,'Rear → front; PCB thickness 1.6',13)
text(sx,217,'2.0 rear plate · 1.8 walls · 1.4 front face',13)

line(45,765,1455,765,'#375363')
text(45,800,'PCB OUTLINE / DATUMS',16,'#a2c5d7','bold')
text(45,827,'KiCad outline: (100,50) → (136,50) → (136,174) → (124,174)',14)
text(45,850,'→ (124,164) → (112,164) → (112,174) → (100,174) → close.',14)
text(45,884,'Holes: (103,99), (133,99), (103,169), (133,169).',14)
text(45,908,'PCB 36 × 124; body centered in enclosure at x2.35 / y3.60.',14)
text(45,932,'Clearance inside side walls: 0.55 each side; ends: 1.80 each end.',14)
text(45,966,'EDITABLE SOURCES',13,'#a2c5d7','bold')
text(45,991,'OpenDual-Reader-enclosure.scad · drawing_source.py · mechanical-fit.md',13)
text(45,1023,'Drawing generated from dimensions; not an OpenSCAD solid render.',13,'#99aebc')

text(783,800,'RELEASE BLOCKERS',16,'#ffc073','bold')
for yy,s in [
    (828,'• Roof pockets give 0.4 clearance; tolerance stack unresolved.'),
    (855,'• LF worst-case pins reach z0.9: 1.1 mm rear-plate conflict.'),
    (882,'• Cable jacket, bend radius, tie access and antenna lead routing.'),
    (909,'• LF/HF edge gap 10.05 mm is not RF validation.'),
    (936,'• Wall screw heads, buzzer acoustics, tamper actuator and sealing.'),
    (963,'• Exact ESP32 antenna rules; nearby metal and ferrite trials.'),
    (990,'• Native STL exports exist; physical assembly remains untested.')]:
    text(783,yy,s,13,'#efcfac')
text(783,1031,'DO NOT ORDER ENCLOSURES FROM THIS FIT STUDY.',14,'#ffba77','bold')
emit('</svg>')
(out/'dimensioned-layout.svg').write_text('\n'.join(svg),encoding='utf-8')
print(out/'dimensioned-layout.svg')
