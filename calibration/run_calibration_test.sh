#!/bin/bash

#Directories to run
PROJECTS_DIR=$HOME'/projects/'
TRILINOS_BIN=$PROJECTS_DIR'trilinos/bin'
PERIDIGM_EXE=$PROJECTS_DIR'Peridigm/bin/Peridigm'
#PERIDIGM_EXE=$PROJECTS_DIR'Peridigm_Hypoelastic/bin/Peridigm'

#Number of processors
#NUM_PRC=$SLURM_NTASKS
NUM_PRC1=68
NUM_PRC2=1020

#Input arguments 
InputParameters=$1
OutputResult=$2

#Calibration tests directories
#DIR="longitudinal transverse notched"

cd Tests/

rm Obj*

# Create the input decks
cp ../$InputParameters .
./create_input_deck_tensile_test_longitudinal.py $InputParameters &
./create_input_deck_notched_test.py $InputParameters &
wait
#cp ../$InputParameters .
#ibrun -o 0 -n 1 ./create_input_deck_tensile_test_longitudinal.py $InputParameters &
#ibrun -o $NUM_PRC -n 1 ./create_input_deck_tensile_test_transverse.py $InputParameters &
#wait

rm $InputParameters

#Run Peridigm
ibrun -o 0 -n $NUM_PRC1 ${PERIDIGM_EXE} InputFileLongitudinal.xml &
ibrun -o $NUM_PRC1 -n $NUM_PRC2 ${PERIDIGM_EXE} InputFileNotched.xml &
wait

#cd Output/

#ibrun -o 0 -n 1 ${TRILINOS_BIN}/epu -p $NUM_PRC ExodusOutputLongitudinal &
#ibrun -o $NUM_PRC -n 1 ${TRILINOS_BIN}/epu -p $NUM_PRC ExodusOutputTransverse &
#wait

#rm *.e.*
#cd ..

#Post process the data
./post-process_longitudinal.py 
./post-process_notched.py 

cd ..

./compute_objective_function.py $OutputResult

