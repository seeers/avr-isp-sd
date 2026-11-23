// Millimeter. Layoutannahmen vor dem Druck am realen Aufbau pruefen.
part = "assembly"; // [assembly, base, lid]
length = 112;
width = 75;
height = 29;
wall = 2.6;
floor = 2.8;
lid = 2.8;
corner = 4;
closure = [[7,7], [105,7], [105,68], [7,68]];
oled_center = [55,38];
oled_hole_pitch = [59.5,29.5];
window = [42,39,33,20]; // Zentrum x/y, Breite, Hoehe
encoder = [74,39,8.4]; // Zentrum x/y, Durchmesser
buttons = [[74,52,5], [74,24,5]];
ports = [[19,9,11,8], [47,9,17,5]]; // Mitte y/z, Breite, Hoehe (linke Wand)
isp = [49,12,16,10]; // Mitte y/z, Breite, Hoehe (rechte Wand)
$fn = 48;

module outer(h) {
    linear_extrude(h)
        hull()
            for (x = [corner,length-corner], y = [corner,width-corner])
                translate([x,y]) circle(r=corner);
}

module bore(point, diameter, z, h) {
    translate([point[0],point[1],z]) cylinder(d=diameter,h=h);
}

module port(p, right=false) {
    translate([right ? length-wall-1 : -1,p[0]-p[2]/2,p[1]-p[3]/2])
        cube([wall+2,p[2],p[3]]);
}

module base() {
    difference() {
        union() {
            difference() {
                outer(height);
                translate([wall,wall,floor]) cube([length-2*wall,width-2*wall,height]);
            }
            for (p = closure) bore(p,9,floor-0.1,height-floor+0.1);
            // Flaechen fuer selbstklebende Platinenhalter.
            for (p = [[27,22],[27,51],[74,22],[74,51]])
                translate([p[0]-6,p[1]-6,floor-0.1]) cube([12,12,1.6]);
        }
        for (p = closure) bore(p,2.6,5,height);
        for (p = ports) port(p);
        port(isp,true);
    }
}

module face() {
    difference() {
        union() {
            outer(lid);
            for (dx = [-1,1], dy = [-1,1])
                bore([oled_center[0]+dx*oled_hole_pitch[0]/2,
                      oled_center[1]+dy*oled_hole_pitch[1]/2],6,lid-0.1,3.1);
        }
        for (p = closure) bore(p,3.4,-1,lid+2);
        translate([window[0]-window[2]/2,window[1]-window[3]/2,-1])
            cube([window[2],window[3],lid+2]);
        bore(encoder,encoder[2],-1,lid+2);
        for (p = buttons) bore(p,p[2],-1,lid+2);
        for (dx = [-1,1], dy = [-1,1])
            bore([oled_center[0]+dx*oled_hole_pitch[0]/2,
                  oled_center[1]+dy*oled_hole_pitch[1]/2],2,lid+0.5,4);
    }
}

if (part == "base") base();
if (part == "lid") face();
if (part == "assembly") {
    base();
    translate([0,width,height+lid]) mirror([0,1,0]) mirror([0,0,1]) face();
}
