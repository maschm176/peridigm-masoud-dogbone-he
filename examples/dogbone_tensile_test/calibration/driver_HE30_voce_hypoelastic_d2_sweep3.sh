#!/usr/bin/env bash
# HE_30 D2 sweep, tight bracket around the collapse cliff found in
# d2_sweep2 (D2=0.3/0.25 collapse, D2=0.35 well-behaved, frac=14.59%,
# obj=0.133) and after the D3=4.0 hypothesis check (d2d3_check) showed
# raising D2/D3 together moves fracture the wrong direction (later, not
# earlier). Testing D2 = 0.33 (just below 0.35, to see how close to the
# cliff a valid result can get), 0.35, 0.37, 0.40, all at D3=3.0, D1=0.02,
# ThresholdDamage=0.97, hardening frozen at HE_30's Stage 1 result.
set -euo pipefail
export CONCENTRATION=30
export RUN_LABEL=HE30_sfc3_d2_sweep3
export INPUT_TEMPLATE=voce_hypoelastic_jc_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
