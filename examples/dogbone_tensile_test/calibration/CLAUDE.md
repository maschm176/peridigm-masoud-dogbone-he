# Dogbone Tensile Test Calibration — Peridigm + Dakota

## Goal
Calibrate a Peridigm peridynamics material model (Isotropic Hardening Bond-Associated
Correspondence model) against a real dogbone tensile test (`calibration_stress_data_HE_0.txt` /
`calibration_strain_data_HE_0.txt`, engineering stress/strain, real fracture at ~15.37% strain),
using Dakota (`coliny_pattern_search`) to optimize hardening-law and damage-model parameters. The
sim runs in a Docker container (`peridigm-masoud:latest`, 4-rank MPI) driven by `driver.sh`.

## Broader context
This is one of **three** stress-strain curves being calibrated this way (baseline material here;
two more to follow — see `~/Documents/Peridynamics-julia/PDHydrogenEmbritttlement` for the related
hydrogen-embrittlement work). The point of optimizing rather than hand-fitting is that these
parameters need to be **reproducible and comparable across curves** — in particular, `CriticalStretch`
is expected to be the dial that gets swept/recalibrated for the embrittlement study on pipes (more
embrittled material → lower critical stretch → earlier fracture), so getting this baseline
calibration procedure exactly right matters beyond just this one curve.

## Current status: fracture point successfully captured — THE OPTIMIZED RESULT

**These are the final optimized parameters for this baseline curve** (material model: Isotropic
Hardening Bond-Associated Correspondence with **Voce hardening law**; damage model: **Critical
Stretch**), from Dakota `coliny_pattern_search` jointly optimizing all 5 parameters together against
the full-curve + fracture-strain objective in `compute_objective.py`. Source: current `dakota.out`,
best of 26 evaluations, **eval 23**.

| Parameter | Model | Optimized value |
|---|---|---|
| InitialYieldStress | Voce hardening | 9.27e9 Pa (927 MPa) |
| SaturatedYieldStress | Voce hardening | 1.35e10 Pa (1350 MPa) |
| ExponentialConstant | Voce hardening | 22.0 |
| LinearConstant | Voce hardening | 2.0e8 Pa/unit-strain (200 MPa/unit-strain) |
| CriticalStretch | Critical Stretch damage | 0.167 |

**Objective trajectory:** started around 0.20–0.36 across the first several evaluations (see
`dakota.out` evals 2, 4, 6, 8...), converged down to **0.060 final** — a very strong fit across the
entire curve, including the failure point specifically targeted by the objective.

**Fit quality:** predicted fracture strain 15.31% vs. real 15.37% — essentially exact. Plot:
`final_best_fit_with_fracture.png` — full curve match is good through the plateau, fracture location
is essentially exact, though the sim peaks a bit earlier than real data (~8% vs ~12%) and naturally
shows post-fracture ringing (physically meaningless noise past the fracture point, not a bug — see
`compute_objective.py` docstring).

**To recreate this exact result:** run `dakota.in` as currently configured (5 continuous params,
`coliny_pattern_search`, initial point already warm-started to values near this optimum) via
`driver.sh` in this directory — see "Key files" below for how the pieces fit together.

This was the last thing produced before the session got blocked by a safety-classifier false
positive (likely tripped by fracture/damage terminology combined with optimization language — the
work itself is legitimate materials-engineering calibration). No conclusions were lost, just cut off
before deciding next steps.

## How we got here (chronological, see archived `curves_run*`/`dakota_run*` files for full history)
1. **Power-law hardening**, tuned via Dakota: objective 0.036 (pre-peak region only).
2. **Voce hardening** (saturating law) hand-guessed, then locally refined: objective 0.049
   (pre-peak metric) — worse on that narrow metric, but visually better at the yield knee, and
   *unexpectedly* reproduced post-peak softening with **zero damage model** — confirmed via direct
   cross-section-radius tracking (see below) to be genuine geometric necking (Considère
   instability), not a numerical artifact.
3. Extended `compute_objective.py` to score the **entire curve**, not just pre-peak. Re-ran Voce
   search on this honest metric: objective 0.040, clearly beating power-law's 0.062 on the same
   full-range basis, and captured the whole shape including the softening tail — except the final
   sharp drop (real fracture), which needs an explicit damage/failure model since it's a discrete
   event, not smooth necking.
4. Since capturing the actual failure point matters for this work, added an explicit damage model:
   - First explored **Johnson-Cook damage** (`Peridigm_JohnsonCookDamageModel`, ductile fracture
     criterion εf = D1 + D2·exp(−D3·triaxiality), consumes bond-level plastic strain/triaxiality
     already computed by the hardening material — no new field plumbing needed).
   - Ended up using the simpler **Critical Stretch** damage model instead (single `CriticalStretch`
     parameter) — the 5-parameter joint Dakota search above (4 Voce params + CriticalStretch) is
     what produced the eval-23 result.
   - `compute_objective.py` was rewritten to interpolate sim onto real strain points, only compare
     stress up to the simulation's own *detected* fracture strain (from `extract_stress_strain.py`,
     defined as damage > 0.5 across a full cross-section slice — a real crack-traversal criterion,
     not a hand-picked crop), and add a fracture-strain relative-error term to the objective so the
     optimizer is directly rewarded for getting the failure point right.
5. **Necking validation** (still holds): tracked cross-sectional radius at 8 y-positions along the
   gauge length over time from Exodus output (`saturation_hardening_forward.e.4.*`) — confirmed a
   real localized "waist" progressively forming at gauge center (−31% radius by final frame) while
   the rest of the gauge barely changes (<2%) — textbook diffuse-to-localized necking, not an
   artifact.

## Key files
- `dakota.in` — current Dakota config (5 continuous params: 4 Voce hardening + CriticalStretch;
  `coliny_pattern_search`, 25 max evals, concurrency 2).
- `driver.sh` — Dakota analysis driver: `dprepro`-templates `input_template.yaml`, runs Peridigm in
  Docker, extracts stress-strain curve, computes objective, archives curve/fracture-strain to
  `curves/`.
- `input_template.yaml` — Peridigm input template (material + damage model blocks) with Dakota
  template markers.
- `compute_objective.py` — full-curve + fracture-strain objective (see step 4 above for logic).
- `extract_stress_strain.py` — post-processes Peridigm Exodus output into `sim_curve.csv` +
  `fracture_strain.txt`.
- `calibration_stress_data_HE_0.txt` / `calibration_strain_data_HE_0.txt` — real experimental data
  (baseline, 0% concentration). `calibration_stress_data_HE_30.txt` / `calibration_strain_data_HE_30.txt`
  is the same for the 30% concentration curve.
- `curves_run1`…`curves_run6_voce_prepeak_criticalstretch_dev/`, `dakota_run*.out` — archived
  results from each prior campaign (see chronological history above for what changed run to run).
- `final_best_fit_with_fracture.png` — latest best-fit plot (eval 23, described above).
- `driver_HE30_stage*.sh`, `dakota_HE30_stage*.in`, `extract_best_params.py` — staged calibration
  infrastructure for the other concentrations, see "Staged calibration for other concentrations"
  below.

## Staged calibration for other concentrations (HE_30, ...)

As of 2026-09-01, the real stress/strain data for the 30% concentration curve
landed (`calibration_stress_data_HE_30.txt` / `calibration_strain_data_HE_30.txt`).
Rather than duplicating the whole calibration setup per concentration (which
would let per-curve setups drift out of sync — see "Broader context" above),
`driver.sh` and `compute_objective.py` were generalized to take a
concentration tag, and each concentration's search is broken into three
sequential Dakota runs instead of one big joint search. Staging this way
directly tests the project's central hypothesis: Voce hardening should stay
~fixed across concentrations while `CriticalStretch` is the parameter that
moves with embrittlement severity.

**Infrastructure changes:**
- `driver.sh` now reads two env vars: `CONCENTRATION` (selects
  `calibration_{stress,strain}_data_HE_<CONCENTRATION>.txt`, default `0`) and
  `RUN_LABEL` (names the `curves/<RUN_LABEL>/` archive subdirectory so
  different concentrations/stages don't overwrite each other's archived
  curves, default `HE<CONCENTRATION>`). Existing HE_0 runs are unaffected —
  they default to `CONCENTRATION=0`, landing in `curves/HE0/`.
- `compute_objective.py` takes an optional 5th CLI arg (concentration,
  default `"0"`) selecting which real data files to score against.
- `driver_HE30_stage1_voce.sh` / `_stage2_criticalstretch.sh` /
  `_stage3_joint_polish.sh` — thin wrappers that export `CONCENTRATION=30`
  and a stage-specific `RUN_LABEL`, then delegate to `driver.sh`. One trio
  like this per concentration; for the next one (e.g. HE_70), copy the
  pattern and search-and-replace `30`/`HE30`.
- `extract_best_params.py <dakota.out> [descriptor ...]` — parses Dakota's
  `<<<<< Best parameters` block into `name=value` lines, used to carry a
  stage's result into the next stage's `continuous_state` block (copy-paste
  handoff, not automated — consistent with the manual "best of N evals"
  reading out of `dakota.out` already used throughout this project).
