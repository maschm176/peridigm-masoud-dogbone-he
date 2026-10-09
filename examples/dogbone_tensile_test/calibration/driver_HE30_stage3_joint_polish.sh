#!/usr/bin/env bash
# Stage 3 of the HE_30 staged calibration: short joint 5-param polish pass
# around the Stage 1 + Stage 2 result (see dakota_HE30_stage3_joint_polish.in).
set -euo pipefail
export CONCENTRATION=30
export RUN_LABEL=HE30_stage3_joint_polish
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
