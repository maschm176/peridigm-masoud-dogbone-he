#!/usr/bin/env bash
# One-off forward solve of the HE_0 Voce-under-Hypoelastic+Johnson-Cook
# calibration's best-to-date parameters (Stage 2 attempt 8, ThresholdDamage
# 0.97, eval 4: obj=0.0537, fracture 15.62% vs real 15.37%). Unlike
# driver.sh, this does NOT delete its work directory on exit -- the whole
# point is to keep the full Exodus output (output.e.4.*) around so it can be
# opened in ParaView, to inspect what the nodes are actually doing during
# the post-fracture "scribbly/circular" oscillation.
set -euo pipefail

PARAMS_FILE=$1
RESULTS_FILE=$2
NUM_PROCS=4
IMAGE=peridigm-masoud:voce-hypoelastic

CALDIR=$(cd "$(dirname "$0")" && pwd)
DAKOTA_BIN=/Users/mschmitz/dakota/bin
WORK="$CALDIR/forward_runs/HE0_best_fit"
rm -rf "$WORK"
mkdir -p "$WORK"

python3 "$DAKOTA_BIN/dprepro" --output-format '%.6e' "$PARAMS_FILE" "$CALDIR/voce_hypoelastic_jc_input_template.yaml" "$WORK/input.yaml"

cp "$CALDIR/tensile_test.g" "$CALDIR/extract_stress_strain.py" "$WORK/"

docker run --rm --platform linux/amd64 -v "$WORK":/output "$IMAGE" bash -c "
set -e
cd /output
decomp -p $NUM_PROCS tensile_test.g
mpirun --allow-run-as-root -np $NUM_PROCS Peridigm input.yaml
python3 extract_stress_strain.py output.e.$NUM_PROCS.0 sim_curve.csv fracture_strain.txt
"

echo "1.0 obj_fn_1" > "$RESULTS_FILE"

echo "Forward solve complete. Exodus output retained in: $WORK"
