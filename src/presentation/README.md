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

First work here: `VIEWER` range walk, lighting and line-of-sight stop as a
logical draw list (`viewer.cpp`), with vector bytes generated from the listing
(`vector_tables.hpp`). Rasterisation stays out.
