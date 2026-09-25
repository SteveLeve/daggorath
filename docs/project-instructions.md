# Dungeons of Daggorath Mobile Preservation Project

## Project Mission

Create a faithful modern implementation of the original **Dungeons of Daggorath** for Android and iOS, preserving the original game's mechanics, presentation, timing, difficulty, command semantics, and overall experience while adapting input and display behavior appropriately for modern touch devices.

The initial product is a preservation-oriented implementation, not a redesign.

After a faithful implementation exists and has been validated, optional extensions, alternate modes, accessibility features, presentation enhancements, or new gameplay may be considered. Those additions must remain architecturally separate from the canonical original-game ruleset.

## Core Product Principles

### 1. Preservation before improvement

The original game is the specification.

Do not change gameplay merely because a mechanic appears inconvenient, primitive, unbalanced, obscure, or inconsistent with modern design conventions.

Examples include:

- combat timing
- stamina and exhaustion
- monster behavior
- the five fixed original dungeon layouts and their historical generation
- item behavior
- command syntax
- progression
- difficulty
- torch mechanics
- sound cues
- heartbeat mechanics
- visibility
- inventory limitations
- historical quirks

Where original behavior is uncertain, research and document it rather than silently choosing a more modern behavior.

### 2. Original Mode must remain canonical

The application should ultimately distinguish between:

- **Original Mode** — faithful reproduction of the historical game.
- **Extended or Enhanced Modes** — optional future functionality.

Original Mode should not gradually accumulate gameplay modifications.

Future enhancements should use extension points, alternate rulesets, presentation layers, or configuration rather than modifying canonical game behavior.

Original Mode must reproduce the same five maps as the original game and preserve the original level populations, scheduled creature regeneration, RNG behavior, and event ordering. The original uses fixed level seeds to build its maps; the time-dependent RNG advance occurs after maze construction. Do not introduce a player-selected seed or newly randomized maps into Original Mode. Optional random maps belong in a separate enhanced mode.

### 3. Separate simulation from presentation

The game simulation must be independent of SDL, Android, iOS, rendering APIs, touch controls, and other platform-specific systems.

Preferred conceptual architecture:

```text
Platform / Application
        |
Input adapters
        |
Command interface
        |
Daggorath Core
        |
Game state / simulation
        |
Presentation state
        |
Renderer / Audio
```

The core should be usable headlessly for testing.

Conceptually, code should support behavior similar to:

```cpp
Game game(Mode::Original);

game.execute(Command::MoveForward);
game.tick(elapsedTime);

GameState state = game.state();
RenderState view = game.renderState();
```

The exact API may differ after the archaeology/design phase.

### 4. Preserve the command model

The original command language should remain a first-class interface to the game.

Touch controls should normally translate into game commands rather than bypassing the command system.

For example:

```text
Touch Forward -> MOVE
Swipe/Turn Left -> TURN LEFT
Tap held weapon -> ATTACK LEFT/RIGHT
```

This permits:

- authentic keyboard play
- mobile touch controls
- automated scripts
- replay testing
- deterministic test fixtures
- potential controller support
- debugging through command traces

### 5. Determinism and testability are first-class requirements

Where practical, Original Mode behavior should be reproducible from:

- initial state
- historical level seeds and RNG state, without a user-selected game seed
- timestamped input (including keystrokes when their timing matters)
- elapsed simulation time and scheduler state

The architecture should support automated behavioral tests for:

- fixed historical maps and level-entry regeneration
- timed creature spawning and event order
- movement
- combat
- creature behavior
- stamina
- heartbeat
- torch lifecycle
- item interaction
- inventory
- magic
- progression
- death
- victory
- rendering state

When historical randomness or hardware timing prevents exact determinism, document the limitation explicitly.

### 6. Do not begin by building the mobile UI

Preferred implementation order:

1. Archaeology and behavioral specification
2. Headless simulation core
3. Command-line reference client/test harness
4. Faithful vector renderer and audio
5. Desktop SDL application
6. Touch input layer
7. Android packaging
8. iOS packaging
9. Mobile UX refinement
10. Optional enhanced modes

A mobile screen should not become the primary environment for debugging core game mechanics.

## Tentative Technology Direction

The current preferred stack is:

- **C++20**
- **SDL3**
- **CMake**
- Android
- iOS

This is a strong preference rather than an irreversible decision until the archaeology/design phase confirms that it is suitable.

The simulation core should not depend directly on SDL.

Potential repository structure:

```text
src/
  core/
    commands/
    dungeon/
    entities/
    combat/
    inventory/
    magic/
    timing/
    random/
    simulation/

  presentation/
    render_state/
    vector_renderer/
    text/
    audio/

  input/
    command_parser/
    keyboard/
    touch/
    controller/

  platform/
    storage/
    preferences/

  app/

tests/

docs/
  archaeology/
  specification/
  architecture/
  licensing/
  provenance/
```

Do not force this exact structure if source archaeology suggests a better domain decomposition.

## Licensing and Provenance Rules

Licensing must be treated as an engineering requirement.

