#!/usr/bin/env bash
# Hard Cutoff test for HE_60, at the ALREADY-correctly-timed wide-ramp point
# (d1=3.5, ThresholdDamage=0.19, obj=0.0540, fracture_strain=7.80% vs real
# 7.68%) -- NOT combined with a higher d1. Reasoning: the narrow-ramp
# attempt (raising both ThresholdDamage AND d1 together) triggered dynamic
# instability, because raising d1 speeds up damage accumulation everywhere
# in the specimen at once, causing many bonds to approach failure nearly
# simultaneously -- a large, abrupt, synchronized stiffness loss, which is
# what destabilizes the explicit solver (same mechanism as the earlier
# plasticity-solver crash saga and the Fracture Plane Cutoff experiment).
#
# Hard Cutoff sharpens the per-bond transition (step function: full
# strength to zero the instant a bond's own damage crosses threshold,
# instead of ramping) WITHOUT touching d1 or the overall accumulation rate
# -- so bonds should still fail in their natural, staggered, spatially
# heterogeneous order (preserving stability), just each one snapping
# cleanly instead of degrading gradually. Goal: compress the ~31.4%
# peak-to-fracture softening seen in the ramp version down toward real
# HE_60 data's near-zero (0.9%) softening, without reintroducing the
# instability seen when d1 was also raised.
#
# Uses the existing voce_hypoelastic_brozzo_hardcutoff_input_template.yaml
# from the HE_0 post-fracture-oscillation investigation (already generic/
# parameterized, Hard Cutoff + disabled flying-point freeze) -- no new
# template needed.
set -euo pipefail
export CONCENTRATION=60
export RUN_LABEL=HE60_brozzo_hardcutoff_test
export INPUT_TEMPLATE=voce_hypoelastic_brozzo_hardcutoff_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic-brozzo
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
