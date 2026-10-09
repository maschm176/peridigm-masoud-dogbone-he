#!/usr/bin/env bash
# Stage 2 of the HE_0 Voce-under-Hypoelastic+Johnson-Cook calibration:
# Johnson Cook damage only (Damage Parameter 1/2/3), hardening frozen at
# the Stage 1 result (see dakota_HE0_voce_hypoelastic_stage2_damage.in).
# ThresholdDamage is ALSO frozen (as continuous_state, currently 0.5)
# rather than searched. Three narrower attempts (0.97, 0.85, 0.7) all
# crashed with "Too many iterations in the plastic radial return" at
# various evals -- the near-simultaneous loss of strength across many
# bonds at once was abrupt enough to push the Voce plasticity model's
# local return-mapping iteration outside its convergence region, and 0.7's
# best result also showed the failure-oscillation starting visually before
# the detected fracture point. Back to the original wide-ramp default
# (0.5) -- the same value every power-law run used without ever hitting
# this crash, and a gradual ramp should also produce the cleaner
# drop-then-fail-then-oscillate shape seen in those plots.
# STRESS_ONLY is NOT set here -- this stage's whole point is fitting the
# real fracture strain via the damage model, so the fracture-strain term
# must stay active.
set -euo pipefail
export CONCENTRATION=0
export RUN_LABEL=HE0_voce_hypoelastic_stage2_damage
export INPUT_TEMPLATE=voce_hypoelastic_jc_input_template.yaml
export IMAGE=peridigm-masoud:voce-hypoelastic
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
