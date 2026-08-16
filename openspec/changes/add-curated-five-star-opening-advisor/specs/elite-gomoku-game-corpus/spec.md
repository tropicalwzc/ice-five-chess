## ADDED Requirements

### Requirement: Elite and auditable source admission
Every corpus source SHALL identify the official organization or tournament organizer, exact URL, retrieval date, checksum or stable record identifier, event/year, rule, board size, player identities or engine division, and redistribution status. Only explicitly compatible high-level competitive records SHALL contribute runtime move evidence. Freestyle and Renju/forbidden evidence SHALL be partitioned. A 20x20 Freestyle record MAY contribute only compact early-opening evidence that is demonstrably edge-independent and losslessly embeddable on 15x15; it SHALL NOT contribute midgame or edge-sensitive evidence.

#### Scenario: Anonymous or low-quality game is encountered
- **WHEN** a record lacks verifiable competitive level, identity, rule compatibility, or provenance
- **THEN** the importer SHALL reject it from the runtime corpus and record the exclusion reason

#### Scenario: Rights are unclear
- **WHEN** an elite database permits research access but not confirmed redistribution
- **THEN** raw games SHALL NOT be bundled and only permitted aggregated factual statistics plus attribution SHALL be emitted

### Requirement: Legal normalization and independent support
The importer SHALL replay every admitted game, reject illegal or terminally inconsistent records, canonicalize all eight symmetries, deduplicate equivalent records, and track independent game, event, and source-family support for each position continuation.

#### Scenario: Same game appears in multiple mirrors or archives
- **WHEN** duplicate records canonicalize to the same game identity
- **THEN** they SHALL contribute one game observation while retaining all provenance references

### Requirement: Tournament assignments are not advice by default
Start-position packages designed to balance tournament play SHALL remain benchmark material and SHALL NOT enter the five-star move corpus unless the continuation is independently supported by admitted completed games.

#### Scenario: Official opening line has no elite game support
- **WHEN** a coordinate sequence comes only from an event opening assignment package
- **THEN** it SHALL NOT be offered as five-star runtime advice

### Requirement: Rule-separated exact and local support
The importer SHALL emit separate rule partitions and SHALL aggregate both exact positions and candidate-centered local opening shapes from actual elite decisions. Local evidence SHALL retain side, boundary class, independent-game/event/source counts, candidate consensus, source board size, and minimum local-stone support.

#### Scenario: Runtime rule differs from source rule
- **WHEN** a candidate is supported only by the other rule partition
- **THEN** lookup SHALL return no advice from that evidence

#### Scenario: A 20x20 opening is compact and central
- **WHEN** every stone and candidate in the bounded early shape fits 15x15 with the frozen target margin and is separated from the source edge
- **THEN** the shape MAY contribute Freestyle support with its original board size recorded

#### Scenario: A 20x20 position depends on extra board space
- **WHEN** the position is beyond the opening horizon, edge-sensitive, or cannot be embedded with the frozen margin
- **THEN** the importer SHALL exclude it and record the reason
