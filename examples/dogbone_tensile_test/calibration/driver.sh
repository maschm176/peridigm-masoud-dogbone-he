#!/usr/bin/env bash
# DAKOTA analysis driver for the dogbone tensile test calibration
# (Isotropic Hardening Bond Associated Correspondence, 4-rank MPI).
# CONCENTRATION selects which calibration_*_data_HE_<CONCENTRATION>.txt files
# to score against (default "0"). RUN_LABEL names the curves/ archive
# subdirectory, so different concentrations/stages sharing this driver don't
# clobber each other's archived curves (default "HE<CONCENTRATION>").
# INPUT_TEMPLATE selects which dprepro template to use (default
# "input_template.yaml", the original Voce/bond-associated-correspondence
# template -- see "hypoelastic_jc_input_template.yaml" for the
# Hypoelastic + power-law + Johnson Cook variant, and
# "voce_hypoelastic_jc_input_template.yaml" for the Hypoelastic + Voce +
# Johnson Cook variant). IMAGE selects the Docker image (default
# "peridigm-masoud:latest"; the Voce-under-Hypoelastic dispatch only exists
# in "peridigm-masoud:voce-hypoelastic" -- see CLAUDE.md). All are normally
# set by a per-run wrapper script (see driver_HE*.sh) rather than exported
# by hand.
set -euo pipefail

PARAMS_FILE=$1
RESULTS_FILE=$2
CONCENTRATION=${CONCENTRATION:-0}
RUN_LABEL=${RUN_LABEL:-HE${CONCENTRATION}}
INPUT_TEMPLATE=${INPUT_TEMPLATE:-input_template.yaml}
IMAGE=${IMAGE:-peridigm-masoud:latest}

CALDIR=$(cd "$(dirname "$0")" && pwd)
DAKOTA_BIN=/Users/mschmitz/dakota/bin
NUM_PROCS=4

WORK=$(mktemp -d "${CALDIR}/work.XXXXXX")
trap 'rm -rf "$WORK"' EXIT

python3 "$DAKOTA_BIN/dprepro" --output-format '%.6e' "$PARAMS_FILE" "$CALDIR/$INPUT_TEMPLATE" "$WORK/input.yaml"

cp "$CALDIR/tensile_test.g" "$CALDIR/extract_stress_strain.py" "$WORK/"

docker run --rm --platform linux/amd64 -v "$WORK":/output "$IMAGE" bash -c "
set -e
cd /output
decomp -p $NUM_PROCS tensile_test.g
mpirun --allow-run-as-root -np $NUM_PROCS Peridigm input.yaml
python3 extract_stress_strain.py output.e.$NUM_PROCS.0 sim_curve.csv fracture_strain.txt
"

python3 "$CALDIR/compute_objective.py" "$WORK/sim_curve.csv" "$WORK/fracture_strain.txt" "$CALDIR" "$RESULTS_FILE" "$CONCENTRATION"

mkdir -p "$CALDIR/curves/$RUN_LABEL"
TAG=$(basename "$PARAMS_FILE" | sed 's/^params\.in//')
cp "$WORK/sim_curve.csv" "$CALDIR/curves/$RUN_LABEL/sim_curve${TAG}.csv"
cp "$WORK/fracture_strain.txt" "$CALDIR/curves/$RUN_LABEL/fracture_strain${TAG}.txt"
