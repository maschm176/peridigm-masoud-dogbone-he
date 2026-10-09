#!/usr/bin/env bash
# Stage 1 of the HE_30 staged calibration: Voce hardening only, damage disabled.
# STRESS_ONLY tells compute_objective.py to drop the fracture-strain term --
# with damage off, that term can only be satisfied by making necking-driven
# softening (not real fracture) arrive at the right strain, which fights a
# clean hardening-only fit. See compute_objective.py's docstring.
set -euo pipefail
export CONCENTRATION=30
export RUN_LABEL=HE30_stage1_voce
export STRESS_ONLY=1
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
