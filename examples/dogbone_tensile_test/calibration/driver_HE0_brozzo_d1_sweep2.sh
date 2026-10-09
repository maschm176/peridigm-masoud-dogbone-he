#!/usr/bin/env bash
# Tighter Brozzo d1 sweep for HE_0, bracketing between the best coarse
# result so far (d1=3.0, frac=16.86%, obj=0.133) and the crash point
# (d1=6.0, NaN in force evaluation) -- d2=0.95, ThresholdDamage=0.85 fixed.
# Separate RUN_LABEL from the coarse sweep so its archived curves
# (d1=1.5/3.0/12.0) aren't overwritten.
set -euo pipefail
export CONCENTRATION=0
export RUN_LABEL=HE0_brozzo_d1_sweep2
export INPUT_TEMPLATE=voce_hypoelastic_brozzo_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic-brozzo
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
