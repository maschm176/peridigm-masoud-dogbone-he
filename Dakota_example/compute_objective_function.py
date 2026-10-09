#!/usr/bin/env python

import sys

outFile = sys.argv[1]

obj_fun = 0.0

with open("Tests/Objective_function_longitudinal.txt", "r") as text_file:
    line = text_file.readline()
    obj_fun += float(line.strip().split(" ")[0])

# If there are multiple calibration tests, they need to be run individually 
# with the tested material parameters, then their errors should be add up (or
# other strategies can be adopted).
# If there is a notched test, for example, I add the following:

# with open("Tests/Objective_function_notched.txt", "r") as text_file:
    # line = text_file.readline()
    # obj_fun += float(line.strip().split(" ")[0])

with open(outFile, "w") as text_file:
    text_file.write(str(obj_fun))
