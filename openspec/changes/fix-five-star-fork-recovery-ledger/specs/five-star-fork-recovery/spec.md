## ADDED Requirements

### Requirement: Candidate fork risk is evaluated after the candidate is placed

The five-star research path SHALL evaluate a legal candidate on a private
position after placing the current side's stone.  Unless that move is an own
win, it SHALL count legal immediate winning replies for the opponent and mark
the candidate as fork-risky when at least two replies exist.  A bounded or
budget-exhausted probe SHALL be reported as incomplete/unknown rather than as
proof that the candidate is safe or that the position is lost.

#### Scenario: A candidate that creates two opponent wins is rejected when a safe alternative exists

- **WHEN** a position contains one legal candidate that leaves two opponent immediate wins and another legal candidate that leaves zero or one
- **THEN** the five-star recovery result selects the safe alternative before deeper proof certainty is available
- **AND** telemetry records the rejected candidate as fork-risky and the selected candidate's opponent reply count

#### Scenario: Own win and mandatory defense retain tactical priority

- **WHEN** a legal candidate immediately wins for the current side or is required to remove an existing opponent immediate win
- **THEN** the candidate is not rejected merely because ordinary fork ordering would rank it lower
- **AND** the result preserves the existing immediate-win or mandatory-defense tactical class

#### Scenario: Forbidden legality is applied to fork replies

- **WHEN** forbidden-black mode is enabled and an apparent opponent winning reply is illegal under the established rule oracle
- **THEN** that reply is excluded from the fork count
- **AND** the selected result remains legal under the same rule mode

### Requirement: Recovery SHALL preserve candidates across an unknown or failed four-star handoff

When the four-star hint is missing, invalid, or incomplete, the five-star path
SHALL initialize an ordered candidate set using its deterministic candidate
generator and run the fork probe before selecting a fallback.  A first
canonical legal coordinate MAY be used only when no usable generated candidate
can be obtained within the decision budget.  When proof is unknown, a
completed fork-safe legal candidate SHALL remain selectable and the decision
status SHALL remain `UNKNOWN_OR_DEADLINE`.

#### Scenario: Invalid four-star handoff does not collapse to one candidate

- **WHEN** the four-star handoff is invalid and the board has multiple legal generated candidates
- **THEN** five-star recovery evaluates the ordered candidates rather than publishing the first canonical legal coordinate as its only candidate
- **AND** the result records that the handoff failed without marking a bounded candidate list as a verified loss

#### Scenario: Unknown proof retains the best completed fork-safe candidate

- **WHEN** proof of the baseline or of the opponent response reaches its budget without a verified certificate
- **THEN** recovery may select the highest-ranked completed fork-safe legal candidate
- **AND** the result status is `UNKNOWN_OR_DEADLINE`, not `VERIFIED_LOSS`

#### Scenario: No fork-safe candidate is found

- **WHEN** every examined legal candidate is fork-risky or the probe cannot complete before the deadline
- **THEN** recovery selects the candidate with the smallest known opponent reply count, followed by stable score and coordinate order
- **AND** telemetry marks candidate coverage incomplete when the full legal universe was not examined

### Requirement: Every non-terminal recovery result SHALL be legal and deterministic

The five-star path SHALL return a legal move whenever at least one legal move
exists.  It SHALL use an explicit no-legal-move state only when the complete
legality check finds no legal move, and SHALL not infer `VERIFIED_LOSS` from an
empty or relevance-truncated candidate list.  Equal candidate annotations
SHALL resolve in a fixed order independent of worker completion order.

#### Scenario: Budget exhaustion still returns a legal move

- **WHEN** the decision deadline or proof budget expires while at least one legal move remains
- **THEN** the result contains a legal fallback move and an unknown/deadline status
- **AND** the fallback is not represented by an uninitialized sentinel coordinate

#### Scenario: A genuinely full board reports no legal move

- **WHEN** the complete legality scan finds no legal move for the side to play
- **THEN** the result reports the explicit no-legal-move state
- **AND** no fallback move is published as a playable coordinate

#### Scenario: Replaying the same position is stable

- **WHEN** the same board, side, rule mode, profile, and seed are analyzed repeatedly
- **THEN** the selected move, candidate tie-break, and decision classification are identical
- **AND** any worker-count difference is recorded as telemetry rather than used as an ordering tie-break
