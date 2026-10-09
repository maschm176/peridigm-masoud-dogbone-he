#!/usr/bin/env bash
# D2 sweep diagnostic: holding DamageParameter1=0.02 (near the floor found
# in the prior D1 sweep -- below this D1 stopped mattering) and
# DamageParameter3=3.0 (the SFC3 paper's value, preserving localization
# sharpness) fixed, stepping DamageParameter2 down from the paper's 2.5.
# The D1 sweep saturated at ~17.45% fracture strain (vs real 15.37%),
# meaning D1 alone can't close the gap once it's negligible compared to
# D2*exp(-D3*triaxiality) at the neck's actual triaxiality -- scaling D2
# down should shrink that term directly. ThresholdDamage=0.97 and hardening
# are our own calibrated HE_0 Voce values.
set -euo pipefail
export CONCENTRATION=0
export RUN_LABEL=HE0_sfc3_d2_sweep
export INPUT_TEMPLATE=voce_hypoelastic_jc_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
