// OpenDual Reader — original mechanical proposal, revision A0.
// Editable OpenSCAD source. Units mm. THIS IS A FIT STUDY, NOT RELEASED CAD.
// Axes: x left-to-right, y top-to-bottom, z rear-to-front.
// Envelope: x 0..40.7, y 0..131.2, z 0..17.6.
// KiCad -> enclosure: [x - 100 + 2.35, y - 50 + 3.6].
// OpenSCAD 2021.01 compiled front/back STL exports on 2026-09-26.
// Native export does not establish assembly clearance, tolerance or physical fit.

$fn = 64;
view = "assembly"; // [assembly,exploded,front,back,pcb,section]
show_components = true;
show_rf_reserves = true;
show_pcb = true;
show_diffuser = true;
enable_acoustic_vent = false; // Rear piezo acoustic path requires measurement.
explosion = 18;

W = 40.7; H = 131.2; D = 17.6;
corner_r = 3.0;
wall = 1.8; rear = 2.0; face = 1.4;
pcb_w = 36.0; pcb_h = 124.0; pcb_t = 1.6;
pcb_x = (W-pcb_w)/2; pcb_y = (H-pcb_h)/2;
pcb_z = 4.0;
pcb_top = pcb_z+pcb_t;
roof_z = D-face;
boss_d = 5.4; screw_clearance = 2.2;
pilot_d = 1.7; // Print/tap trial required; not a verified thread specification.
head_d = 4.2; head_depth = 1.2; // Nominal countersink proxy; verify actual screw.
mounts = [[3,49],[33,49],[3,119],[33,119]];
wall_slots = [[W/2,25],[W/2,112]];
led_xy = [W/2,33.6];
cable_xy = [W/2,120.0];
vent_xy = [8.05,95.6];

function local_xy(kx,ky) = [kx-100+pcb_x,ky-50+pcb_y];

module rounded_rect(w,h,r) {
    translate([r,r]) offset(r=r) square([w-2*r,h-2*r]);
}
module rounded_box(w,h,t,r) {
    linear_extrude(height=t) rounded_rect(w,h,r);
}
module oval(w,h,t) {
    hull() for (sx=[-(w-h)/2,(w-h)/2])
        translate([sx,0,0]) cylinder(d=h,h=t);
}
module mount_locations() {
    for (m=mounts) translate([pcb_x+m[0],pcb_y+m[1],0]) children();
}
module pilot_holes(z0=0.6,ztop=4.1) {
    mount_locations() translate([0,0,z0]) cylinder(d=pilot_d,h=ztop-z0);
}
module rear_plate() {
    difference() {
        union() {
            rounded_box(W,H,rear,corner_r);
            // Thin locating lip: 0.2 mm radial fit gap to cover's inner wall.
            translate([2.0,2.0,rear-0.2]) linear_extrude(height=1.6)
                difference() {
                    rounded_rect(W-4.0,H-4.0,1.0);
                    translate([0.65,0.65]) rounded_rect(W-5.3,H-5.3,0.35);
                }
            mount_locations() translate([0,0,rear-0.1]) cylinder(d=boss_d,h=pcb_z-rear+0.1);
            // Cable-tie bridge, supported at both ends; within the PCB notch.
            for (xx=[W/2-5.5,W/2+4.0])
                translate([xx,123,1.9]) cube([1.5,4,6.1]);
            translate([W/2-5.5,123,7.0]) cube([11,4,1.0]);
        }
        pilot_holes();
        // 10 x 10 x 0.5 inside pocket: selected rear piezo body is 9 x 9 x 1.9.
        // Rear wall remains 1.5 mm; sound transmission is not yet tested.
        translate([3.05,90.6,1.5]) cube([10,10,0.51]);
        translate([cable_xy[0],cable_xy[1],-0.1]) oval(12,5,rear+0.2);
        // Provisional 3.5 x 7 mm wall mounting slots. Flush head requirement.
        for (s=wall_slots) translate([s[0],s[1],-0.1]) rotate([0,0,90]) oval(7,3.5,rear+0.2);
    }
}

