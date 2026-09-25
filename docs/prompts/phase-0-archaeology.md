We are beginning Phase 0 of a project to create a faithful mobile implementation of the original **Dungeons of Daggorath**, ultimately targeting Android and iOS.

Read and follow the project's persistent instructions before proceeding (instructions.md).

The purpose of this session is **source archaeology and behavioral design specification**, not implementation.

## Objective

Research the original Dungeons of Daggorath implementation deeply enough to produce a modern engineering model of how the game actually works.

Use primary historical sources wherever possible, especially the original 6809 assembly source and original documentation.

Use later ports and community implementations as secondary sources. Do not assume that their behavior or code is authoritative, and do not copy third-party source into our future implementation unless its license and provenance have been explicitly reviewed.

Browse the web and inspect relevant public repositories and historical documentation as necessary.

## Sources to investigate

At minimum, locate and inspect:

- the original Dungeons of Daggorath 6809 assembly source
- the Douglas J. Morgan licensing grant associated with the game
- original game instructions/manual/reference material where available
- existing C/C++ or SDL-based ports
- the current browser/WebAssembly implementation if relevant
- other well-documented ports where they provide useful behavioral evidence

Search for additional high-quality historical sources if they materially improve understanding.

For every repository or substantial source used, record:

- project/source name
- URL
- author/maintainer where known
- license
- whether it is a primary or secondary source
- how we used it
- whether its code may potentially be reused or should remain reference-only

## Investigate the original source architecture

Map the original assembly modules and important routines.

Determine the responsibilities of modules related to topics such as:

- initialization
- dungeon generation
- dungeon representation
- player state
- movement
- view direction
- command parsing
- creatures
- creature spawning
- creature AI
- creature movement
- combat
- stamina/energy
- heartbeat
- inventory
- hands
- objects/items
- torches
- magic/incantations
- progression
- timing
- random number generation
- sound
- vector graphics
- view transformation
- game-over behavior
- victory conditions

Do not merely enumerate filenames. Explain how the important subsystems interact.

## Reconstruct the game loop

Determine how the original game advances over time.

Specifically investigate:

- whether simulation is frame-, interrupt-, clock-, command-, or event-driven
- how real time passes while the player is typing
- how creature actions are scheduled
- how attack timing works
- how stamina recovery/depletion works
- heartbeat calculation and cadence
- torch timing
- other timers
- how player input and simulation updates interact
- whether hardware timing assumptions affect gameplay

Produce a conceptual modern game loop that preserves the original behavior without yet writing production code.

For example, determine whether the modern simulation should resemble:

```text
receive command
advance timers
update creatures
update player
update sound state
produce render state
```

or some materially different sequence.

The ordering matters. Document it.

## Reconstruct the game state

Identify the minimum authoritative state required to reproduce the game.

Create a conceptual state model covering things such as:

- dungeon
- player position
- facing
- player attributes
- fatigue/stamina/energy
- hands
- inventory
- objects
- creatures
- creature state
- timers
- torches
- progression
- random-number state
- sound/heartbeat state
- parser state if relevant

Distinguish true simulation state from presentation-only state.

## Dungeon model and generation

Document:

- number of dungeon levels
- coordinate system
- dimensions
- representation of walls/passages
- stairs
- object placement
- creature placement/spawning
- generation algorithm
- random inputs
- invariants
- whether generation can be made deterministic from a seed
- any special-case locations or level rules

If dungeon generation is procedural, reconstruct the algorithm at the conceptual/pseudocode level.

## Commands

Build a complete command reference from primary sources and source code.

For every command identify:

- canonical syntax
- abbreviations
- arguments
- parser behavior
- relevant aliases
- prerequisites
- state changes
- timing effects
- failure behavior
- messages/output
- unusual parser behavior

Also determine how commands should map to a future strongly typed command model.

Do not design the touch UI yet, except to note implications of the command semantics.

## Creatures

Catalog each creature type and determine:

- attributes
- spawn conditions
- movement behavior
- perception
- pursuit logic
- attack behavior
- timing
- interaction with player state
- level-specific behavior
- death/removal behavior
- associated sound cues

