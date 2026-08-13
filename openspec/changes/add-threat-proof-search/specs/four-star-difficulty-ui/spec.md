## ADDED Requirements

### Requirement: Four distinct player difficulty levels
The iPhone and iPad player interfaces SHALL expose four difficulty choices ordered as four, three, two, and one stars. Four stars SHALL invoke the opening-book-disabled proof-guided profile, while three stars SHALL invoke the frozen legacy advisor directly. Existing two-star and one-star behavior SHALL remain unchanged.

#### Scenario: Player selects four stars
- **WHEN** the player selects four stars and requests an AI move
- **THEN** the controller SHALL invoke the proof-guided four-star entry with opening-book lookup disabled

#### Scenario: Player selects three stars
- **WHEN** the player selects three stars and requests an AI move
- **THEN** the controller SHALL invoke the frozen legacy `harsh_analysisboard` path without enabling proof search

### Requirement: Saved difficulty compatibility
Persisted difficulty values SHALL retain their previous meaning: 0 is three stars, 1 is two stars, and 2 is one star. Four stars SHALL use the new persisted value 3. The UI-to-persisted-value mapping SHALL be identical on iPhone and iPad.

#### Scenario: Existing three-star save is loaded
- **WHEN** an existing save contains difficulty value 0
- **THEN** the UI SHALL select the three-star segment and gameplay SHALL continue to use the frozen legacy advisor

#### Scenario: Four-star save is loaded
- **WHEN** a save contains difficulty value 3
- **THEN** the UI SHALL select the four-star segment and gameplay SHALL use the no-book proof-guided profile