module front_cover() {
    difference() {
        union() {
            difference() {
                translate([0,0,rear]) rounded_box(W,H,D-rear,corner_r);
                translate([wall,wall,rear-0.1])
                    rounded_box(W-2*wall,H-2*wall,roof_z-rear+0.1,corner_r-wall);
            }
            // Shared fasteners clamp the PCB against the rear bosses.
            mount_locations() translate([0,0,pcb_top]) cylinder(d=boss_d,h=roof_z-pcb_top+0.05);
        }
        mount_locations() {
            translate([0,0,pcb_top-0.1]) cylinder(d=screw_clearance,h=D-pcb_top+0.2);
            translate([0,0,D-head_depth]) cylinder(d1=screw_clearance,d2=head_d,h=head_depth+0.01);
        }
        translate([led_xy[0],led_xy[1],roof_z-0.1]) cylinder(d=4.0,h=face+0.2);
        // Maximum TSR body otherwise touches inner roof. Recesses add 0.4 mm
        // nominal clearance, leaving 1.0 mm front wall; tolerance check pending.
        for (kx=[108.55,125.55]) {
            rp=local_xy(kx,153.7);
            translate([rp[0]-7.1,rp[1]-5.05,roof_z-0.01]) cube([14.2,10.1,0.41]);
        }
        if (enable_acoustic_vent)
            for (dx=[-1.2,0,1.2]) translate([vent_xy[0]+dx,vent_xy[1],roof_z-0.1]) cylinder(d=0.6,h=face+0.2);
    }
}

module pcb_model() {
    difference() {
        translate([pcb_x,pcb_y,pcb_z]) cube([pcb_w,pcb_h,pcb_t]);
        // Agreed 12 x 10 mm cable-routing notch at PCB bottom center.
        translate([pcb_x+12,pcb_y+114,pcb_z-0.1]) cube([12,10.1,pcb_t+0.2]);
        mount_locations() translate([0,0,pcb_z-0.1]) cylinder(d=2.2,h=pcb_t+0.2);
    }
}
module component_box(kx,ky,w,h,t,c) {
    p=local_xy(kx,ky);
    color(c) translate([p[0]-w/2,p[1]-h/2,pcb_top]) cube([w,h,t]);
}
module components() {
    component_box(118,62.75,18,25.5,3.3,[0.65,0.68,0.71]); // ESP32 body proxy.
    component_box(118,116,27.1,25.9,6.6,[0.15,0.15,0.16]); // LF maximum body envelope.
    component_box(118,88,25,10,2,[0.6,0.26,0.12]); // HF antenna thickness UNKNOWN: 2 mm placeholder.
    component_box(118,138,25,16.4,4.4,[0.15,0.35,0.65]); // HF electronics body proxy.
    // TSR1 maximum body: nominal 11.7 x 7.5 x 10.1 plus 0.5 mm tolerance.
    // Local roof pockets give 0.4 mm headroom before all remaining tolerances.
    // XY depth 8.1 is a conservative footprint-envelope bound (datasheet max8.0).
    component_box(108.55,153.7,12.2,8.1,10.6,[0.17,0.19,0.22]);
    component_box(125.55,153.7,12.2,8.1,10.6,[0.17,0.19,0.22]);
    // Rear Murata PKMCS0909E4000-R1 nominal body: pocket gives 0.6 mm gap.
    bp=local_xy(105.7,142);
    color([0.75,0.65,0.42]) translate([bp[0]-4.5,bp[1]-4.5,pcb_z-1.9]) cube([9,9,1.9]);
}
module rf_reserves() {
    // Placement reservations, not manufacturer-validated RF keepout volumes.
    // Actual ESP32 antenna keepout must be taken from the exact footprint guide.
    color([0.95,0.4,0.1,0.16]) translate([pcb_x,pcb_y,pcb_top]) cube([36,10,roof_z-pcb_top]);
    p_hf=local_xy(118,88);
    color([0.95,0.4,0.1,0.16]) translate([p_hf[0]-15.5,p_hf[1]-8,pcb_z-1]) cube([31,16,roof_z-pcb_z+1]);
    p_lf=local_xy(118,116);
    color([0.95,0.4,0.1,0.16]) translate([p_lf[0]-16.55,p_lf[1]-15.95,pcb_z-1]) cube([33.1,31.9,roof_z-pcb_z+1]);
}
module diffuser() {
    // Original flush stepped diffuser proxy; optical design and adhesion pending.
    translate([led_xy[0],led_xy[1],roof_z]) cylinder(d=3.8,h=face);
    translate([led_xy[0],led_xy[1],roof_z-0.7]) cylinder(d=5.5,h=0.7);
}
module assembly(explode=0) {
    color([0.22,0.24,0.27]) rear_plate();
    if (show_pcb) color([0.03,0.33,0.23]) pcb_model();
    if (show_components) components();
    if (show_rf_reserves) rf_reserves();
    translate([0,0,explode]) {
        color([0.6,0.63,0.67,0.25]) front_cover();
        if (show_diffuser) color([0.8,0.9,1,0.7]) diffuser();
    }
}
if (view=="front") front_cover();
else if (view=="back") rear_plate();
else if (view=="pcb") pcb_model();
else if (view=="exploded") assembly(explosion);
else if (view=="section") intersection() { assembly(); translate([W/2,-1,-1]) cube([W,H+2,D+2]); }
else assembly();
