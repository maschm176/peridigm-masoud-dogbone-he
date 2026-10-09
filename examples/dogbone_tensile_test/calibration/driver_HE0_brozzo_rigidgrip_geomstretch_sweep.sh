#!/usr/bin/env bash
# Geometric Critical Stretch sweep for HE_0, rigid-grip BC, locked-in Brozzo
# d1/d2/ThresholdDamage (3.5/0.99/0.85) and Voce hardening. Maps the
# oscillation-amplitude vs fracture-timing tradeoff across the Geometric
# Stretch Cutoff threshold, bracketing the two single points already run
# (0.15: std 297.8 vs baseline 325.2, frac 13.87% vs real 15.37%; 0.30:
# essentially no effect, frac ~18.42% matching baseline). Requires the
# peridigm-masoud:voce-hypoelastic-brozzo-geomstretch image. Run serially
# (no evaluation_concurrency), per the established concurrency lesson.
set -euo pipefail

PARAMS_FILE=$1
RESULTS_FILE=$2
NUM_PROCS=4
IMAGE=peridigm-masoud:voce-hypoelastic-brozzo-geomstretch

CALDIR=$(cd "$(dirname "$0")" && pwd)
DAKOTA_BIN=/Users/mschmitz/dakota/bin
TAG=$(basename "$PARAMS_FILE" | sed 's/^params\.in//')
WORK="$CALDIR/forward_runs/HE0_brozzo_rigidgrip_geomstretch_sweep${TAG}"
rm -rf "$WORK"
mkdir -p "$WORK"

python3 "$DAKOTA_BIN/dprepro" --output-format '%.6e' "$PARAMS_FILE" "$CALDIR/voce_hypoelastic_brozzo_rigidgrip_geomstretch_input_template.yaml" "$WORK/input.yaml"

cp "$CALDIR/tensile_test.g" "$CALDIR/extract_stress_strain.py" "$WORK/"

docker run --rm --platform linux/amd64 -v "$WORK":/output "$IMAGE" bash -c "
set -e
cd /output
decomp -p $NUM_PROCS tensile_test.g
mpirun --allow-run-as-root -np $NUM_PROCS Peridigm input.yaml
python3 extract_stress_strain.py output.e.$NUM_PROCS.0 sim_curve.csv fracture_strain.txt
"

mkdir -p "$CALDIR/curves/HE0_brozzo_rigidgrip_geomstretch_sweep"
cp "$WORK/sim_curve.csv" "$CALDIR/curves/HE0_brozzo_rigidgrip_geomstretch_sweep/sim_curve${TAG}.csv"
cp "$WORK/fracture_strain.txt" "$CALDIR/curves/HE0_brozzo_rigidgrip_geomstretch_sweep/fracture_strain${TAG}.txt"
cp "$WORK/input.yaml" "$CALDIR/curves/HE0_brozzo_rigidgrip_geomstretch_sweep/input${TAG}.yaml"

echo "1.0 obj_fn_1" > "$RESULTS_FILE"

echo "Forward solve complete. Exodus output retained in: $WORK"
