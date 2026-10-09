#!/usr/bin/env bash
# Direct test of the "safer D2, higher D3" hypothesis for HE_30: instead of
# pushing D2 down toward the ~0.3 collapse cliff (D3=3.0 fixed), keep D2 in
# a safe range (0.8, 1.0) and increase D3 to 4.0 to suppress the ceiling
# term via a stronger triaxiality dependence instead. D1=0.02 fixed,
# ThresholdDamage=0.97, hardening frozen at HE_30's Stage 1 result.
set -euo pipefail
export CONCENTRATION=30
export RUN_LABEL=HE30_sfc3_d2d3_check
export INPUT_TEMPLATE=voce_hypoelastic_jc_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
