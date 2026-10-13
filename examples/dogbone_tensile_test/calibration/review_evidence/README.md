# Review evidence: Connectivity Severance / Symmetric Severance Propagation

Small, derived result files pulled out of `forward_runs/<run>/` (which is
`.gitignore`'d because it also holds the raw Exodus `.e` output, often
>90MB per run). These are the only files from that directory worth
tracking in git — everything else is either regenerable from the YAML
templates + Docker image, or too large to usefully commit.

See `../CLAUDE.md` for the full narrative (root cause, prior failed
interventions, and how each run below fits in) and
`../../../src/damage/Peridigm_BrozzoDamageModel.cpp`/`.hpp` for the actual
mechanism implementations.

## Files

- `sim_curve_*.csv` — columns `strain,stress`, one row per output frame.
  Extracted by `../extract_stress_strain.py` from each run's raw Exodus
  output.
- `fracture_strain_*.txt` — the single detected fracture-strain value
  (percent engineering strain) for that run, from the same script.
- `flying_point_count_per_frame_*.csv` — the `Flying_Point_Flag` field
  summed across all 4 MPI ranks, per output frame (regenerated directly
  from each run's `output.e.4.{0,1,2,3}` via netCDF4, not re-derived from
  the CSVs above).

## Which run is which

| File prefix | What it is |
|---|---|
| `connectivity_diagnostic_baseline_validation` | Connectivity Severance metric validation run (threshold fixed at an unreachable 0.99, so this measures the raw `Connectivity_Asymmetry` field without ever triggering severance) — confirms the baseline-corrected metric reads 0 at t=0 and climbs to ~0.49 post-fracture. |
| `connectivity_severance_threshold0.20` | Connectivity Severance actually enabled, no propagation fix — the one-sided-bookkeeping version. 6 points go flying, frozen immediately with no cascade (see the matching `flying_point_count_per_frame_*` file). |
| `symmetric_severance_instant_threshold0.20` | Stage 1 (Symmetric Severance Propagation) at the same threshold, instant per-step mirror-kill — the avalanche run: 2,426 points go flying within ~20 output frames, then frozen. Note: this run's strain axis is corrupted past fracture onset (a tracked gage node gets swept into the avalanche and free-falls) — don't trust `stress` values vs. `strain` past the fracture point in this file; use frame index / the flying-point-count file instead for anything post-fracture. |
| `symmetric_severance_ratelimited0.02_threshold0.20` | Same as above, but the per-bond mirror-kill is throttled to 0.02/step (~50 steps to fully die) instead of instant. Delays the avalanche's onset by ~15 frames but lands at the identical final count (2,426) — same strain-axis corruption caveat applies. |

The fourth variant discussed (rate-limiting Bond Cleanup too,
`ratelimited2`) crashed with a NaN force evaluation before producing any
output — there's no result file for it, which is itself the finding (see
`CLAUDE.md`).
