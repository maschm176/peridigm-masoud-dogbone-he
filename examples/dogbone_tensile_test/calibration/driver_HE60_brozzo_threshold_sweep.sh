#!/usr/bin/env bash
# ThresholdDamage sweep for HE_60, d1=3.5 and d2=0.99 fixed (transferred
# unchanged from HE_0 to HE_30, expected to transfer again -- see CLAUDE.md
# "Reframing the original cross-curve hypothesis"). Hardening frozen at
# HE_60's own locked-in Stage 1 result (InitialYieldStress=9.77e9,
# SaturatedYieldStress=1.35e10, ExponentialConstant=22.0, LinearConstant=2.0e8).
#
# HE_60's real fracture strain (7.68%) is far below HE_30's (13.27%), which
# itself needed ThresholdDamage down to 0.51 from HE_0's 0.85. Bracketing
# wide and low (0.3, 0.2, 0.1) rather than fine increments near 0.5, since
# HE_60 is substantially more embrittled and the needed threshold is
# expected to be considerably lower -- not yet known whether a new crash
# boundary appears at these lower values for this concentration.
set -euo pipefail
export CONCENTRATION=60
export RUN_LABEL=HE60_brozzo_threshold_sweep
export INPUT_TEMPLATE=voce_hypoelastic_brozzo_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic-brozzo
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
