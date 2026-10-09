#!/usr/bin/env bash
# Tight check around D2=1.0 (D1=0.02, D3=3.0 fixed, ThresholdDamage=0.97):
# D2=1.0 gave fracture 15.60% (obj=0.0512), a clear outlier vs its
# neighbors 0.75/0.9/1.1/1.25 which all landed 16.3-17.6% -- checking
# whether there's a real narrow basin around 1.0 or whether it's an
# isolated fluke tied to a specific bond-breaking event's timing.
set -euo pipefail
export CONCENTRATION=0
export RUN_LABEL=HE0_sfc3_d2_sweep3
export INPUT_TEMPLATE=voce_hypoelastic_jc_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
