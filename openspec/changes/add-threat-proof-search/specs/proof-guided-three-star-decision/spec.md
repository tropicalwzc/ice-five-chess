## ADDED Requirements

### Requirement: Evidence-driven override of the legacy advisor
The production four-star decision SHALL use the frozen legacy three-star move as its default suggestion and SHALL use the formally demonstrated opening-book-disabled proof profile. It SHALL override that suggestion only for an immediate legal win, a complete necessary defense, a verified own forced win, or a verified opponent forced win after the default move for which a safer legal alternative is found. The player-facing three-star difficulty SHALL continue to use the frozen legacy advisor directly.

#### Scenario: New move has a verified win
- **WHEN** the proof verifier accepts a forced-win certificate for a move different from the default suggestion
- **THEN** the production decision SHALL select the verified winning move

#### Scenario: Search remains unknown
- **WHEN** proof search exhausts its budget without proving the default unsafe or another move winning
- **THEN** the production decision SHALL retain the legal and immediately safe default suggestion

### Requirement: Proof-guided defensive search
Before accepting a non-winning default move in a tactically eligible position, the decision layer SHALL search for an opponent VCF/VCT after that move. If it proves the default loses in scope, it SHALL evaluate legal defensive alternatives and may override only with a move that removes that proof or creates a faster verified own win.

#### Scenario: Legacy move permits opponent VCF
- **WHEN** the opponent has a verified VCF after the legacy suggestion and an alternative move has no opponent forced win in the same completed search scope
- **THEN** the decision SHALL reject the legacy suggestion and select the verified alternative

#### Scenario: All moves are proven losing
- **WHEN** every legal candidate permits an opponent verified win
- **THEN** the result SHALL expose a proven-loss classification and choose deterministically according to longest verified survival rather than claim safety

### Requirement: Proof-safe user randomness
User-game randomness SHALL be limited to at least two legal candidates with the same completed proof class and tactical obligation. A shorter proven win SHALL rank ahead of longer wins, and a proven-safe-in-scope candidate SHALL rank ahead of an unknown or proven-losing candidate. Evaluation and benchmark modes SHALL support deterministic best selection.

#### Scenario: Multiple equivalent proven wins
- **WHEN** two or more moves have verified wins of the same shortest distance and no unique higher-ranked move exists
- **THEN** user mode MAY select among them using the isolated seeded random source

#### Scenario: Unique necessary defense
- **WHEN** exactly one legal move satisfies every immediate or verified required defense
- **THEN** every random seed SHALL select that move

### Requirement: Decision observability and board integrity
Each decision SHALL report default source, default coordinates, selected coordinates, override reason, book identifier if used, proof result and distance, searched threat class, certificate identifier, node/depth/budget statistics, and random-selection metadata. Analysis SHALL leave the caller's board unchanged.

#### Scenario: Proof overrides an opening move
- **WHEN** a book suggestion is replaced because it permits a verified opponent win
- **THEN** telemetry SHALL identify the book entry, rejected move, replacement, proof class, and certificate identifier
