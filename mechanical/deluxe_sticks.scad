$fn = 80;

bottom_height = 2;
main_height = 14;

clearance_diam = 5;
bottom_diam = 5;
top_diam = 12;
ball_diam = top_diam;
top_height = 12 - (ball_diam / 2);

stem_width = 1.85;
stem_thickness = 1.15;
stem_depth = 5;
stem_slop = 1.05;

module stick() {
difference() {
    union() {
        cylinder(bottom_height, d1= clearance_diam, d2 = bottom_diam);
        translate([0, 0, bottom_height])
            cylinder(main_height, d1 = bottom_diam, d2 = top_diam);
        translate([0, 0, main_height + bottom_height]) {
            cylinder(top_height, d = top_diam);
        }
        translate([0, 0, main_height + top_height + bottom_height]) {
            sphere(d = ball_diam);
        }
    }
    
    union() {
        translate([0, 0, stem_depth / 2]) 
            cube([stem_width * stem_slop, stem_thickness * stem_slop, stem_depth * 1.05], center = true);
    }
} 
}

$start_radius = 0.5;
$tip_radius = 0.2;
$tip_height = 1;
$full_support_height = 0.42;
$debug_supports = false;
$base_radius = $debug_supports ? $start_radius : $start_radius * 6.5;



module one_support(position, height) {
    transition_point = height - $tip_height;
    color([1, 0, 0]) union() {
        translate(position) {
            cylinder(0.20, $base_radius, $base_radius, false);
            cylinder(transition_point, $start_radius, $start_radius, false);
            translate([0, 0, transition_point]) {
                cylinder($tip_height, $start_radius, $tip_radius, false);
            }
        }
    }
}

support_height = 3;
s_off = (bottom_diam / 2) - ($tip_radius * 2);

stick();

//union() {
//    stick();
//    translate([0, 0, -support_height]) {
//        for (x = [-1.35, 1.35]) {
//            for (y = [-1.35, 1.35]) {
//                one_support([x, y], support_height);
//            }
//        }
//        one_support([0, s_off], support_height);
//        one_support([0, -s_off], support_height);
//        one_support([s_off, 0], support_height);
//        one_support([-s_off, 0], support_height);
//    }
//}