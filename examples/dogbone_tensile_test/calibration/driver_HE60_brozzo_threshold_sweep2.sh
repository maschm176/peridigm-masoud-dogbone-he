#!/usr/bin/env bash
# Finer follow-up to the first HE_60 ThresholdDamage sweep (0.3/0.2/0.1),
# which bracketed the real 7.68% fracture target between 0.1 (5.49%) and
# 0.2 (8.09%). Separate RUN_LABEL from the first sweep so archived curves
# don't collide (file_tag numbering restarts at 1 per Dakota run).
set -euo pipefail
export CONCENTRATION=60
export RUN_LABEL=HE60_brozzo_threshold_sweep2
export INPUT_TEMPLATE=voce_hypoelastic_brozzo_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic-brozzo
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
