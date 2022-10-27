//
// Quick & Dirty ESP-CAM mount for remote controlled car project
//

width = 45;
length = 15;
height = 2;

pillar_width = 10;
pillar_depth = 5;
pillar_height = 35;

screw_distance = 27;
screw_diameter = 3;

difference() {
    // mount parts
    union() {
        // Base
        cube([width, length, height]);
        
        // Pillar
        translate([(width/2) - (pillar_width/2), 0, 0]) {
            cube([pillar_width, pillar_depth, pillar_height]);
        }
    }
    
    // Screw mounting holes
    translate([(width - screw_distance)/2, length/2, -1]) {
        cylinder(h = height + 2, d = screw_diameter, $fn = 120);
    }
    
    translate([width - (width - screw_distance)/2, length/2, -1]) {
        cylinder(h = height + 2, d = screw_diameter, $fn = 120);
    }
    
    // Pillar cut-out (to save print time)
    translate([(width/2) - (pillar_width/2) + 2, 2, height]) {
        cube([pillar_width - 4, pillar_depth, pillar_height]);
    }
}