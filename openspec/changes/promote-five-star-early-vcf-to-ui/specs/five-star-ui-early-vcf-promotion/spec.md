## ADDED Requirements

### Requirement: Playable five-star uses the frozen early micro-VCF profile
The system SHALL bind the playable five-star difficulty to `five-star-early-micro-vcf-sentinel-candidate@5.8.1-early-micro-vcf-adaptive-16k-80ms-2a` without altering that profile's frozen parameters.

#### Scenario: Phone player selects five stars
- **WHEN** an iPhone player selects the five-star difficulty and the AI moves
- **THEN** the shared five-star analysis entry SHALL select the frozen 5.8.1 early micro-VCF profile

#### Scenario: Tablet player selects five stars
- **WHEN** an iPad player selects the five-star difficulty and the AI moves
- **THEN** the same shared five-star analysis entry SHALL select the frozen 5.8.1 early micro-VCF profile

#### Scenario: Promoted profile snapshot
- **WHEN** the playable five-star profile is assigned
- **THEN** its snapshot SHALL identify version `5.8.1-early-micro-vcf-adaptive-16k-80ms-2a`, adaptive depth 5/7, 16,000 nodes, 80 ms, and at most two early alternatives

### Requirement: Lower difficulties and persisted mappings remain unchanged
The promotion SHALL NOT change the four-star or lower-level analysis bindings, difficulty ordering, controller routing, or persisted difficulty values.

#### Scenario: Player selects four stars
- **WHEN** a player selects four stars after the promotion
- **THEN** the AI SHALL continue using the frozen proof-guided no-book four-star profile with no early micro-VCF promotion

#### Scenario: Existing difficulty value is loaded
- **WHEN** an existing persisted difficulty value is loaded after promotion
- **THEN** it SHALL resolve to the same difficulty level as before promotion

### Requirement: Exact 5.4.1 remains available for rollback and control
The system SHALL preserve `fc_profile_five_star_proof_engine_candidate()` as exact `5.4.1-transactional-deadline-root-parallel-5s` and SHALL keep it selectable outside the playable five-star binding.

#### Scenario: Benchmark selects exact control
- **WHEN** a benchmark requests the exact five-star control
- **THEN** it SHALL receive unchanged 5.4.1 rather than the promoted 5.8.1 profile

#### Scenario: Product rollback is required
- **WHEN** the playable five-star binding must be rolled back
- **THEN** changing only the shared five-star assignment back to the exact 5.4.1 factory SHALL restore the prior product selection

### Requirement: Promotion is regression verified
The promotion SHALL include automated checks that distinguish the shared playable binding from retained research and control factories.

#### Scenario: Source binding regression runs
- **WHEN** the UI-binding regression inspects the product source
- **THEN** it SHALL verify the 5.8.1 factory inside the five-star method, reject 5.4.1 inside that method, verify the four-star factory remains unchanged, and verify both controllers still call the shared five-star method

#### Scenario: Existing AI verification runs
- **WHEN** promotion verification completes
- **THEN** the existing AI/profile test suite and available application compilation SHALL pass without changing the profile identities
