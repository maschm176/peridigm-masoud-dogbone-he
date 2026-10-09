#!/usr/bin/env python

import sys

outFile = sys.argv[1]

obj_fun = 0.0

scale_factor = 1.0 # if interested to give nonuniform weight to the tests

with open("Tests/Objective_function_longitudinal.txt", "r") as text_file:
    line = text_file.readline()
    obj_fun += scale_factor * float(line.strip().split(" ")[0])

with open("Tests/Objective_function_notched.txt", "r") as text_file:
    line = text_file.readline()
    obj_fun += float(line.strip().split(" ")[0])

# with open("notched/Objective_function_notched.txt", "r") as text_file:
    # line = text_file.readline()
    # obj_fun += float(line.strip().split(" ")[0])/5.0

with open(outFile, "w") as text_file:
    text_file.write(str(obj_fun))
