#!/usr/bin/env bash
# Stage 1 re-run of the HE_0 Hypoelastic+Johnson-Cook calibration, targeting
# the LOW hardening-exponent regime (see hypoelastic_lowexp_necking_test.yaml
# / hypoelastic_lowexp_jc_positive_d3_test.yaml): the original Stage 1
# (dakota_HE0_hypoelastic_stage1_hardening.in) converged to
# HardeningExponent=0.4405, which via Considere's criterion doesn't produce
# natural necking/softening until ~35% engineering strain -- far past the
# real ~15.37% fracture point -- forcing Johnson Cook damage to manufacture
# the entire peak-and-drop shape by itself, which pushed DamageParameter3
# non-physically negative and crashed (NaN overflow in
# exp(-DamageParameter3*triaxiality)) during Stage 2/3.
#
# This re-run searches HardeningExponent in a tight band around 0.28 (the
# Considere estimate for necking onset near 15.37%) instead of letting it
# roam back to the unconstrained ~0.44 optimum, while YieldStress and
# HardeningCharacteristicStrain stay free to find the best achievable
# pre-peak fit within that constraint. Damage stays disabled
# (DamageParameter1=5.0, ThresholdDamage=0.9) and STRESS_ONLY scores the
# full real curve shape, same pattern as the original Stage 1.
set -euo pipefail
export CONCENTRATION=0
export RUN_LABEL=HE0_hypoelastic_stage1_lowexp_hardening
export INPUT_TEMPLATE=hypoelastic_jc_input_template.yaml
export STRESS_ONLY=1
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
