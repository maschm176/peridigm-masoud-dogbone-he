#!/usr/bin/env bash
# Stage 2 of the HE_30 staged calibration: CriticalStretch only, Voce params
# frozen at the Stage 1 optimum (see dakota_HE30_stage2_criticalstretch.in).
set -euo pipefail
export CONCENTRATION=30
export RUN_LABEL=HE30_stage2_criticalstretch
exec "$(cd "$(dirname "$0")" && pwd)/driver.sh" "$@"
