## ADDED Requirements

### Requirement: Only the frozen production five-star version is selectable
The system SHALL expose only the existing 5.8.1 five-star profile and SHALL preserve its effective parameters, identity and supported game behavior.

#### Scenario: Production profile is constructed
- **WHEN** the app or supported benchmark requests five stars
- **THEN** it receives `5.8.1-early-micro-vcf-adaptive-16k-80ms-2a` with the same complete parameter snapshot as before cleanup

#### Scenario: Retired research version is requested
- **WHEN** a caller requests a removed five-star version
- **THEN** supported tooling rejects that version instead of silently substituting another profile

### Requirement: Shared dependencies and historical evidence survive cleanup
The system SHALL preserve lower difficulties, required shared proof/rule/corpus helpers and historical spec/evidence files. It SHALL remove dedicated obsolete research factories, analysis routes and executable experiments.

#### Scenario: Existing saved difficulty is loaded
- **WHEN** any supported difficulty value is restored
- **THEN** it retains its previous meaning and lower-level algorithm binding

#### Scenario: Historical reproduction is needed
- **WHEN** a researcher reads an earlier spec
- **THEN** its original evidence remains available and the cleanup documentation explains that retired executables require their historical Git revision

### Requirement: Cleanup is regression verified
The system SHALL verify profile equivalence, production tactical behavior, shared algorithm tests and application compilation after removing obsolete branches.

#### Scenario: Verification runs
- **WHEN** cleanup is considered complete
- **THEN** the frozen parameter comparison, current algorithm regression tests, UI-binding check and available app tests/builds pass
