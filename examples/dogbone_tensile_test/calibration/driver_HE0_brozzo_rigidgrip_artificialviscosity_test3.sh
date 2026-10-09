#!/usr/bin/env bash
# Narrower bracket scan (1e7, 2e7, 4e7) between known-safe c=1e6 and
# known-crash c=1e8 for Bond-Level Artificial Viscosity.
set -euo pipefail
export CONCENTRATION=0
export RUN_LABEL=HE0_brozzo_rigidgrip_artificialviscosity_test3
export INPUT_TEMPLATE=voce_hypoelastic_brozzo_rigidgrip_artificialviscosity_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic-brozzo
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
