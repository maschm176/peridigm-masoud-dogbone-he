#!/usr/bin/env bash
# HE_30 D2 sweep, narrowed to bracket between D2=0.4 (frac=15.06%, still
# a real fracture) and D2=0.2 (frac=5.68%, almost certainly a false-
# positive detector artifact like HE_0's D2=0.25 case) -- D1=0.02, D3=3.0
# fixed, ThresholdDamage=0.97, hardening frozen at HE_30's Stage 1 result.
set -euo pipefail
export CONCENTRATION=30
export RUN_LABEL=HE30_sfc3_d2_sweep2
export INPUT_TEMPLATE=voce_hypoelastic_jc_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
