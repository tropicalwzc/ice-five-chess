## ADDED Requirements

### Requirement: 5.6 alias is reproducible

The system SHALL identify model `5.6` as the frozen
`5.6.2-white-v541-black-v521-overlap-aware-parallel8-5s` candidate and SHALL
retain immutable references to the 5.6.0 serial control, 5.6.1 parallel
candidate, source hashes, and report bundle hashes.

#### Scenario: Historical evidence remains addressable

- **WHEN** a report or replay requests 5.6.0 or 5.6.1
- **THEN** it resolves to the historical profile and bundle rather than the
  5.6 alias.

### Requirement: Parallel utilization is measured by actual work

The benchmark SHALL record launched workers, concurrent workers, root jobs,
overlap groups, completed jobs, and fallbacks.  A configured worker cap SHALL
not be reported as evidence of parallel search.

#### Scenario: Multi-root position reaches the dispatcher

- **WHEN** at least two independent root gains are available before the shared
  deadline
- **THEN** the diagnostic SHALL show more than one worker or explicitly record
  why the batch fell back, and every gain SHALL retain an independent result.

### Requirement: Escape proof uses the same parallel contract

Loss-aware escape verification SHALL use a shared absolute deadline and
reserved budget, private mutable proof sessions, legal best-completed fallback,
and independent replay of any accepted scoped disproof.

#### Scenario: Default move has a verified opponent win

- **WHEN** multiple legal escape candidates are generated
- **THEN** they may be searched concurrently, but a completed scoped disproof
  for one candidate SHALL not be generalized to another candidate.

### Requirement: Forbidden legality is oracle-equivalent

Any incremental forbidden-black legality cache SHALL match the original
full-board oracle for every tested position and move, including exact-five,
overline, double-four, double-three, edge, and crossing-line cases.

#### Scenario: Cache disagrees with reference

- **WHEN** a randomized or adversarial fixture finds a mismatch
- **THEN** the cached path SHALL be disabled for that position and the mismatch
  SHALL invalidate the candidate evaluation.

### Requirement: Opening protocols are reported separately

The benchmark SHALL distinguish raw forbidden/no-swap diagnostics from formal
RIF exchange/Swap2/Taraguchi-style protocols and SHALL report candidate color,
stone color, and first-player results without pooling rule cells.

#### Scenario: Natural-prefix suite is used

- **WHEN** a game starts from an official-game prefix without swap metadata
- **THEN** the report SHALL label it as natural-prefix/no-swap and SHALL not
  interpret its black score as a formal Renju opening-balance result.
