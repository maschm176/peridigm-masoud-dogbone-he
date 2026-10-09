#!/usr/bin/env bash
# Test of the new Bond-Level Artificial Viscosity damping model at HE_0's
# locked-in Brozzo parameters, rigid-grip BC, fast rate (220 cm/s). Scans
# ViscosityCoefficient across three orders of magnitude (1e4, 1e5, 1e6,
# CGS units -- a rough a priori estimate put a "reasonable" starting force
# magnitude around c~1e5 for typical post-fracture bond velocities, see
# voce_hypoelastic_brozzo_rigidgrip_artificialviscosity_input_template.yaml)
# to find whether this quadratic (self-limiting) damping form has a usable
# window, unlike the earlier linear Bond-Level Micro-Damping attempt.
set -euo pipefail
export CONCENTRATION=0
export RUN_LABEL=HE0_brozzo_rigidgrip_artificialviscosity_test
export INPUT_TEMPLATE=voce_hypoelastic_brozzo_rigidgrip_artificialviscosity_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic-brozzo
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
