## ADDED Requirements

### Requirement: Auditable professional opening sources
Every bundled opening SHALL have provenance containing its source URL, source organization, retrieval date, rule set, board size, event/year or opening-set name, original identifier, move prefix, and redistribution/license review status. Sources SHALL be restricted to reputable public tournament or federation resources approved during implementation.

#### Scenario: Gomocup official opening set
- **WHEN** an opening is derived from a Gomocup annual package that publishes complete results and expert-selected openings
- **THEN** its manifest SHALL identify the exact annual results page, downloadable package, checksum, league rule, and opening index

#### Scenario: Source rights are unclear
- **WHEN** a source such as an online game database does not clearly authorize redistribution
- **THEN** its records SHALL be used only for research or frequency validation and SHALL NOT be bundled until permission or an acceptable license is recorded

### Requirement: Rule and board compatibility
The library SHALL keep freestyle 15x15, standard, Renju, and other rule families separate. Runtime lookup and benchmark selection SHALL use only entries compatible with the active board size, forbidden-move behavior, win condition, and side-to-move semantics.

#### Scenario: Forbidden moves are disabled
- **WHEN** the application is playing its 15x15 no-forbidden rule
- **THEN** the library SHALL NOT silently select a Renju-only or exact-five opening as though it were a freestyle entry

#### Scenario: Imported opening is already terminal
- **WHEN** an opening prefix contains an illegal move, a winner, inconsistent alternating colors, or no legal continuation
- **THEN** validation SHALL reject it before it enters the library or benchmark schedule

### Requirement: Symmetry normalization and duplicate control
The import pipeline SHALL normalize all eight rotations/reflections of a 15x15 opening to a canonical identity while retaining reversible transforms. Duplicate and near-duplicate prefixes SHALL be reported so that a frequently copied line cannot dominate selection.

#### Scenario: Rotated duplicate
- **WHEN** two source records contain the same prefix under rotation or reflection
- **THEN** the library SHALL store one canonical opening identity with multiple provenance references rather than treat them as independent openings

### Requirement: Reproducible but varied opening selection
Opening lookup SHALL offer more than one safe continuation where the curated source provides alternatives. User mode SHALL use an isolated per-game seed to vary equivalent book continuations, while evaluation mode SHALL deterministically reproduce the same entry, transform, and continuation from a recorded seed.

#### Scenario: Repeated user games
- **WHEN** multiple compatible common openings and continuations exist
- **THEN** user games using independently derived seeds SHALL be capable of following different openings without bypassing tactical safety checks

### Requirement: Offline immutable runtime library
All accepted opening records and source manifests SHALL be versioned local assets. The application SHALL require no network access to choose a move, and a benchmark SHALL record the exact library version and checksum.

#### Scenario: Device has no network
- **WHEN** a user starts a game while offline
- **THEN** opening lookup SHALL behave identically to online operation for the same library version and seed

