#!/usr/bin/env bash
# Tighter bracket scan (2e6, 4e6, 7e6) between known-negligible c=1e6 and
# known-crash c=1e7 for Bond-Level Artificial Viscosity.
set -euo pipefail
export CONCENTRATION=0
export RUN_LABEL=HE0_brozzo_rigidgrip_artificialviscosity_test4
export INPUT_TEMPLATE=voce_hypoelastic_brozzo_rigidgrip_artificialviscosity_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic-brozzo
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