Where practical, express AI behavior as state machines or pseudocode.

## Combat and player physiology

Reconstruct:

- attack mechanics
- weapon behavior
- hand selection
- damage
- hit/miss mechanics
- creature defenses
- player health/damage
- stamina/exhaustion
- recovery
- heartbeat
- death

Identify formulas, lookup tables, counters, timers, random components, and integer-overflow or fixed-width assumptions that could matter.

## Items, inventory, and magic

Document every meaningful object type and relevant operation.

Determine behavior for commands such as:

- GET
- DROP
- PULL
- STOW
- USE
- ATTACK
- REVEAL
- INCANT

Identify:

- object state
- inventory capacity
- hand behavior
- torch behavior
- weapon behavior
- rings or magical items
- hidden/revealed properties
- magic effects
- item degradation or timing if applicable

## Rendering

Study the original vector rendering architecture.

Determine:

- representation of vector models
- world-to-view transformation
- perspective model
- wall rendering
- clipping
- creature/object geometry
- text rendering
- screen coordinate system
- animation behavior
- redraw strategy

Separate the historical rendering algorithm from what is merely an artifact of the CoCo display hardware.

Produce recommendations for implementing a faithful scalable vector renderer in SDL while retaining the original geometry.

Do not write the renderer yet.

## Sound

Identify all game sounds and what simulation events drive them.

Pay special attention to:

- heartbeat
- creature sounds
- combat
- environmental cues
- timing significance

Determine whether sound is merely presentation or whether audio timing conveys state that must remain synchronized with the simulation.

## Randomness and determinism

Identify every source of randomness.

Determine whether we can replace the historical PRNG with a controlled deterministic implementation while preserving distribution and behavior.

If the exact original PRNG is known, document it.

Consider what information would be necessary to reproduce a session exactly from:

```text
seed
initial state
timestamped commands
```

## Historical bugs and quirks

Record unusual behaviors you discover.

For each, classify it tentatively as:

- intentional
- probably intentional
- implementation artifact
- probable bug
- confirmed bug
- uncertain

Do not recommend fixing quirks merely because they look incorrect.

## Licensing and provenance

Perform a focused licensing review suitable for engineering planning.

This is not a substitute for legal advice.

Document:

- the Morgan license text and provenance
- what it clearly appears to authorize
- what remains ambiguous
- licensing of important community ports
- whether looking at or copying their source could affect our intended licensing
- any separate concerns involving game name, trademarks, manuals, promotional art, audio, or other assets

Create a practical provenance policy for future agents.

## Deliverables

Produce a structured archaeology report containing at least:

1. Executive summary
2. Source and provenance inventory
3. Original source-tree/module map
4. Architectural reconstruction
5. Simulation/game-loop model
6. Authoritative game-state model
7. Dungeon representation and generation
8. Complete command model
9. Creature/AI model
10. Combat and physiology model
11. Items/inventory/magic model
12. Timing model
13. Randomness model
14. Rendering model
15. Audio model
16. Progression, death, and victory
17. Historical quirks and unresolved questions
18. Licensing/provenance findings
19. Proposed modern subsystem boundaries
20. Proposed conformance-testing strategy
21. Open questions requiring additional archaeology or emulator testing
22. Recommended next implementation milestone

Where appropriate, include:

- pseudocode
- state-machine diagrams in text or Mermaid
- tables
- formulas
- module dependency diagrams
- suggested data structures

## Evidence requirements

Distinguish carefully between:

- facts directly established by original source
- facts documented in original manuals
- behavior reported by later ports
- community claims
- our own inference

Cite sources throughout the report.

For important behavioral conclusions, identify the relevant original module/routine where practical.

If sources conflict, explain the conflict rather than silently selecting one.

## Scope boundary

Do **not** begin production implementation in this session.

Small pseudocode examples are appropriate.

Do not create the mobile UI.

Do not redesign gameplay.

Do not propose new monsters, levels, items, mechanics, or balance changes.

The output of this phase should give a future coding agent enough information to implement `daggorath-core` without repeatedly rediscovering how the original game works.
