## ADDED Requirements

### Requirement: Four-star-first conservative fusion
Five-star SHALL compute the book-blind four-star decision and treat it as the authoritative fallback. A corpus move SHALL replace it only after passing legality, immediate-win/defense obligations, comparable proof search, and frozen support/quality thresholds.

#### Scenario: Corpus candidate is unproven or incomparable
- **WHEN** the corpus candidate has unknown, budget-exhausted, certificate-invalid, or worse evidence than the four-star move
- **THEN** five-star SHALL play the original four-star move

#### Scenario: Corpus candidate is demonstrably superior
- **WHEN** a supported corpus candidate has a strictly better verified proof class or distance under the same completed scope
- **THEN** five-star SHALL select it and record the evidence and corpus provenance

### Requirement: Free opponent play with exact and local matching
The advisor SHALL match the actual current position rather than require a stored move sequence. It SHALL impose no move on the opponent. Exact-position evidence SHALL have priority; after a deviation, separately supported rule/side/boundary-compatible local opening evidence MAY advise a move while ignoring only stones outside its frozen window. If neither tier qualifies, five-star SHALL fall back to four-star.

#### Scenario: Opponent deviates from every recorded continuation
- **WHEN** the current legal position has no qualifying corpus entry
- **THEN** five-star SHALL behave identically to book-blind four-star for that decision

#### Scenario: Opponent deviates outside a supported local shape
- **WHEN** the exact position is absent but a high-confidence local signature still matches and all tactical gates pass
- **THEN** five-star MAY evaluate that candidate against the four-star baseline and SHALL record a local-match lookup

#### Scenario: Local advice conflicts with tactics
- **WHEN** the candidate is illegal, misses an immediate obligation, permits an immediate loss, or has a verified opponent forced win
- **THEN** five-star SHALL reject it regardless of corpus support

### Requirement: Strict runtime observability
Every five-star decision SHALL report the four-star move, corpus candidates and support counts, accepted/rejected candidate, rejection reason, proof comparison, source identifiers, random-equivalence state, and final move.

#### Scenario: Book advice is rejected
- **WHEN** a corpus candidate fails any gate
- **THEN** telemetry SHALL identify the failed gate without labeling the candidate safe or advantageous
