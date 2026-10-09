#!/bin/bash
#----------------------------------------------------
# Example SLURM job script to run MPI applications on 
# TACC's Stampede system.
#
# $Id: job.mpi 1580 201test1-01-08 04:10:50Z karl $
#----------------------------------------------------

#SBATCH -A Sandia-Fracture-Chal # Project name
#SBATCH -J hardening            # Job name
#SBATCH -o RR.o%j               # Output file (%j expands to jobId)
#SBATCH -p normal               # Queue name
#SBATCH -N 16                   # Total number of nodes requested (16 cores/node)
#SBATCH -n 1088                 # Total number of mpi tasks requested (16 task/node)
#SBATCH -t 48:00:00             # Run time (hh:mm:ss) - 1.5 hours

module load dakota
dakota -i calibration_test_dakota.in -o calibration_test_dakota.out > calibration_test_dakota.stdout
