#!/usr/bin/env bash
# Joint d1/d2 polish search for the Brozzo damage model on HE_0, under
# Influence Function: Parabolic Decay (instead of Peridigm's default
# constant "One"). Warm-started at the one point already evaluated under
# this influence function (d1=3.5, d2=0.95, obj=0.120) -- worse than the
# constant-weight best (obj=0.0715) at the same d1/d2, since Parabolic
# Decay changes the mechanical response near the softening transition
# enough that d1/d2 tuned for "One" don't directly transfer. Hardening and
# ThresholdDamage stay fixed: the two influence functions track almost
# identically through the whole elastic/hardening region, only diverging
# near fracture, so only the damage parameters should need retuning.
set -euo pipefail
export CONCENTRATION=0
export RUN_LABEL=HE0_brozzo_parabolicdecay_d1d2_polish
export INPUT_TEMPLATE=voce_hypoelastic_brozzo_parabolicdecay_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic-brozzo
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
