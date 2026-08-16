## ADDED Requirements

### Requirement: Five player difficulty choices
Both iPhone and iPad interfaces SHALL display difficulty in descending order as five, four, three, two, and one stars. Five stars SHALL use the curated corpus advisor over four-star; four stars SHALL remain completely corpus-blind.

#### Scenario: Player selects five stars
- **WHEN** a player selects the five-star segment
- **THEN** AI moves SHALL use the conservative five-star profile and its offline corpus

#### Scenario: Player selects four stars
- **WHEN** a player selects the four-star segment
- **THEN** AI moves SHALL use the frozen no-book proof-guided profile without corpus lookup

### Requirement: Backward-compatible persistence
Existing values 0, 1, 2, and 3 SHALL retain their meanings of three, two, one, and four stars respectively; five stars SHALL persist as value 4.

#### Scenario: Existing four-star save is loaded
- **WHEN** persisted difficulty value 3 is loaded after the upgrade
- **THEN** the four-star segment SHALL be selected and corpus access SHALL remain disabled
