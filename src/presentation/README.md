# presentation

Derived, observable consequences of simulation state: what is visible, which
display mode is active, which sounds fire and when, and the timed transitional
frames a command produces.

Rules for this module:

- It reads simulation state; it never mutates it.
- It emits an ordered event stream (screen and audio), because the original's
  ordering is part of the behaviour under test.
- It owns no rendering API. Turning presentation state into pixels or samples is
  `platform`'s job.
- Flags that feed back into timing or input (`UPDATE`, `HEARTF`, `NEWLUK`,
  display mode) belong to the **core**, not here — the original couples them to
  the scheduler, so they are not "only graphics".

`viewer.cpp` walks `VIEWER`'s range, lighting, and line of sight. Vector bytes come from generated `vector_tables.hpp`. `mapper.hpp`, `examine.hpp`, and `text.hpp` are the character-cell projections. Rasterisation stays out.