- No changes needed to `input_template.yaml` — Dakota's `dprepro` substitutes
  both `continuous_design` and `continuous_state` variables into `{...}`
  template markers identically, so the same 5-placeholder template serves
  every stage; only which variables are "active" (optimized) vs "state"
  (fixed passthrough) changes per stage's `dakota.in`.

**The three stages (`dakota_HE30_stage{1,2,3}_*.in`):**
1. **Stage 1 — Voce only. DONE/CONVERGED as of 2026-09-04, see result below.**
   4 `continuous_design` params (hardening), `CriticalStretch` fixed as
   `continuous_state` at `0.5` — comfortably above any bond stretch this
   curve will reach, so damage never triggers and the objective's
   stress-shape term (scored over HE_30's own real strain range) drives the
   fit alone.
2. **Stage 2 — CriticalStretch only. NOT YET RUN, paused here to try
   something else.** 1 `continuous_design` param, the 4 Voce params frozen
   as `continuous_state` at the Stage 1 result. **Before running**, edit
   `dakota_HE30_stage2_criticalstretch.in` and replace the placeholder
   `continuous_state` `initial_state` line (currently the HE_0 optimum) with
   Stage 1's actual result:
   ```
   continuous_state = 4
     descriptors    'InitialYieldStress' 'SaturatedYieldStress' 'ExponentialConstant' 'LinearConstant'
     initial_state   1.027e10             1.35e10                22.0                  2.0e8
   ```
   Its wrapper `driver_HE30_stage2_criticalstretch.sh` does NOT set
   `STRESS_ONLY` (unlike Stage 1's) — Stage 2's whole point is calibrating
   `CriticalStretch` against the real fracture strain, so the fracture-strain
   term must stay active there. Then run per the "Running a stage" command
   below, swapping in the stage 2 filenames.
3. **Stage 3 — joint polish (optional but recommended). NOT YET RUN.** All 5
   params together, ~12 evals, tight bounds (~±15%) around the Stage 1+2
   result — catches any Voce/CriticalStretch coupling the two-stage split
   misses, since the objective's fracture-strain term does interact with
   hardening shape. **Before running**, fill in `initial_point`/bounds from
   the Stage 1+2 results the same way.

### Stage 1 result (converged 2026-09-04)

**Final Voce parameters for HE_30** (damage still disabled/fixed during this
stage — see table below for what's actually calibrated vs. carried over):

| Parameter | Value | vs. HE_0 |
|---|---|---|
| InitialYieldStress | 1.027e10 Pa (1027 MPa) | up from 927 MPa (+11%) |
| SaturatedYieldStress | 1.35e10 Pa (1350 MPa) | unchanged |
| ExponentialConstant | 22.0 | unchanged |
| LinearConstant | 2.0e8 Pa/unit-strain (200 MPa/unit-strain) | unchanged |
| CriticalStretch | 0.5 (fixed, disabled — not yet calibrated) | n/a, Stage 2's job |

Objective (`STRESS_ONLY` mode, pure stress-shape error, no fracture term):
**0.1781**, best evaluation 2 of 14 — converged cleanly (a full round of 6
directional probes around eval 2 found no improvement). Source:
`dakota_tabular_HE30_stage1.dat`, `dakota_HE30_stage1_voce.out`. Fit plot:
`stage1_HE30_quickcheck_fit.png` — sim tracks the real HE_30 data almost
point-for-point from 0% through ~12% strain, diverging only at/after the
real fracture point (13.27%), which is expected and correct since this
stage has no damage model to produce a discrete fracture.

**How this result was reached (debugging journey, worth keeping — the same
traps apply to the next concentration):**
1. **Near-zero real-data bug**: HE_30's real stress data has a 1e-6 MPa point
   right after t=0 (sensor noise). The objective's relative-error mask only
   excluded `real_stress <= 0`, so this point passed through and its
   near-zero denominator blew the objective up to millions. Fixed by adding
   `NOISE_FLOOR_FRACTION` (1% of peak stress) to `compute_objective.py`'s
   mask. Verified byte-identical HE_0 result after the fix (HE_0's data has
   no such point, so this was always a latent bug, not a regression).
2. **Bounds-too-tight mistake (self-inflicted)**: first attempt tightened
   `InitialYieldStress` bounds to roughly ±30-40% around HE_0's value,
   reasoning that a narrower box would converge faster since concentrations
   are similar. Backwards: pattern search only walks as far as the objective
   keeps improving, so a wide box costs nothing when the optimum is close,
   and is required when it's not (as turned out here). Result: 23 evals of
   `InitialYieldStress` monotonically climbing every round, hitting 91% of
   the artificial bound with no plateau in sight.
3. **Dakota restart doesn't survive bound changes**: tried to "cheaply
   continue" the above run via `-read_restart` with just the upper bound
   raised. Discovered `coliny_pattern_search`'s default step size
   (`initial_delta`) is 10% of each variable's `(upper - lower)` range,
   confirmed by reverse-engineering the exact deltas in the tabular data —
   so widening a bound changes that variable's step and the entire
   subsequent search trajectory, defeating the cache. `initial_delta` also
   only accepts a single scalar (applied proportionally per-variable), so
   there's no way to pin one variable's step while leaving others alone.
   Lesson: get bounds right before launching, don't try to patch them
   mid-run.
4. **Re-ran fresh with HE_0's original wide bounds** (`InitialYieldStress`
   5e9-1.5e10, etc., matching `dakota.in`'s bounds) — 31 evals, objective
   0.4712, `InitialYieldStress` genuinely plateaued at 1.227e10 this time
   (two separate attempts to push higher both got clearly worse). But the
   fit plot looked worse than expected, and `LinearConstant` had been driven
   to its lower bound (0.0) — suspicious given HE_0's own history showed
   Voce hardening alone (no damage) can reproduce almost the entire real
   curve shape, including the softening tail (see "How we got here" step 2
   above).
5. **Real root cause found**: `extract_stress_strain.py`'s fracture
   detector fires on any sharp force-drop, including geometric
   necking-driven softening — no real bond damage required. Since Stage 1
   deliberately disables damage, the objective's fracture-strain term was
   still comparing against this necking-onset pseudo-fracture, and the
   optimizer was being rewarded for warping the hardening curve (driving
   `LinearConstant` toward 0, which kills late-strain hardening and triggers
   Considère instability earlier) purely to make *necking timing* match the
   real fracture strain — a job that belongs to the damage model in Stage 2,
   not the hardening law. Confirmed directly: at every `InitialYieldStress`
   level tested, `LinearConstant=0` always produced an earlier pseudo-fracture
   than `LinearConstant=2e8`.
6. **Fix**: added a `STRESS_ONLY` env var to `compute_objective.py` — when
   set, drops the fracture-strain term entirely and scores pure stress-shape
   error over the full real curve. Set only by `driver_HE30_stage1_voce.sh`
   (Stage 2/3 and HE_0 are unaffected; re-verified byte-identical HE_0
   result with it unset). Re-running with this fix converged in 14 evals (a
   fraction of the 31 before) to the modest, sensible +11%
   `InitialYieldStress` increase in the table above, instead of the runaway
   +32% the confounded objective had produced — and the fit plot matches the
   real curve almost exactly through the real fracture point.

**Running a stage:** each stage needs its own restart and output files —
Dakota defaults to `dakota.rst`/stdout, which would silently collide across
different concentrations/stages sharing this directory. Use, e.g.:
```
/Users/mschmitz/dakota/bin/dakota -i dakota_HE30_stage1_voce.in \
  -write_restart dakota_HE30_stage1_voce.rst \
  -o dakota_HE30_stage1_voce.out
```
(swap in the stage 2/3 filenames as you progress through the sequence).

## Voce-under-Hypoelastic pivot: parameter reference (as of 2026-09-16)

This section is a reference for the current, actively in-progress line of work: pivoting from
power-law hardening (Isotropic Hardening Hypoelastic Correspondence, adopted earlier to get
native Johnson Cook damage support without source patches) to **Voce hardening now added to that
same Hypoelastic material** (see "Adding Voce hardening to Hypoelastic Correspondence" below),
because power-law's hardening-modulus math can't reproduce this data's sharp yield knee no matter
how it's tuned (see the Considère-criterion analysis from the 2026-09-14/15 sessions: power-law's
initial hardening modulus is bounded, Voce's isn't). Everything below is warm-start/bounds
material for the next round of Dakota campaigns.

### Voce hardening (the path going forward)

| Parameter | Original wide bounds (`dakota.in`) | HE_0 converged result (obj=0.060, eval 23) |
|---|---|---|
| InitialYieldStress | 5.0e9 – 1.5e10 | **9.27e9** (927 MPa) |
| SaturatedYieldStress | 1.0e10 – 1.5e10 | **1.35e10** (1350 MPa) |
| ExponentialConstant | 10.0 – 150.0 | **22.0** |
| LinearConstant | 0.0 – 2.0e9 | **2.0e8** (200 MPa/unit-strain) |

This converged result is the natural warm start for the new Voce-under-Hypoelastic campaign — it's
what `voce_hypoelastic_smoke_test.yaml` used successfully (see below). It was originally paired
with `CriticalStretch=0.167` for damage, not Johnson Cook, so that parameter doesn't carry over.

### Power-law hardening (superseded under Hypoelastic, kept for reference/comparison)

| Parameter | Original Stage 1 (obj=0.05728, eval 25) | Low-exponent Stage 1 rerun (obj=0.05115, eval 15) |
|---|---|---|
| YieldStress | 1.0455e10 | 1.1205e10 |
| HardeningCharacteristicStrain | 0.1361 | 0.1361 |
| HardeningExponent | 0.4405 | 0.2925 |

Bounds used: YieldStress [5.0e9, 2.0e10] both times; HardeningCharacteristicStrain [1e-2, 1.0]
originally, tightened to [0.08, 0.20] for the low-exponent rerun; HardeningExponent [5e-2, 2.0]
originally, deliberately constrained to [0.15, 0.40] for the low-exponent rerun specifically to
keep the search away from the unconstrained ~0.44 optimum (which via Considère's criterion doesn't
neck until ~35% engineering strain, far past the real ~15.37% fracture point).

### Johnson Cook damage — needs a real correction before reuse, not a copy-paste

| Source | D1 | D2 | D3 | ThresholdDamage | Note |
|---|---|---|---|---|---|
| Our best (Stage 2/3 under power-law, now known unstable) | 0.205–0.2112 | 0.04 | **-0.4 to -0.53** | 0.5 | Non-physical sign (see below), crashes near D3≈-0.6 to -0.8 |
| Original informed guess (never actually calibrated) | 0.30 | 0.04 | +0.3 | 0.5 | Positive/correct sign |
| **Reference paper (316L steel, same functional form)** | 0.04 | **2.5** | **3** | n/a (different damage function, hard cutoff at D=1) | Behzadinasab & Foster 2019, SFC3 |
| Paper's own Dakota search bounds | 0.01 – 0.6 | **0.1 – 4** | **0.1 – 5** | n/a | From their Fig. 4 |

Two important corrections to make before the next damage calibration, both found by comparing
against Behzadinasab & Foster's 2019 Sandia Fracture Challenge 3 paper (same Peridigm/Dakota
workflow, same power-law hardening formula, same Johnson Cook damage formula
`epsF = d1 + d2*exp(-d3*triaxiality)`):

1. **Our D2 upper bound (0.5) has been far too tight.** The paper's own calibrated value (2.5) and
   search range (0.1–4) sit entirely outside it.
2. **D3 should stay non-negative.** Our own search pushed D3 negative because, under the
   power-law hardening's flawed shape, negative D3 was the only direction that improved the
   fracture-timing match — but it does so non-physically (it makes epsF *grow* with triaxiality
   in the neck instead of shrinking, fighting the natural strain localization) and it sits right
   next to a real numerical cliff (`exp(-D3*triaxiality)` overflows for very negative D3 combined
   with the neck's high triaxiality, causing the repeated `NaN returned by force evaluation`
   crashes in Stage 2/3). The paper's own calibrated D3=3 (clearly positive) is consistent with
   this being the physically correct sign.

Recommended bounds/warm-start for the next damage calibration (once Voce hardening is locked in):
D1 [0.02, 0.6], D2 [0.05, 4.0], D3 [0.0, 5.0], warm-started from the positive-sign guess
(D1=0.30, D2=0.04, D3=0.3) rather than the negative values found under power-law.

### Adding Voce hardening to Hypoelastic Correspondence

Implemented 2026-09-16 in `src/materials/Peridigm_IsotropicHardeningPlasticHypoelasticCorrespondenceMaterial.cpp`/`.hpp`:
added a `Hardening Rule` dispatch (`"Power-law"`, the default if omitted for backward
compatibility with all existing examples, or `"Saturation type exponential"`), mirroring the
dispatch pattern already used by `IsotropicHardeningPlasticBondAssociatedCorrespondenceMaterial`.
This was a small, low-risk change: the actual Voce physics kernels
(`updateElasticIsotropicSaturationExponentialHardeningPlasticCauchyStress` and its bond-level
counterpart) already existed as shared, reusable functions in
`isotropic_hardening_correspondence.h/.cxx` — only the dispatch/parameter-reading needed adding,
no new fields, and none of the `Jacobian_Determinant`/`Undamaged_Weighted_Volume` field-collision
risk that made the earlier Bond-Associated+Johnson-Cook patch (see the Johnson Cook section
elsewhere in this project's history) more involved. Built into Docker image
`peridigm-masoud:voce-hypoelastic`. YAML usage: set `Hardening Rule: "Saturation type
exponential"` plus `Initial Yield Stress`/`Saturated Yield Stress`/`Exponential Constant`/`Linear
Constant` instead of the power-law params, at the same flat level under `Materials: My Material:`
(no sublist needed) — see `voce_hypoelastic_smoke_test.yaml`. Validated via a 10%-length smoke
test (damage disabled): produced the correct sharp-knee-then-saturate Voce shape, no crashes.

## Voce-under-Hypoelastic Stage 1 result (converged 2026-09-17/18)

Stage 1 (`dakota_HE0_voce_hypoelastic_stage1_hardening.in`, Voce hardening only, damage disabled,
`STRESS_ONLY`) was manually halted after 10 of 25 evaluations once gains clearly plateaued (same
practice as the power-law Stage 1 low-exponent rerun). Best result: **eval 9, objective 0.0360**:

| Parameter | Value | vs. old Bond-Associated optimum |
|---|---|---|
| InitialYieldStress | 9.77e9 (977 MPa) | up from 927 MPa (+5.4%) |
| SaturatedYieldStress | 1.35e10 (1350 MPa) | unchanged |
| ExponentialConstant | 22.0 | unchanged |
| LinearConstant | 2.0e8 (200 MPa/unit-strain) | unchanged |

Only `InitialYieldStress` moved meaningfully — the old Bond-Associated-tuned Voce parameters
transferred almost perfectly to Hypoelastic as-is (confirmed first via
`voce_hypoelastic_full_test.yaml`, a full-length forward test with damage disabled: near-exact
match through the yield knee and plateau, and genuine geometric-necking softening past ~12% strain
from the hardening law alone, same as the original Bond-Associated finding).

## Stage 2 crash saga: a real bug in the Voce plasticity return-mapping solver

Stage 2 (Johnson Cook damage, hardening frozen at the Stage 1 result above) went through **seven
attempts** before converging cleanly. The first six were spent chasing what looked like a
ramp-width problem; the real cause turned out to be a latent numerical bug exposed for the first
time by this exact combination (Voce + active damage), unrelated to `ThresholdDamage`'s value.

**The "near-hard-cutoff" idea:** Behzadinasab & Foster's SFC3 paper uses a damage function that's a
hard binary cutoff (`ωD = 1` if `D < 1`, else `0`) rather than Peridigm's built-in Johnson Cook model's
gradual linear ramp (`bondDamage` climbs from 0 to 1 as accumulated damage `D` climbs from
`ThresholdDamage` to 1.0 — see `Peridigm_JohnsonCookDamageModel.cpp:220-231`). Since a lower
`ThresholdDamage` just means a *wider* version of that same linear ramp, pushing `ThresholdDamage`
close to 1.0 approximates the paper's hard cutoff with zero source changes. The template's fixed
`Critical Damage` was bumped from 0.95 to 0.999 to leave room for this (it's validated but never
functionally consumed in the damage-evolution code, so raising it is free).

**Attempts 1-3 (ThresholdDamage 0.97, then 0.85, then 0.7 with D1/D2 pulled down to compensate):**
all three crashed with the identical error, at different eval counts and different D1/D2/D3
combinations:
```
isotropic_hardening_correspondence.cxx::radialReturnSaturationExponential: Too many iterations in the plastic radial return.
```
The working hypothesis at the time: a narrower ramp causes many bonds to lose strength nearly
simultaneously, and that abrupt a stiffness change was pushing Voce's local plasticity
return-mapping iteration outside its convergence region.

**Attempt 4: reverted all the way back to `ThresholdDamage=0.5`** — the exact value every power-law
run in this whole project used successfully, many dozens of times, without ever hitting this
error. **It crashed too**, at eval 4. This disproved the ramp-width hypothesis entirely: the crash
was never about how narrow or wide the ramp was.

**Root cause, found by reading `radialReturnSaturationExponential` directly**
(`src/materials/isotropic_hardening_correspondence.cxx`): the Newton-iteration convergence
tolerance was a pure *absolute* value, `tol = 1.0e-16 * bulkMod` (≈1.3e-4 in this problem's raw
units), with a 100-iteration cap. This is shared verbatim by `radialReturnPowerlaw` too — it's not
Voce-specific code. Under ordinary, gradual loading this converges in a handful of iterations
easily (evidenced by literally thousands of successful calls across every prior run in this
project). But when damage causes an abrupt, large stiffness loss in a single time step, the
resulting elastic "trial stress" fed into the solver can spike unusually large — at that scale, the
absolute tolerance (pegged to `bulkMod`, ~1.3e12, not to the actual working stress scale of
~1e9-1e10) becomes effectively unreachable: the iteration oscillates at floating-point precision
without ever satisfying it, and hits the 100-iteration cap. This never showed up under power-law
because every power-law run that ever had active damage also had the (separately diagnosed,
already-fixed) negative-D3 `exp()` overflow bug, which always killed the run first, before the
simulation ever survived long enough into an active-damage regime to expose this second, unrelated
bug. Voce (with D3 correctly kept positive) was the first path to actually reach that regime.

**The fix** (`isotropic_hardening_correspondence.cxx`, inside `radialReturnSaturationExponential`):
added a relative-tolerance fallback, computed right after the trial von Mises stress
(`vmStress_trial`) is known:
```cpp
tol = std::max(1.0e-16 * bulkMod, 1.0e-12 * vmStress_trial);
```
This changes nothing for any normal-scale step (the original absolute term still dominates and is
reached quickly there — every prior result in this project is unaffected), and only relaxes the
target when `vmStress_trial` spikes unusually large, letting the solver converge at a
still-extremely-tight (1e-12) relative precision instead of chasing an unreachable absolute one.
Rebuilt into `peridigm-masoud:voce-hypoelastic`. (`radialReturnPowerlaw` has the identical latent
issue and would likely benefit from the same fix if a future power-law + active-damage campaign
ever needs it, but wasn't touched since nothing currently exercises that path.)

### Attempt 6 (post-fix): ThresholdDamage=0.85 — converged cleanly, best result yet

With the fix in place, `ThresholdDamage=0.85` ran all 30 evaluations with **zero crashes**. Best
result: **eval 15, objective 0.0413** — the best full-objective (stress-shape + fracture-strain)
result of the entire project:

| Parameter | Value |
|---|---|
| DamageParameter1 | 0.1355 |
| DamageParameter2 | 0.03 |
| DamageParameter3 | 0.55 |
| ThresholdDamage | 0.85 (fixed) |
| **Predicted fracture strain** | **15.27%** vs real 15.37% (0.10 points off) |

Visual inspection (`voce_hypoelastic_stage2_postfix_eval15_best_fit_filtered.png`) showed the
pre-fracture fit tracking the real curve almost exactly through the whole yield/plateau region,
with a clean, coherent post-fracture decline once smoothed. One residual issue: a small disturbance
in the raw curve starts around 13.3% strain, before the main oscillation kicks in right at the
detected fracture point (15.27%) — visually messier than the "hold steady, then fail, then
oscillate" sequence seen in the old power-law plots.

**Direction for tightening this further: raise `ThresholdDamage`, not lower it.** A lower threshold
widens the ramp, which is exactly what causes bonds to start softening earlier and inject noise
into the aggregate curve before the official fracture point — confirmed directly by comparing
attempt 6 (0.85, noise ~13.3%, tight gap to 15.27% fracture) against the earlier attempt 3 (0.7,
noise ~13%, fracture at 15.12%, a wider ~2-point gap). Raising the threshold keeps bonds fully
intact longer, producing a tighter, more sudden coupling between the noise onset and the actual
fracture point. **Attempt 7 (in progress as of this writing):** `ThresholdDamage=0.95`,
warm-started from attempt 6's best result (D1=0.1355, D2=0.03, D3=0.55) — a step short of the 0.97
that crashed pre-fix, now worth revisiting with the tolerance fix in place.

## Brozzo/Wellman damage model — HE_0 final result (locked in 2026-09-30)

Superseded Johnson-Cook (above) with the **Brozzo/Wellman ductile damage law** from Behzadinasab &
Foster 2020 ("Revisiting the third Sandia Fracture Challenge"), implemented in
`Peridigm_BrozzoDamageModel.cpp`/`.hpp` (built into `peridigm-masoud:voce-hypoelastic-brozzo`):

```
D_dot = d1 * <2*sigmaT / (3*(sigmaT - sigmaM))>^d2 * eqps_dot
```

(sigmaT = bond-level max principal stress, sigmaM = hydrostatic stress, Heaviside-gated to
tension only). Uses the same linear damage-ramp mechanic as Johnson-Cook
(`bondDamage = (D - ThresholdDamage)/(1 - ThresholdDamage)`), mathematically identical in
structure to the paper's own `ωD = (1-D)/(1-Dc)`.

**Locked-in HE_0 result** (Voce hardening frozen at the Stage 1 values above; hardening + damage
never rejointly polished together for Brozzo — only d1/d2 were optimized, on the same reasoning
Johnson-Cook used):

| Parameter | Value |
|---|---|
| DamageParameter1 (d1) | **3.5** |
| DamageParameter2 (d2) | **0.99** |
| ThresholdDamage | 0.85 (fixed; same value used throughout the Johnson-Cook work, never beaten by 0.9/0.95 alternatives) |
| Influence Function | "One" (Peridigm default, constant weight — see below) |
| **Predicted fracture strain** | **15.87%** vs real 15.37% |
| **Objective** | **0.0688** |

Source: `dakota_HE0_brozzo_d1d2_polish.in` + its resumed continuation
(`dakota_HE0_brozzo_d1d2_polish_resumed.out`), a `coliny_pattern_search` over d1/d2 jointly, best
of 13 evaluations (eval 3) — note Dakota's own end-of-run "Best parameters" block misreported eval
1 as best here; always cross-check the raw `dakota_tabular_HE0_brozzo_d1d2_polish.dat` (a recurring
gotcha in this project). Plot: `voce_hypoelastic_HE0_brozzo_best_d1_3p5_d2_0p99_truncated.png`.

**How this was reached:**
1. The paper's own d1=0.76/d2=0.95 (calibrated for a different material) never accumulated enough
   damage to fracture the HE_0 specimen at all, at any `ThresholdDamage` tried (0.85/0.9/0.95 gave
   byte-identical no-fracture results) — ruled out threshold-only tuning.
2. Coarse d1 sweep {1.5, 3.0, 6.0, 12.0} (d2 fixed at paper's 0.95): d1=6.0 crashed
   (`NaN returned by force evaluation`, too-aggressive damage accumulation); d1=3.0 was the best
   surviving point.
3. Tighter sweep {3.5, 4.0, 4.5, 5.0}: d1=3.5 best (frac=15.91%, obj=0.0715).
4. Joint d1/d2 `coliny_pattern_search` warm-started at (3.5, 0.95), bounds d1∈[3.0,4.5]/d2∈[0.75,1.15]
   → converged to the (3.5, 0.99) result above; confirmed a genuine local optimum (all other 12
   evals scored worse).
5. **Influence Function side-investigation (concluded, not adopted):** tried Peridigm's
   `"Parabolic Decay"` influence function (tapered bond weight, quadratic decay to 0 at the horizon
   edge, closest supported analog to the paper's own `"Full Parabolic Decay"` found in this repo's
   bundled `calibration/Tests/InputFile*.xml` reference files) in place of the default constant
   `"One"` used everywhere else in this project. A same-d1/d2 A/B comparison and a dedicated 5-eval
   joint search under Parabolic Decay both did *worse* (best obj 0.1202 vs constant-weight's 0.0688)
   and did not fix the known post-fracture oscillation (see below) — concluded Parabolic Decay is
   not worth adopting; `"One"` (already Peridigm's default, used implicitly by every run before this
   investigation) is the setting going forward. Template kept for reference:
   `voce_hypoelastic_brozzo_parabolicdecay_input_template.yaml`.

**Post-fracture oscillation (known, root cause corrected 2026-09-30, still not fixed):** originally
guessed (never verified) to be caused by a missing Contact/Self-Contact model letting separated
fragments interpenetrate. **That hypothesis has since been directly disproven.** Added Peridigm's
native "Short Range Force" Contact model (General + Self Contact, Contact Radius 0.05, Search
Radius 0.075 — see `voce_hypoelastic_brozzo_contact_input_template.yaml` /
`driver_HE0_brozzo_contact_forward_solve.sh`) to the locked-in best point (d1=3.5, d2=0.99) and
re-ran the full forward solve: the `Contact_Force` output field was **exactly zero at every node,
every timestep, every component**, and the raw stress-strain curve came out byte-identical to the
no-contact run. Went further and directly measured node proximity in the post-fracture region
(comparing current vs. reference-configuration distances for every node pair within the search
radius, across 8 frames spanning the oscillation): **zero node pairs were found that weren't
already bonded neighbors in the undeformed mesh** — i.e. no two originally-separate surfaces ever
swing back into geometric contact. The oscillation is therefore *not* a fragment-interpenetration
problem **for the Brozzo model specifically**.

**Root cause identified (2026-09-30), confirming the user's own hypothesis:** pulled the per-element
`Damage` field directly from the contact-enabled forward solve at several frames well past the
detected fracture point (15.87% strain, frame ~536 of 918). Result: of 5,000+ elements carrying any
damage at all, essentially **none ever reach full failure (D≈1)** — at frames 536/700/850 exactly
zero elements were fully failed, and even at the very last frame (917) only a single element had.
Every other damaged element sits in a partial, still-load-bearing state (D somewhere between ~0.01
and ~0.9) for the entire remainder of the simulation. **The specimen never actually severs into two
disconnected pieces at the bond level** — this is also *why* the contact-model test above found zero
interpenetration: there's nothing to interpenetrate, since the "two halves" are never mechanically
independent. This matches what `extract_stress_strain.py`'s own docstring already documented (the
crack surface is rough/patchy, not a clean flat plane) and confirms fracture-strain detection there
is really flagging a big aggregate stiffness/strength drop from widespread *partial* damage, not a
literal clean break.

This directly explains the oscillation mechanism: the grips are rigidly locked to their prescribed
`y*45.0*t` trajectory (zero freedom to move off it), but the neck between them becomes a spatially
heterogeneous field of bonds at wildly different damage levels (many barely touched, some at
30-50%, a rare few near full failure). As the grips keep pulling at a constant rate, that ragged,
still-evolving stiffness field responds unevenly — individual bonds keep crossing their
damage-ramp thresholds at different moments in different places, each one releasing stored elastic
energy into the surrounding still-intact material as a small dynamic snap, which is what shows up
as oscillation in the reaction-force curve and visually in ParaView near the neck. It's also
plausibly why the correspondence-material shape-tensor calculation gets numerically touchy here: a
shape tensor drawn from neighbors spanning that wide a spread of bond degradation is exactly the
condition that tends toward ill-conditioning — so the "partial, patchy damage never fully clearing"
finding and the earlier shape-tensor-sensitivity guess aren't competing explanations, they're the
same underlying cause viewed from two angles. Doesn't affect the locked-in calibration since
`compute_objective.py` already excludes the post-fracture region.

**Important nuance (per user, 2026-09-30):** under the earlier **Johnson-Cook** damage model,
ParaView visualization of the fracture region showed nodes visibly crossing/passing through one
another after the bar breaks — genuine geometric interpenetration, exactly the failure mode a
Contact model is meant to catch. That visual interpenetration is no longer present under Brozzo, so
the negative contact-model result above is specific to Brozzo's damage evolution/fracture morphology,
not a general statement that contact never matters for this project. If Johnson-Cook (or any other
damage model with that visible cross-through behavior) is revisited later, a Contact model is still
worth trying there — it was never actually tested under JC, only guessed at.
Plotting workaround still in use: truncate the sim curve at its own detected fracture strain
*before* Savitzky-Golay filtering (see the `_truncated.png` plots referenced above).

## Fracture Plane Cutoff ("Option 3") -- tried, also failed (2026-10-01)

After Options 1 (freeze nearly-failed points) and 2 (per-bond hard cutoff) both failed, implemented
"Option 3": once any point within a fixed geometric band around `Fracture Plane Y` (0.0, the
specimen's symmetric center) reaches `Fracture Trigger Damage` (0.3), every bond touching that band
is forced to full failure (D=1) in one step -- a genuinely complete cross-section severance, checked
symmetrically from both endpoints and synchronized across MPI ranks via an `Epetra_Comm::MaxAll`
reduction on the trigger flag (so it runs correctly at full 4-rank parallelism, not just single-rank).
See `Peridigm_BrozzoDamageModel.cpp`/`.hpp` (`Enable Fracture Plane Cutoff`, `Fracture Plane Y`,
`Fracture Plane Half Thickness`, `Fracture Trigger Damage`), `voce_hypoelastic_brozzo_fractureplane_input_template.yaml`.

**Result: the severance itself worked exactly as designed, but the oscillation got *worse*, not
better.** Verified directly via the `Flying_Point_Flag` field: exactly 80 elements, all within the
intended plane band (reference Y in [-0.222, 0.222]), became fully disconnected starting the instant
the trigger fired and stayed stable afterward -- a genuine, complete, one-time cut, not a partial or
spreading one. Despite that, the raw stress-strain curve oscillated *earlier* (starting ~11% instead
of ~16%) and just as violently all the way to the end of the simulation, instead of settling down.

**Likely explanation:** forcing a complete cross-section severance in a single time step releases all
the elastic energy stored across that entire plane's bonds simultaneously, rather than gradually
bond-by-bond like the ramp did -- a bigger dynamic shock, not a smaller one. This simulation has
essentially no energy-dissipation mechanism to absorb that shock (no contact engagement, as already
established; no artificial damping). So instead of a patchy, gradually-decaying ligament, forcing a
clean cut produces a sudden, large-amplitude free vibration of both now-separated halves with nothing
to damp it out.

**Reframing:** all three options attacked *how damage gets applied* to bonds. The actual missing
ingredient may be *energy dissipation* -- this system has no good way to absorb the kinetic energy
released once real fracture starts, regardless of how cleanly or messily that fracture is
represented. Not yet investigated (e.g. artificial/numerical damping). Tabled as of 2026-10-01 in
favor of the existing truncate-before-filter plotting workaround, which remains unaffected since it
doesn't touch the pre-fracture region used for actual calibration.

## Bond Associated Correspondence material test ("Option 4") -- also failed (2026-10-01)

Based on the SFC3 authors' own 2019 paper documenting a shape-tensor-singularity failure mode in
ordinary ("Hypoelastic") correspondence materials once enough of a point's bonds break (Fig. 15:
a rank-deficient shape tensor can go undetected and "continue with the simulation, leading to large
errors afterwards"), tested whether swapping to `Isotropic Hardening Bond Associated
Correspondence` (which computes a separate local shape tensor per bond instead of one shared one
per point, and is therefore structurally more resistant to this) would fix the oscillation. Ran
the exact same locked-in point (d1=3.5, d2=0.99, ThresholdDamage=0.85, HE_0 Voce hardening) under
this material -- see `voce_bondassociated_brozzo_input_template.yaml` / `driver_HE0_bondassociated_brozzo_forward_solve.sh`
(no rebuild needed, already core compiled Peridigm source).

**Result: no improvement.** Runtime ~3700s, essentially identical to the Hypoelastic baseline (no
meaningful per-bond-shape-tensor slowdown here). Fracture strain 15.67% vs baseline's 15.91% (real
15.37%) -- both close. Objective improved slightly (0.0547 vs baseline's 0.0688), but that tracks
back to marginally tighter pre-fracture/fracture-point matching, not reduced oscillation:
post-fracture stress std was statistically identical (314.1 vs baseline's 313.6), and the overlay
plot (`voce_hypoelastic_vs_bondassociated_brozzo_comparison.png`) shows both materials oscillating
with the same onset and the same amplitude, fully tangled together.

**This rules out the shape-tensor-singularity hypothesis as well.** Four separate interventions now
tried (Contact, Hard Cutoff, Fracture Plane severance, Bond Associated correspondence) have each
left the post-fracture oscillation unchanged or made it worse. This is reasonably strong convergent
evidence that the cause isn't the damage law or the material's shape-tensor kinematics, but the
earlier energy-dissipation hypothesis above: there's simply no mechanism in this simulation to
absorb the kinetic energy released once real fracture/stiffness-loss begins. Next step, if pursued:
implement actual damping (see the three candidate approaches discussed 2026-10-01 -- global
velocity-proportional/Rayleigh damping, a damage-scaled version of it, or bond-level
relative-velocity micro-damping/dashpot) rather than further damage-law or material-model changes.

## Bond-Level Micro-Damping ("Option 5") -- implemented and tried, also failed (2026-10-01)

Implemented a new `DampingModel` category (mirroring the existing `DamageModel` architecture) in
`src/damage/Peridigm_DampingModel.hpp`, `Peridigm_DampingModelFactory.{hpp,cpp}`,
`Peridigm_BondLevelMicroDampingModel.{hpp,cpp}`, wired into `Peridigm_Block.{hpp,cpp}`,
`Peridigm_ModelEvaluator.cpp` (invoked right after each block's material force computation, before
Contact), and `Peridigm.cpp` (new top-level `Damping Models:` YAML sublist, same pattern as
`Damage Models:`). For every bonded pair (i,j), adds `f_i -= c * (v_i - v_j) * Volume_j` directly
into `Force_Density`, scaled by `(1 - Bond_Damage)` so it fades out as bonds fail. Chosen
specifically because it only depends on RELATIVE velocity (zero under rigid-body translation), so
unlike global/Rayleigh damping it shouldn't resist the overall prescribed grip motion or disturb the
already-calibrated pre-fracture fit. See `voce_hypoelastic_brozzo_damping_input_template.yaml` for
the full rationale and the order-of-magnitude derivation used to pick a starting coefficient.

**Result: no useful regime found across three orders of magnitude.** Tested Damping Coefficient =
1.0e6, 1.0e7, 1.0e8 (all CGS) at the locked-in HE_0 point (d1=3.5, d2=0.99, ThresholdDamage=0.85).
Post-fracture stress std: baseline (no damping) 313.6 -> 1e6: 313.2 (zero effect) -> 1e7: 301.2
(~4% reduction, fracture-strain detection also shifted to 17.28% from 15.91%, worsening the
objective to 0.1596) -> 1e8: 303.7 (no better than 1e7, fracture strain back to 16.07%, obj 0.0772).
None of the three runs crashed. Overlay plots at every coefficient show the damped and baseline
curves sitting almost exactly on top of each other through the entire post-fracture oscillation
region -- pre-fracture fit was also completely unaffected at every coefficient (confirming the
relative-velocity design worked as intended on that front, just not usefully).

**Root cause of the non-result:** worked out directly from the explicit-integration math, not
guessed. For an isolated bonded pair, the damping term gives a decay rate `lambda = 2*c*V/m`. The
relevant criterion isn't blowup (that threshold is far higher, consistent with never crashing even
at 1e8) -- it's whether `dt*lambda` stays small enough for the explicit (Verlet) integrator to
represent the term as smooth exponential decay rather than a numerically neutral sign-flip every
step. That crossover works out to `c ~ 4e7` for this mesh/timestep -- right in the middle of the
tested range. Below it (1e6, 1e7), decay is clean but far too slow to meaningfully reduce amplitude
within the handful of oscillation cycles available before the simulation ends (the oscillation
period here is only a few timesteps). At or above it (1e8), the explicit scheme can no longer
resolve the term as true dissipation at all -- it just flips sign step to step, contributing no net
energy removal. There is no window between "too slow to matter" and "too fast for explicit Verlet to
resolve" with this formulation. A real fix would require treating the damping term semi-implicitly
(backward-Euler for the dissipative part specifically, within the otherwise-explicit integrator) --
a materially bigger change to the integrator loop, not just the materials/damage layer.

**Decision (2026-10-01, user call):** given five independent interventions now failed (Contact, Hard
Cutoff, Fracture Plane severance, Bond Associated correspondence, Bond-Level Micro-Damping),
abandoned further pursuit of a post-fracture-oscillation fix. The existing truncate-before-filter
plotting workaround remains in place and is unaffected -- it already fully excludes the
post-fracture region from `compute_objective.py`'s scoring, so none of this affects the locked-in
calibration results. The new `Damping Model` infrastructure is left in the codebase (harmless,
opt-in, zero effect on any block that doesn't request it) in case a semi-implicit version is ever
worth revisiting.

## Rigid-grip BC, loading rate match, and a ThresholdDamage x Hard Cutoff sweep ("Option 6") -- also failed (2026-10-05/07)

Revisited the oscillation investigation after a gap, starting from two methodology corrections
found while re-reading Behzadinasab & Foster 2020 ("Revisiting...") more closely:

1. **Rigid-grip boundary condition.** The paper applies "displacements... prescribed on 5 layers of
   peridynamic nodes at each end" -- a single uniform displacement per grip layer, not a
   position-dependent field. Our original BC (`y*45.0*t`, scaling displacement by each node's own
   y-coordinate within the grip) was the opposite of this. Switched to a uniform rigid-grip BC
   (`-220.0*t` / `220.0*t`) -- see `voce_hypoelastic_brozzo_rigidgrip_input_template.yaml`. The rate
   (220 cm/s) was rescaled from an initial naive 45 cm/s after discovering the grip node sets
   actually sit at y in [-5.08,-4.70]/[4.70,5.08] in the mesh, so the original `y*45.0*t` gave those
   nodes ~4.9x more displacement than a flat 45.0*t would.
2. **Loading rate matched to the paper's own value.** The paper states their loading rate is 1 m/s
   (validated via their own 1/0.1/0.01 m/s rate-sensitivity comparison as negligibly different
   except in early elastic loading). Our rate (220 cm/s = 2.2 m/s) was ~2.2x faster than their
   fastest validated rate. Re-ran at the literal 1 m/s (100 cm/s, Final Time scaled 2.2x to
   4.048e-3s to reach the same total displacement) -- see
   `voce_hypoelastic_brozzo_rigidgrip_slowrate_input_template.yaml`. Result: fracture strain moved
   closer to the real target (15.79% vs the 220 cm/s version's 18.42%, real target 15.37%), but
   quantitative comparison (post-fracture std 312.5 @ 220cm/s vs 333.1 @ 100cm/s, elastic-region
   sawtooth noise statistically identical at both rates) confirmed loading rate has no meaningful
   effect on the oscillation itself -- consistent with the already-established root cause (stored
   elastic energy release is governed by the rate-independent material law, not loading rate).

**Hard Cutoff revisited under the rigid-grip BC, three single-point tests across concentrations:**
tested Hard Cutoff at each concentration's own locked-in `ThresholdDamage` (HE_0 @ 0.85, HE_30 @
0.51, HE_60 @ 0.19) to see how the "deleted ramp region" width (`1 - ThresholdDamage`) affects
outcome:

| | ThresholdDamage | Ramp-mode fracture | Hard Cutoff fracture | Shift | Post-fracture std |
|---|---|---|---|---|---|
| HE_0 | 0.85 (narrow) | 15.87% | 18.09% (18.01% under rigid-grip) | later | 277.8 (325.2 rigid-grip) |
| HE_30 | 0.51 (mid) | 13.23% | 14.83% | later | 341.0 |
| HE_60 | 0.19 (wide) | 7.80% | 4.40% | **earlier** | catastrophic |

Mechanistic read: Hard Cutoff's effect depends on how many bonds cross threshold simultaneously, not
the literal ramp width -- a low threshold is an "easy milestone" reached by many bonds across the
necking region at once, so Hard Cutoff disconnects that whole cluster in one step (HE_60's
catastrophic, early failure); a high threshold is reached by fewer, more individually-staggered
bonds, so Hard Cutoff's per-event shock is smaller (HE_0's milder result). Also notable: the same
threshold (0.85) gave a meaningfully *worse* std under the rigid-grip BC (325.2) than the original
BC (277.8) despite near-identical fracture timing -- BC convention has some independent effect on
oscillation amplitude, separate from the threshold value.

**Full 6-point `ThresholdDamage` sweep under Hard Cutoff=true, HE_0, rigid-grip BC, fast rate (220
cm/s)** -- see `dakota_HE0_brozzo_rigidgrip_hardcutoff_sweep.in` /
`driver_HE0_brozzo_rigidgrip_hardcutoff_sweep.sh`, run serially, archived to
`curves/HE0_brozzo_rigidgrip_hardcutoff_sweep/`. Goal: check whether some threshold exists where
oscillation amplitude is acceptably reduced *and* fracture timing is still close enough to the real
target to be useful, i.e. whether narrowing the synchronized-failure cluster via Hard Cutoff could
ever actually beat plain ramp mode rather than just being less bad than its worst cases.

| ThresholdDamage | Fracture strain | Post-fracture std |
|---|---|---|
| 0.60 | 16.18% | 343.2 |
| 0.70 | 17.23% | 338.3 |
| 0.75 | 17.68% | 333.4 |
| 0.85 | 18.01% | 325.2 |
| 0.90 | 18.46% | 322.3 |
| 0.95 | 18.66% | 315.1 |
| **Ramp mode (reference)** | **15.87%** | **313.6** |

**Result: no sweet spot anywhere in the range -- Hard Cutoff is strictly dominated by plain ramp
mode on both axes, at every threshold tested.** Oscillation amplitude decreases smoothly and
monotonically as threshold rises, but even at the top of the tested range (0.95) it only reaches
315.1 -- still *above*, not below, ramp mode's own 313.6. Fracture timing is worse than ramp mode's
15.87% at every single point, from 16.18% all the way out to 18.66%. So across this entire sweep,
there is no threshold where Hard Cutoff improves on what the default ramp already achieves for free
on either metric, let alone both at once. Plot: `HE0_hardcutoff_threshold_sweep_summary.png`.

**Interpretation, tying back to a direct visual observation (user, watching the fracture in
ParaView):** after the neck loses stiffness, the displacement field shows a classic longitudinal
elastic wave reverberating between the two ends -- the clamp end advances, the broken end "catches
up" and overtakes it, then the clamp end advances again, back and forth. This fits the established
root cause precisely: the grips are kinematically driven and never stop moving regardless of what
happens at the neck, while the damaged neck acts as a partially-reflecting junction between two
still-stiff segments -- a textbook setup for a standing/traveling wave that never decays, since
nothing in the system dissipates energy and the driven ends keep re-injecting it. This explains why
Hard Cutoff (and the whole ThresholdDamage sweep) can only ever modulate the *size of the initial
shock* that launches the wave -- it does nothing about the reflecting junction or the continuously-
driven boundary that sustains the reverberation afterward, so no amount of threshold tuning under
this mechanism was ever going to make the oscillation disappear, only shrink it somewhat at the cost
of fracture-timing accuracy.

**Decision:** this closes out Hard Cutoff / ThresholdDamage tuning as a sixth failed intervention,
alongside the original five (Contact, Hard Cutoff single-point, Fracture Plane severance, Bond
Associated correspondence, Bond-Level Micro-Damping). The real lead going forward, per the wave
interpretation above, is a genuine dissipation mechanism for the traveling wave -- i.e. revisiting
Bond-Level Micro-Damping's semi-implicit integration idea (see above), not further damage-law
parameter tuning.

**Side investigation (2026-10-06, concluded without changing anything): comparing our model
formulation against the paper's Section 2 equation-by-equation.** Found and corrected a real misread
along the way (worth remembering: always verify which material class a given function actually
belongs to before drawing conclusions about "our" implementation):
- Paper's bond-level velocity gradient (with objectivity correction) -- **matches** our
  `computeBondLevelVelocityGradient` (`correspondence.cxx:1966`) almost exactly.
- Paper's damage-weighted shape tensor/influence state (`omega = omega_eta * omega_D`) -- **matches**
  our actual material (`HypoelasticCorrespondenceMaterial`, via `computeGradientWeights` and
  `computeWeightedVolume`, both of which take a live `bondDamage` array). An earlier claim in this
  investigation that our shape tensor is damage-blind was wrong -- it was based on reading
  `computeShapeTensorInverseAndApproximateDeformationGradient` in the unrelated, unused
  `Peridigm_CorrespondenceMaterial.cpp` base class (which does hardcode `bondDamage=0.0`, but none of
  our materials inherit from it).
- Paper's semi-Lagrangian neighbor list (`H(x)` redefined every step from the *current* deformed
  configuration, bonds forming/dropping purely from relative motion) -- **does not match**. Confirmed
  `globalNeighborhoodData` is built once at initialization from the undeformed mesh
  (`Peridigm.cpp:938`) and never rebuilt during the time-stepping loop, for any material including
  Bond Associated. However, a source comment in the author's own fork
  (`Peridigm_Material.cpp:54`, `// For the semi-Lagrangian (Hypoelastic) models`) labels our
  Hypoelastic materials themselves as "semi-Lagrangian" -- apparently referring to using *current*
  (not reference) coordinates for the per-step geometry within a *fixed* membership list, which we do
  have, rather than literal dynamic re-neighboring, which we don't. Net effect: this gap is real but
  likely smaller than first thought, and its relevance to the oscillation specifically is unverified
  either way.
- Also checked whether the paper's actual damage-law numbers (D1=0.04/D2=2.5/D3=3, their own
  Johnson-Cook SFC3 calibration) still ring when dropped onto our own geometry/hardening -- an
  existing result from an earlier session (`forward_runs/HE0_sfc3_damage_params/`) already answered
  this: yes, still rings (fracture 18.26%, same violent post-fracture pattern), arguing against a
  full Table-1-parameter-swap test being worth running, since it would carry the same
  geometry-mismatch confound for no new information.

## HE_30 Brozzo calibration — ThresholdDamage sweep breaks the stalled d1/d2 polish (2026-10-02)

The joint d1/d2 `coliny_pattern_search` polish (see `dakota_HE30_brozzo_d1d2_polish.in`) stalled
after 5 evals: best was eval 3 (d1=3.5, d2=1.03, obj=0.1813), only ~1.5% better than the warm-start
(obj=0.1840), with both d1±0.3 directions already worse -- a sign d1=3.5 was a local optimum, and
continuing would at best polish d2 without closing the fracture-strain gap (d1=3.5 fractures at
15.40% vs the real HE_30 target of 13.27%; pushing d1 higher to close that gap crashes between 5.0
and 5.5, see the "Direction for tightening" sweep notes above).

Instead of continuing that search, tested `ThresholdDamage` itself (fixed at HE_0's value, 0.85,
throughout every prior HE_30 run and never tuned for this curve) -- see
`driver_HE30_brozzo_threshold_sweep.sh` / `dakota_HE30_brozzo_threshold_sweep.in`. Rationale:
lowering the threshold widens the damage ramp (bonds lose stiffness gradually over a larger
accumulated-damage range instead of a sharp final 15%), which could shift the force-drop-detected
fracture point earlier at a fixed, numerically-safe d1=3.5 instead of needing to push d1 into the
crash zone. Held d1=3.5, d2=0.99 fixed (the polish's original warm-start point); hardening frozen at
HE_30's own Stage 1 Voce result, same as every other HE_30 run.

**Result: a far bigger and cleaner lever than d1/d2 ever was.** Strong, monotonic, non-crashing
trend:

| ThresholdDamage | Fracture strain | Objective |
|---|---|---|
| 0.85 (prior baseline) | 15.40% | 0.1840 |
| 0.75 | 14.84% | 0.1414 |
| 0.65 | 14.29% | 0.1005 |
| **0.5** | **13.23%** | **0.0271** |

ThresholdDamage=0.5 lands the fracture strain essentially exactly on the real target (13.23% vs
13.27%), and the objective (0.0271) is now the best result in the entire project, including HE_0's
own locked-in 0.0688. Fit plot (`voce_hypoelastic_HE30_brozzo_threshold0p5_fit.png`) shows the
pre-fracture curve tracking the real data closely through the full yield/plateau region, with
fracture landing right where the real curve drops; post-fracture region shows the usual
(already-understood-and-accepted, see oscillation sections above) ringing, correctly excluded from
the objective.

**Not yet done:** this was a 1-D sweep holding d1/d2 at their old (pre-threshold-change) values --
a joint d1/d2/ThresholdDamage polish around (3.5, 0.99, 0.5) could plausibly tighten this further,
and it's not yet known whether going below 0.5 helps more or starts overshooting/destabilizing.
Current result is already strong enough to consider locking in as-is.

## HE_30 Brozzo — joint d1/d2/ThresholdDamage polish and FINAL locked-in result (2026-10-04)

Ran a joint `coliny_pattern_search` over all three damage parameters together, warm-started at the
threshold sweep's best point (d1=3.5, d2=0.99, ThresholdDamage=0.5, obj=0.0271), bounds d1∈[2.0,5.0]
(the old, pre-threshold-change safe cap -- not remeasured under the new threshold), d2∈[0.75,1.15]
(same window used in every prior joint polish), ThresholdDamage∈[0.35,0.75] (centered on 0.5, open
in both directions since only one point below 0.65 had been tried). Hardening frozen at HE_30's own
Stage 1 Voce result, as always. See `dakota_HE30_brozzo_d1d2threshold_polish.in` /
`driver_HE30_brozzo_d1d2threshold_polish.sh`.

**Infrastructure note, worth remembering for future multi-eval runs on this machine:** the first
launch used `asynchronous evaluation_concurrency = 2` (the pattern used throughout this project) and
badly thrashed -- two concurrent 4-rank MPI jobs (8 total processes) saturated the Docker Desktop VM
at ~770-800% CPU with essentially zero headroom (confirmed via `docker stats`: each container capped
at ~400% individually, meaning the VM's real allocation is ~4 vCPUs total, not 8 -- so 2 concurrent
4-rank jobs fight over 4 vCPUs instead of cleanly splitting 8, causing *far* worse than a 2x
slowdown). Evals 4/5 ran 3+ hours instead of the normal ~55min before being killed (confirmed healthy
the whole time via `docker stats` -- steady CPU%, flat memory, no crash -- genuinely just contention,
not a hang). Resumed via `-read_restart dakota.rst` with concurrency removed entirely (serial
execution, see `dakota_HE30_brozzo_d1d2threshold_polish_serial.in`) -- each remaining eval then ran
at the normal ~55min pace with the full VM allocation to itself. **Lesson: on this single machine,
concurrency=2 is not a safe default for two full 4-rank Peridigm jobs -- prefer serial execution
(or verify the Docker Desktop VM's actual CPU allocation first) for future multi-eval Dakota runs.**

**Result:** 19 total evaluations (3 restored from the restart cache, 16 newly run), pattern search
converged cleanly (progressively shrinking step size across all three parameters, no further
improving direction found). **Final best: eval 16 -- d1=3.5, d2=0.99, ThresholdDamage=0.51,
obj=0.0256, fracture_strain=13.23%** (real target 13.27%) -- only a ~6% objective improvement over
the threshold sweep's own already-strong 0.5 result (0.0271), and the fracture strain barely moved
(13.23% at both 0.5 and 0.51), confirming the gain is from marginally tighter pre-fracture curve
shape, not the fracture point itself. No other explored direction (d1 as low as 3.2/as high as
3.65, d2 from 0.95-1.03, ThresholdDamage from 0.46-0.54) beat this point. Fit plot:
`voce_hypoelastic_HE30_brozzo_polish_best_eval16.png`.

**This is the final, locked-in HE_30 Brozzo result:**

| Parameter | Value |
|---|---|
| InitialYieldStress | 1.027e10 (1027 MPa, Stage 1 Voce, unchanged from HE_0 except this) |
| SaturatedYieldStress | 1.35e10 (1350 MPa, unchanged from HE_0) |
| ExponentialConstant | 22.0 (unchanged from HE_0) |
| LinearConstant | 2.0e8 (200 MPa/unit-strain, unchanged from HE_0) |
| DamageParameter1 (d1) | 3.5 (identical to HE_0's own locked-in value) |
| DamageParameter2 (d2) | 0.99 (identical to HE_0's own locked-in value) |
| **ThresholdDamage** | **0.51 (vs HE_0's 0.85 -- the real embrittlement-sensitive lever for this curve)** |
| **Predicted fracture strain** | **13.23% vs real 13.27%** |
| **Objective** | **0.0256 (best result in the entire project)** |

**Reframing the original cross-curve hypothesis:** the project's original expectation (see "Broader
context" at the top of this file) was that a single damage-rate parameter (first `CriticalStretch`,
later expected to be Brozzo's `d1`) would be the dial that moves with embrittlement severity, with
everything else -- including `ThresholdDamage` -- staying fixed across concentrations. That turned
out to be wrong for Brozzo specifically: d1 and d2 transferred from HE_0 to HE_30 *unchanged*, and
the parameter that actually needed to move was `ThresholdDamage` (0.85 -> 0.51). Worth keeping in
mind if/when a third concentration is calibrated: check `ThresholdDamage` first, not d1/d2.

## HE_60 — Stage 1 Voce hardening, locked in (2026-10-04/05)

Real data for the third concentration (`calibration_stress_data_HE_60.txt` /
`calibration_strain_data_HE_60.txt`) landed 2026-10-04. Real fracture strain is 7.68% -- a big drop
from HE_30's 13.27% and HE_0's 15.37%, consistent with substantially more embrittlement. Overlay
(`real_data_HE0_HE30_HE60_comparison.png`) shows HE_60 tracking HE_0/HE_30's elastic and early-
plastic region almost exactly before diverging/fracturing much earlier -- confirming the expectation
that hardening should transfer well and the real work is in the damage parameters.

Ran Stage 1 (Voce hardening only, damage disabled, `STRESS_ONLY` objective) fresh rather than
assuming exact transfer, same practice as every prior concentration -- see
`driver_HE60_voce_hypoelastic_stage1_hardening.sh` / `dakota_HE60_voce_hypoelastic_stage1_hardening.in`,
warm-started from HE_30's own Stage 1 result. Run **serially** (no `evaluation_concurrency`) from the
start, per the concurrency lesson learned during HE_30's polish (see above) -- stayed healthy and
fast the whole way, each eval taking the normal ~50-60min.

`coliny_pattern_search` capped at 25 evals, manually stopped at **18** once gains clearly plateaued
(several combinations tried around the best point all came back worse -- same practice as HE_0
(stopped at 10/25) and HE_30 (stopped at 14/25) Stage 1). Trajectory: started at the HE_30-warm-start
point (obj=0.0318), `InitialYieldStress` lower (9.27e9, then refined to **9.77e9** -- incidentally
exactly HE_0's own original Stage 1 value) was the dominant improving direction, `SaturatedYieldStress`
slightly lower (1.3e10) also helped somewhat; `ExponentialConstant`/`LinearConstant` barely moved the
needle in either direction. **Final locked-in Stage 1 result: eval 13, obj=0.0151** (the best
Stage-1-only objective of any concentration so far):

| Parameter | Value | vs. HE_30 |
|---|---|---|
| InitialYieldStress | 9.77e9 (977 MPa) | down from 1027 MPa (same as HE_0's own value) |
| SaturatedYieldStress | 1.35e10 (1350 MPa) | unchanged |
| ExponentialConstant | 22.0 | unchanged |
| LinearConstant | 2.0e8 (200 MPa/unit-strain) | unchanged |

Fit plot: `voce_hypoelastic_HE60_stage1_locked_in.png` -- tracks the real HE_60 data tightly through
the entire measured range (0-7.68%), then continues into geometric necking/softening past that point
since damage is disabled at this stage (expected/correct Stage 1 shape, same as HE_0/HE_30).

## HE_60 — ThresholdDamage sweep (2026-10-05)

Swept `ThresholdDamage` with d1=3.5/d2=0.99 fixed (transferred unchanged from HE_0 through HE_30,
assumed to transfer again) and hardening frozen at HE_60's own Stage 1 result above. See
`driver_HE60_brozzo_threshold_sweep.sh` / `dakota_HE60_brozzo_threshold_sweep.in` (coarse pass) and
`driver_HE60_brozzo_threshold_sweep2.sh` / `dakota_HE60_brozzo_threshold_sweep2.in` (fine pass).
Real target: 7.68% fracture strain. Ran serially throughout, per the concurrency lesson from HE_30.

**Coarse pass** (0.3 / 0.2 / 0.1 -- much lower than HE_30's eventual 0.51, since HE_60 is
substantially more embrittled): 0.3 → 11.05% (obj 0.4542, still far off), 0.2 → 8.09% (obj 0.0809,
close), 0.1 → 5.49% (overshot past target). Bracketed the optimum between 0.1 and 0.2.

**Fine pass** (0.19 / 0.18 / 0.17, interpolating off the coarse pass's 0.1-0.2 slope): **0.19 → 7.80%
(obj 0.0540) is the local optimum** -- 0.18 → 7.49% (obj 0.0619) and 0.17 (obj 0.0975) both worse, a
clean monotonic trend either side of 0.19. Fracture strain at 0.19 (7.80%) is only 0.12 points from
the real 7.68% target.

**Current best HE_60 point: d1=3.5, d2=0.99, ThresholdDamage=0.19, obj=0.0540.** No crashes anywhere
in either sweep -- HE_60's crash boundary (if any) at these much-lower threshold values remains
uncharacterized, but wasn't hit. **Next step (not yet started):** joint d1/d2/ThresholdDamage polish
warm-started at this point, same procedure as HE_30's final polish (see "HE_30 Brozzo -- joint
d1/d2/ThresholdDamage polish" above) -- run serially from the start.

## Known open items / gaps
- Sim peak strain (~8%) still comes in earlier than real data's peak (~12%) — not yet re-examined
  since adding either damage model; may be worth revisiting once fracture-matching is locked in.
- Post-fracture oscillation's real cause is still unresolved — six separate fixes tried (Contact,
  Hard Cutoff, Fracture Plane severance, Bond Associated correspondence, Bond-Level Micro-Damping,
  and the 2026-10-07 ThresholdDamage x Hard Cutoff sweep; see the dedicated sections above) all
  failed or made it worse. A direct ParaView observation during the 2026-10-07 revisit (a
  longitudinal elastic wave reverberating between the clamp end and the damaged neck, never
  decaying) sharpened the diagnosis: the grips' kinematic (never-stopping) boundary condition keeps
  re-injecting energy, and the damaged neck acts as a partially-reflecting junction, with nothing in
  the system to dissipate the traveling wave — explaining why every damage-law-side intervention
  (including the full Hard Cutoff threshold sweep) could only ever change the size of the initial
  shock, never the wave's persistence. Investigation deliberately closed out as of 2026-10-01 (user
  call), revisited once more 2026-10-05/07 without changing that conclusion, rather than pursued
  further (e.g. semi-implicit damping, which the Bond-Level Micro-Damping section identifies as the
  theoretically correct next step — now further reinforced by the wave-reverberation read — but a
  materially bigger integrator change). Current truncate-before-filter plotting workaround remains
  the only mitigation, and it's sufficient — it already fully excludes the post-fracture region from
  `compute_objective.py`'s scoring, so this has no effect on the locked-in calibration results.
- Nothing in `peridigm-masoud/` (including this whole `calibration/` directory) is committed to git
  yet — `git status` in the repo root shows it as untracked. Worth committing if this result is
  meant to be kept.

## Suggested next steps to discuss
1. HE_0's Brozzo calibration (d1=3.5, d2=0.99, ThresholdDamage=0.85, obj=0.0688) is now locked in as
   the baseline result — decide whether to push further (e.g. open ThresholdDamage as a third free
   parameter, tighter search around the d2∈[0.96,1.0] cluster) or move on to the next concentration.
2. Replicate the Brozzo calibration procedure on HE_30 (and any further concentrations), the same
   way Johnson-Cook/Critical-Stretch were staged earlier — hardening frozen at its already-converged
   value, only damage parameters (d1/d2) recalibrated per concentration.
3. A Contact/Self-Contact model has been tried and ruled out as the fix for post-fracture
   oscillation (2026-09-30 — see above). If this is still worth chasing, the next angle would be
   investigating correspondence-material shape-tensor stability under heavy local damage, not
   geometric contact.
4. Commit the calibration setup and results to git.
5. Once all concentrations are calibrated, compare how d1/d2 move with embrittlement severity —
   analogous to the `CriticalStretch`-vs-concentration relationship originally hypothesized for the
   Critical Stretch damage model, this is the cross-curve comparison the whole staged-calibration
   infrastructure was built for.
