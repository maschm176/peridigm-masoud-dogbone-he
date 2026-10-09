#! /usr/bin/env python
# Coarse cylindrical mesh + tensile-test node sets, adapted from
# examples/fragmenting_cylinder/fragmenting_cylinder_generate_mesh.py.
# Instead of the original single "all nodes" set (used there for an initial
# radial-velocity fragmentation load), this writes separate top/bottom
# node sets (bands near each end, by z-coordinate) for a prescribed-
# displacement tensile pull along the cylinder axis (z).

import math
import random

if __name__ == "__main__":

    # Cylinder dimensions (same physical size as fragmenting_cylinder)
    cylinder_height = 0.100
    cylinder_outer_radius = 0.025
    cylinder_inner_radius = 0.020

    # Coarse: 2 cells through the wall thickness (vs. 5 in the original)
    num_points_radius = 2

    element_size = (cylinder_outer_radius - cylinder_inner_radius)/num_points_radius
    num_points_circumference = int((2.0 * math.pi * (cylinder_outer_radius + cylinder_inner_radius)/2.0)/element_size + 0.5)
    num_points_height = int(cylinder_height/element_size + 0.5)
    arc_length = 2.0*math.pi/num_points_circumference

    print("\nGenerating coarse cylinder mesh:")
    print("  element size: {}".format(element_size))
    print("  number of points through wall thickness: {}".format(num_points_radius))
    print("  number of points around circumference: {}".format(num_points_circumference))
    print("  number of points along cylinder length: {}".format(num_points_height))
    print("  total number of points: {}".format(num_points_radius*num_points_circumference*num_points_height))

    data = []
    for k in range(num_points_height):
        for j in range(num_points_circumference):
            for i in range(num_points_radius):

                elem_inner_radius = cylinder_inner_radius + i*(cylinder_outer_radius - cylinder_inner_radius)/num_points_radius
                elem_outer_radius = cylinder_inner_radius + (i+1)*(cylinder_outer_radius - cylinder_inner_radius)/num_points_radius
                alpha = arc_length/2.0
                large_circular_sector_centroid_distance = 2.0*elem_outer_radius*math.sin(alpha)/(3.0*alpha)
                small_circular_sector_centroid_distance = 2.0*elem_inner_radius*math.sin(alpha)/(3.0*alpha)
                large_circular_sector_area = alpha*elem_outer_radius*elem_outer_radius
                small_circular_sector_area = alpha*elem_inner_radius*elem_inner_radius
                elem_area = large_circular_sector_area - small_circular_sector_area
                elem_centroid_distance = (large_circular_sector_centroid_distance*large_circular_sector_area - small_circular_sector_centroid_distance*small_circular_sector_area)/elem_area
                elem_centroid_angle = (j + 0.5)*arc_length
                elem_height = (k + 0.5)*cylinder_height/num_points_height
                elem_vol = elem_area*cylinder_height/num_points_height

                x = elem_centroid_distance * math.cos(elem_centroid_angle)
                y = elem_centroid_distance * math.sin(elem_centroid_angle)
                z = elem_height
                block_id = 1
                data.append([x, y, z, block_id, elem_vol])

    # Perturb node locations randomly to break symmetry (same as original)
    magnitude = 0.001 * element_size
    random.seed(42)
    for i in range(len(data)):
        data[i][0] += (2.0*random.random() - 1.0) * magnitude
        data[i][1] += (2.0*random.random() - 1.0) * magnitude
        data[i][2] += (2.0*random.random() - 1.0) * magnitude

    with open("cylinder_tensile_test.txt", 'w') as mesh_file:
        for datum in data:
            mesh_file.write(str(datum[0]) + " " + str(datum[1]) + " " + str(datum[2]) + " " + str(datum[3]) + " " + str(datum[4]) + "\n")

    # Tensile-test node sets: bands within one element height of each end.
    band = 1.0*cylinder_height/num_points_height
    bottom_ids = [i+1 for i, d in enumerate(data) if d[2] < band]
    top_ids = [i+1 for i, d in enumerate(data) if d[2] > cylinder_height - band]

    with open("bottom_nodeset.txt", 'w') as f:
        for node_id in bottom_ids:
            f.write(str(node_id) + "\n")
    with open("top_nodeset.txt", 'w') as f:
        for node_id in top_ids:
            f.write(str(node_id) + "\n")

    print("\nDiscretization written to cylinder_tensile_test.txt")
    print("Bottom node set ({} nodes) written to bottom_nodeset.txt".format(len(bottom_ids)))
    print("Top node set ({} nodes) written to top_nodeset.txt".format(len(top_ids)))