The historical Dungeons of Daggorath licensing grant associated with Douglas J. Morgan appears unusually permissive toward preservation, reproduction, emulation, development, and distribution while emphasizing preservation of the original game.

However:

- Do not assume that every later Daggorath port shares the original game's licensing terms.
- Do not assume that box art, advertising artwork, manuals, logos, screenshots, music, or other associated material automatically share the game's licensing status.
- Do not assume that permission to reproduce the game resolves trademark questions.
- Before commercial distribution, current copyright and trademark status should receive appropriate legal review.

### Provenance classes

Maintain an explicit provenance distinction among at least:

1. Original DynaMicro / historical material
2. Material covered by the Morgan preservation grant
3. Later community ports
4. GPL or other copyleft implementations
5. Permissively licensed implementations
6. Documentation/reference-only sources
7. New code and assets created for this project

### No accidental source contamination

Do not copy source code from third-party ports simply because it is publicly visible.

Before copying or adapting code:

- identify the source repository
- identify its license
- record provenance
- determine compatibility with this project's intended licensing

A third-party implementation may be studied behaviorally without copying its code.

When uncertain, treat third-party code as reference-only.

### Preserve licensing evidence

Store copies or references for relevant:

- license grants
- repository licenses
- copyright notices
- source provenance
- historical documentation

Create a provenance ledger as part of the project.

## Archaeology Before Implementation

Do not translate the 6809 source mechanically before understanding the design.

The preferred process is:

```text
Original implementation
        |
Source archaeology
        |
Behavioral model
        |
Written specification
        |
Tests
        |
Modern implementation
```

The original assembly source is authoritative evidence of implementation behavior.

Existing ports are useful secondary sources for:

- identifying misunderstood behavior
- learning platform adaptation techniques
- discovering edge cases
- locating historical documentation

They are not automatically authoritative over the original source.

## Handling Ambiguity

When historical sources disagree:

1. Identify the disagreement.
2. Prefer primary sources where possible.
3. Compare behavior in the original implementation.
4. Record the uncertainty.
5. Avoid silently inventing behavior.
6. Mark unresolved questions for targeted testing or emulation.

Distinguish:

- documented behavior
- source-code behavior
- observed behavior
- community interpretation
- suspected bug
- deliberate design
- unresolved behavior

## Bugs and Historical Quirks

Do not automatically fix original bugs.

Classify discovered quirks as:

- clearly intentional behavior
- likely intentional behavior
- implementation artifact
- confirmed bug
- uncertain

Decisions about preservation of confirmed bugs should be documented individually.
The canonical list is [`specification/quirks.md`](specification/quirks.md).
Phase reconciliation files keep dated copies; they do not replace that list.

Original Mode should favor historically observable behavior unless there is a compelling reason otherwise.

## Mobile UX

Touch controls are an adaptation layer, not a new ruleset.

Goals:

- minimize screen clutter
- retain the original command model
- support both portrait and landscape evaluation before choosing defaults
- preserve game urgency and timing
- avoid pausing gameplay merely because a touch interface is open unless historically appropriate
- maintain access to the original command interface
- consider Bluetooth/physical keyboard support
- consider controller support later

Potential touch concepts may include:

- directional movement/turn controls
- separate left/right hand interaction
- contextual commands
- gesture shortcuts
- inventory overlay
- optional visible command trace

Do not finalize the mobile control system until the original interaction model has been fully documented.

## Agentic Development Rules

Agents working on this repository should:

- inspect existing documentation before modifying behavior
- avoid large speculative rewrites
- make behavior changes independently testable
- add regression tests for discovered historical behavior
- preserve module boundaries
- document licensing provenance
- document uncertainty
- keep commits narrowly scoped
- avoid introducing dependencies unless they materially simplify the project
- avoid replacing simple deterministic code with unnecessary frameworks
- explain deviations from the historical game

An agent must not "clean up" unusual mechanics merely because they appear obsolete.

## Documentation Is Part of the Product

Maintain documentation for:

- historical architecture
- command semantics
- game state
- dungeon representation
- dungeon generation
- simulation timing
- player state
- creatures and AI
- combat
- items
- inventory
- magic
- rendering
- vector data
- sound/audio
- random-number behavior
- progression
- save behavior
- known quirks
- licensing
- provenance
- unresolved questions

The documentation produced during archaeology should later serve as the basis for automated conformance tests.

## Definition of Faithful

Faithfulness should ultimately be evaluated in four areas.

### Game-state fidelity

Same rules, state transitions, entities, progression, combat, dungeon behavior, timing, and outcomes.

### Presentation fidelity

Same or functionally equivalent:

- vector geometry
- perspective
- text
- visual feedback
- heartbeat
- sound cues
- torch behavior
- screen transitions

Modern display scaling may improve pixel/line clarity without changing the underlying presentation model.

### Interaction fidelity

The original command interface remains available and authoritative.

Alternate input methods produce equivalent commands.

### Historical fidelity

Known quirks and implementation artifacts are explicitly documented and consciously preserved or excluded.

## Current Phase

The project is currently in:

**Phase 0 — Archaeology, licensing verification, and behavioral design specification.**

Do not begin substantial production implementation until this phase has produced enough documentation to define the simulation core with confidence.
