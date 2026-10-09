#!/usr/bin/env bash
# HE_30 Stage 2, SFC3-style: proper Dakota search (not a manual sweep) over
# D1/D2/D3 jointly, narrow bounds informed by the D2 sweeps already done
# (dakota_HE30_sfc3_d2_sweep*.in). D2 below ~0.3 causes a non-physical
# specimen collapse (strain plateaus ~7-9%, never reaches a genuine later
# fracture); D2=0.35 (D1=0.02, D3=3.0 fixed) gives a well-behaved fracture
# at 14.59% vs real 13.27%. Opening D1 and D3 too, since HE_30's own Stage 1
# hardening differs from HE_0's (higher InitialYieldStress, same
# Saturated/Exponential/Linear), meaning the same D1/D3 don't necessarily
# produce the same triaxiality-vs-strain relationship in HE_30's neck as in
# HE_0's -- forcing D1/D3 to stay fixed at HE_0's exact values may be
# preventing a better fit. ThresholdDamage=0.97 fixed, hardening frozen at
# HE_30's own Stage 1 result.
set -euo pipefail
export CONCENTRATION=30
export RUN_LABEL=HE30_voce_hypoelastic_stage2_sfc3style
export INPUT_TEMPLATE=voce_hypoelastic_jc_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
