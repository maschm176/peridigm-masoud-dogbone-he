#!/usr/bin/env bash
# Narrowed D2 sweep around 1.0, the best point found in the first D2 sweep
# (D1=0.02, D3=3.0 fixed, ThresholdDamage=0.97): D2=1.0 gave fracture 15.60%
# vs real 15.37% (obj=0.0512), the closest match yet while preserving the
# SFC3 paper's smooth, spatially-localized softening shape. This sweep
# brackets more tightly around 1.0 to see if a nearby value does even
# better.
set -euo pipefail
export CONCENTRATION=0
export RUN_LABEL=HE0_sfc3_d2_sweep2
export INPUT_TEMPLATE=voce_hypoelastic_jc_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
