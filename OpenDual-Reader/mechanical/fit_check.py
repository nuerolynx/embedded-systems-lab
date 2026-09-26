"""Reproducible dimension arithmetic; not a CAD interference solver.
Reads scalar dimensions and component-box calls from the OpenSCAD source.
No physical validation or manufacturing approval is implied.
"""
from pathlib import Path
import re
import math
import json

here=Path(__file__).resolve().parent
source=(here/'OpenDual-Reader-enclosure.scad').read_text(encoding='utf-8')
def scalar(name):
    match=re.search(r'\b'+re.escape(name)+r'\s*=\s*([\d.]+)\s*;',source)
    if not match: raise ValueError('Missing scalar '+name)
    return float(match.group(1))
W,H,D=(scalar(n) for n in ('W','H','D'))
wall,rear,face=(scalar(n) for n in ('wall','rear','face'))
pcb_w,pcb_h,pcb_t,pcb_z=(scalar(n) for n in ('pcb_w','pcb_h','pcb_t','pcb_z'))
pcb_top=pcb_z+pcb_t
roof=D-face
calls=re.findall(r'component_box\(\s*([\d.]+)\s*,\s*([\d.]+)\s*,\s*([\d.]+)\s*,\s*([\d.]+)\s*,\s*([\d.]+)\s*,',source)
names=['ESP32','LF','HF antenna','HF module','3.3 V regulator','5 V regulator']
if len(calls)!=len(names): raise ValueError('Component call count changed; update explicit names.')
boxes={name:tuple(map(float,call)) for name,call in zip(names,calls)}

def bbox(box):
    x,y,w,h,t=box
    return x-w/2,y-h/2,x+w/2,y+h/2
def point_rect_dist(p,b):
    x1,y1,x2,y2=bbox(b)
    return math.hypot(max(x1-p[0],0,p[0]-x2),max(y1-p[1],0,p[1]-y2))

mounts=[(103,99),(133,99),(103,169),(133,169)]
front_collisions=[]
for i,a in enumerate(names):
    ax1,ay1,ax2,ay2=bbox(boxes[a])
    for b in names[i+1:]:
        bx1,by1,bx2,by2=bbox(boxes[b])
        if min(ax2,bx2)>max(ax1,bx1) and min(ay2,by2)>max(ay1,by1):
            front_collisions.append([a,b])

results={
    'status':'FIT STUDY ONLY; MANUFACTURING TOLERANCE STACK UNRESOLVED',
    'verification':'Analytic dimensions and rectangular body projections only. Separate OpenSCAD 2021.01 front/back STL exports compiled on 2026-09-26; this script does not validate their solid geometry or assembly fit.',
    'external_mm':{'height':H,'width':W,'depth':D},
    'pcb_mm':{'height':pcb_h,'width':pcb_w,'thickness':pcb_t},
    'side_clearance_mm':round((W-2*wall-pcb_w)/2,3),
    'end_clearance_mm':round((H-2*wall-pcb_h)/2,3),
    'regulator_max_height_mm':boxes['3.3 V regulator'][4],
    'regulator_unpocketed_clearance_mm':round(roof-pcb_top-boxes['3.3 V regulator'][4],3),
    'regulator_clearance_with_0_4_pocket_mm':round(roof+0.4-pcb_top-boxes['3.3 V regulator'][4],3),
    'remaining_front_thickness_above_pocket_mm':round(face-0.4,3),
    'buzzer_nominal_clearance_with_0_5_pocket_mm':round(pcb_z-1.9-(rear-0.5),3),
    'remaining_rear_thickness_below_pocket_mm':round(rear-0.5,3),
    'LF_nominal_untrimmed_pin_clearance_mm':round(pcb_top-(9.9-6.0)-rear,3),
    'LF_worst_bound_untrimmed_pin_clearance_mm':round(pcb_top-(10.5-5.8)-rear,3),
    'LF_pin_clearance_after_1mm_protrusion_trim_mm':round(pcb_z-1.0-rear,3),
    'LF_HF_body_edge_gap_mm':round(bbox(boxes['LF'])[1]-bbox(boxes['HF antenna'])[3],3),
    'LF_HF_module_body_edge_gap_mm':round(bbox(boxes['HF module'])[1]-bbox(boxes['LF'])[3],3),
    'LF_nearest_mount_center_to_body_mm':round(min(point_rect_dist(p,boxes['LF']) for p in mounts),3),
    'LF_nearest_5_4mm_pillar_to_body_mm':round(min(point_rect_dist(p,boxes['LF']) for p in mounts)-2.7,3),
    'front_body_projection_collisions':front_collisions,
    'not_checked':['complete assembly solid-interference analysis','solder joints and headers','complete actual PCB population','RF keepout suitability','assembly tolerances','manufacturing variation','cable bend radius','strain relief','fastener thread strength','tamper actuator','thermal performance','sound level','physical fit'],
}
(here/'fit-check-results.json').write_text(json.dumps(results,indent=2)+'\n',encoding='utf-8')
print(json.dumps(results,indent=2))
