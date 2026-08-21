## ADDED Requirements

### Requirement: Universal opponent-forcing acceptance guard
The candidate five-star SHALL audit every proposed legal move that lacks an independently verified own-win certificate for an opponent forced win after that move. Guard eligibility SHALL NOT depend on the frozen four-star proof result, corpus availability, ordinary versus mandatory-defense tactical class, opening identity, opponent identity, or prior game outcome.

#### Scenario: Frozen four-star returns unknown
- **WHEN** frozen four-star proposes a move whose opponent-after-move proof is `unknown`
- **THEN** candidate five-star SHALL use its own guarded defensive scope before accepting the move and SHALL NOT treat the inherited `unknown` as a completed defense

#### Scenario: Position has no corpus match
- **WHEN** a guarded position has no eligible corpus entry
- **THEN** the candidate SHALL complete or honestly exhaust the opponent-forcing guard before returning rather than bypassing it at the no-corpus exit

#### Scenario: Own win is independently verified
- **WHEN** the proposed move immediately wins legally or begins an own forced-win certificate accepted by the independent verifier
- **THEN** the candidate MAY accept that move without an additional opponent-after-move guard and SHALL record the verified-own-win skip reason

### Requirement: VCF-first rule-aware defensive scope
The opponent-forcing guard SHALL run strict rule-aware VCF before any broader VCT query. A verified opponent VCF or VCT SHALL require a replayable certificate; a negative result SHALL retain its completed search class and scope, and unfinished enumeration, deadline, overflow, or certificate failure SHALL return `unknown`.

#### Scenario: Shallow opponent continuous-four win
- **WHEN** placing the proposed move gives the opponent a verified continuous-four win within the guarded VCF scope
- **THEN** the candidate SHALL classify the proposed move as a verified loss and SHALL NOT accept it while a better completed candidate class is available

#### Scenario: VCF is disproved but VCT signal remains
- **WHEN** the VCF scope completes with no forced win and the resulting position contains a qualifying opponent VCT root or dependency signal with reserved resources remaining
- **THEN** the guard SHALL continue with bounded VCT and SHALL preserve separate VCF and VCT statuses in telemetry

#### Scenario: Guard exhausts before completion
- **WHEN** the defensive query reaches a node, time, memory, enumeration, or certificate limit without a completed result
- **THEN** it SHALL report `unknown`, restore the original board, and SHALL NOT label the move safe or disproved

### Requirement: Mandatory-defense continuation audit
When the side to move must answer an immediate opponent win, candidate five-star SHALL preserve every generated legal immediate defense and SHALL audit the opponent forcing continuation after each examined defense under the same guarded scope. Blocking the current immediate win alone SHALL NOT count as proof that the continuation is safe.

#### Scenario: Preferred block permits a continuation VCF
- **WHEN** the provisional mandatory block removes the current immediate win but permits a verified opponent VCF and another legal block completes a comparable opponent disproof
- **THEN** the candidate SHALL select the completed defensive block and retain the mandatory-defense tactical obligation in telemetry

#### Scenario: Mandatory defenses remain unresolved
- **WHEN** no examined mandatory defense completes a disproof and at least one immediately safe defense remains `unknown`
- **THEN** the candidate SHALL prefer an `unknown` defense over a verified loss while labeling the decision `unknown`

#### Scenario: Every mandatory defense is verified losing
- **WHEN** every completed legal immediate defense permits an opponent verified win
- **THEN** the candidate SHALL select the longest verified survival and SHALL expose that all examined defenses are verified losses without claiming a global proven loss unless coverage is complete

### Requirement: Certificate-directed defensive widening and proof-class ordering
When a proposed move permits a verified opponent forcing win, candidate five-star SHALL search certificate gain, cost, rest, dependency, interruption, and qualifying counter-threat points before progressively widening through tactical, ordinary, and remaining legal alternatives within the guarded budget. It SHALL rank own verified win, comparable completed opponent disproof, immediately safe `unknown`, and opponent verified win in that order, with verified losses ordered by longest survival.

#### Scenario: Certificate blocker lies outside heuristic width
- **WHEN** a legal certificate dependency outside the ordinary candidate limit completes a comparable opponent disproof
- **THEN** the candidate SHALL be able to select it and SHALL record its certificate-directed widening stage

#### Scenario: Unknown alternative competes with verified loss
- **WHEN** the proposed move is a verified loss and an immediately safe legal alternative remains `unknown` within the guarded budget
- **THEN** the candidate SHALL select the unknown alternative and SHALL NOT describe it as a proven defense

#### Scenario: Corpus move conflicts with completed defense
- **WHEN** corpus advice differs from a completed guarded defense and lacks an own verified win or equal-or-broader completed opponent disproof
- **THEN** the corpus move SHALL be rejected and the completed defense SHALL remain selected

### Requirement: Non-starvable bounded defensive resources
The candidate profile SHALL reserve deterministic node resources and a bounded time opportunity for the opponent-forcing guard inside the existing aggregate decision ledger and absolute deadline. Earlier own-attack, quiet, corpus, or inconclusive proof work MUST NOT consume the reserved guard nodes, and the reservation MUST NOT extend the internal 4.5-second or player-visible 5-second ceilings.

#### Scenario: Earlier own search consumes its allocation
- **WHEN** an own VCF/VCT or quiet stage exhausts all resources available to that earlier stage without a verified own win
- **THEN** the defensive reservation SHALL remain available for the opponent guard subject to the unchanged absolute deadline

#### Scenario: Absolute deadline interrupts guard stage
- **WHEN** the absolute decision deadline is reached during a defensive audit or widening batch
- **THEN** the candidate SHALL unwind workers, restore boards and ledger resources, commit only the last completed legal result, and return within the five-second hard ceiling

#### Scenario: Guard is ineligible after verified own win
- **WHEN** an independently verified own win makes the guard unnecessary
- **THEN** the candidate SHALL release unused reserved resources without changing the proof result or granting a new deadline

### Requirement: Defensive telemetry and profile isolation
The candidate SHALL expose guard eligibility/skip reason, proposed and selected moves, per-class status/distance/nodes/time, certificate verification, candidates and stages examined, completed disproof/unknown/verified-loss counts, reserved and consumed resources, rollback, and avoided verified-loss information. One-star through frozen four-star and the current five-star control SHALL remain behaviorally selectable without this guard.

#### Scenario: Candidate avoids a verified opponent VCF
- **WHEN** the provisional move has a verified opponent VCF and the selected alternative completes a comparable disproof
- **THEN** telemetry SHALL identify both coordinates, proof classes, distance, certificate validity, escape stage, and the avoided verified-loss event

#### Scenario: Lower difficulty is selected
- **WHEN** one-star, two-star, legacy three-star, or frozen four-star analyzes the same board
- **THEN** it SHALL use its frozen profile without defensive-guard stages, resource fields, or move-selection changes

#### Scenario: Evaluation repeats a fixed position
- **WHEN** the same board, side, rule, candidate profile, and deterministic budget are evaluated repeatedly without reaching the wall deadline
- **THEN** the selected move, completed proof class, guard stage, and independently verified certificate SHALL be reproducible
