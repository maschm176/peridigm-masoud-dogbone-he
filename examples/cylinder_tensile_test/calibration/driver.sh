#!/usr/bin/env bash
# DAKOTA analysis driver for the coarse cylinder tensile test.
set -euo pipefail

PARAMS_FILE=$1
RESULTS_FILE=$2

CALDIR=$(cd "$(dirname "$0")" && pwd)
DAKOTA_BIN=/Users/mschmitz/dakota/bin
IMAGE=peridigm-masoud:latest

WORK=$(mktemp -d "${CALDIR}/work.XXXXXX")
trap 'rm -rf "$WORK"' EXIT

python3 "$DAKOTA_BIN/dprepro" --output-format '%.6e' "$PARAMS_FILE" "$CALDIR/input_template.yaml" "$WORK/input.yaml"

cp "$CALDIR/cylinder_tensile_test.txt" "$CALDIR/bottom_nodeset.txt" "$CALDIR/top_nodeset.txt" "$CALDIR/extract_stress_strain.py" "$WORK/"

docker run --rm --platform linux/amd64 -v "$WORK":/output "$IMAGE" bash -c "
set -e
cd /output
Peridigm input.yaml
python3 extract_stress_strain.py output.e sim_curve.csv
"

python3 "$CALDIR/compute_objective.py" "$WORK/sim_curve.csv" "$CALDIR" "$RESULTS_FILE"
