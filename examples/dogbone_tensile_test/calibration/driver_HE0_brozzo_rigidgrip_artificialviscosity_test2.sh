#!/usr/bin/env bash
# Follow-up, much higher coefficient scan for Bond-Level Artificial
# Viscosity (1e8, 1e10, 1e12) -- the first scan (1e4-1e6) showed no real
# effect. Separate RUN_LABEL from the first scan so results don't collide.
set -euo pipefail
export CONCENTRATION=0
export RUN_LABEL=HE0_brozzo_rigidgrip_artificialviscosity_test2
export INPUT_TEMPLATE=voce_hypoelastic_brozzo_rigidgrip_artificialviscosity_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic-brozzo
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
