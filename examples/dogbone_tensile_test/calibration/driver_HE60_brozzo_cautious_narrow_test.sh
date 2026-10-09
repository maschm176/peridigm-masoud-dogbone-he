#!/usr/bin/env bash
# Cautious intermediate step after two failed attempts to sharpen HE_60's
# pre-fracture softening shape: (1) raising d1+threshold together to 6.4/0.35
# and (2) Hard Cutoff at the unchanged 3.5/0.19 -- both produced the same
# violent dynamic oscillation. Diagnosis: at a low ThresholdDamage, many
# bonds cluster near the trigger point at similar strains; the wide ramp's
# gradualness was accidentally acting as a numerical shock absorber for that
# cluster, and anything that sharpens the transition (bigger d1, or Hard
# Cutoff) removes that absorber and exposes a synchronized, destabilizing
# stiffness loss.
#
# This test takes a much smaller step in the same direction -- regular ramp
# (no Hard Cutoff), ThresholdDamage 0.19 -> 0.25 (+32%, vs the prior
# attempt's +84%), d1 scaled proportionally 3.5 -> 4.6 to roughly preserve
# fracture timing. Goal: find out whether a small narrowing is tolerable
# (partial win) or whether instability appears even here (stronger evidence
# the low-threshold/wide-ramp combo is a hard numerical constraint, not just
# an extreme-step artifact).
set -euo pipefail
export CONCENTRATION=60
export RUN_LABEL=HE60_brozzo_cautious_narrow_test
export INPUT_TEMPLATE=voce_hypoelastic_brozzo_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic-brozzo
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
