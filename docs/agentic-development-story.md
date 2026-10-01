# Building Dungeons of Daggorath with agents

*A case study for software engineers who use agentic development tools. Snapshot: `main` at
[`b8afeb8`](https://github.com/SteveLeve/daggorath/commit/b8afeb869436fee80e05ee3e5b9b10fdaf46466e),
written 1 October 2026. Times are US Central. [Sources, method and gaps](#sources-method-and-gaps)
says what each kind of claim rests on.*

## Why this experiment

The project was a deliberate experiment in taking the hands off the wheel: let agents do most
of the work while one person guides. A video game made that reasonable, because the blast
radius of a mistake is small. It was also a test of the major vendors' coding harnesses on two
questions: how well they turn an idea into a detailed plan, and how well they turn a detailed
plan into working software with little human intervention.

By the owner's assessment, most of the project's goals are met at this checkpoint. One person
took the 1983 *Dungeons of Daggorath* from a planning chat to a deterministic C++20 core, an
SDL3 desktop build, a local WebAssembly PWA and a debug Android APK running on a phone in about
six and a half days (24 September 12:11 to 30 September 22:55). The work used ordinary
subscriptions to ChatGPT/Codex, Claude, Cursor and GitHub Copilot, plus Gemini for research. No agent framework was written. Licensing, not
engineering, is the main obstacle to public deployment.

The owner wrote almost no code and very few long prompts. Across roughly 110 interactive
sessions the typical instruction is one or two sentences. What the owner supplied was
decisions, hand tests, and a steady insistence that each agent write the *next* agent's
instructions. The owner's own summary is that larger tracks of long-running work could run in
parallel, with less context switching between terminals, leaving attention for design and
specification.

The project's subject made evidence unusually important. A reconstruction of a 1983 game is
only as good as its reading of the original, and a confident wrong reading looks the same as a
right one. So the same methods that kept the agents productive also had to keep their claims
honest, and this document tries to hold itself to that standard: each statement names what
supports it and what it does not show.

### Terms

| Term | Meaning here |
|---|---|
| Original Mode | The game as shipped: the five fixed maps, original rules and timing, no player-supplied seed |
| Jiffy | One 60 Hz interrupt; the core's unit of simulated time |
| Phase prompt | A committed Markdown file in [`prompts/`](prompts/README.md) giving one phase's preservation requirement, evidence rule, work order, "do not build" list and completion gate |
| ADR | An architecture decision record in [`adr/`](adr/README.md), for decisions that cross phases |
| Reconciliation | A dated per-phase record of what was checked, the result, and what stays open; corrections are appended, not rewritten |
| Evidence labels | `source-proven`, `ROM-observed`, `inferred`, `unresolved`; every behavioural claim carries one |
| Fixture, manifest | Data extracted from the listing with a source location and extraction method; `MANIFEST.json` holds a hash per fixture and `make verify` checks it |
| Track R | The ROM observation track ([ADR-0003](adr/0003-rom-observation-track.md)): captures of the original cartridge under MAME, run alongside the gameplay phases |
| Deviation (D-n) | A recorded, deliberate difference from the original, listed in [`clock-and-scheduler.md`](specification/clock-and-scheduler.md) §13 |
| Goal | A persistent objective (`/goal` in Cursor, Codex and Claude Code) that re-prompts the agent each time it stops, until done or blocked |
| Loop | The same prompt run on a schedule or self-paced, with state kept in a log file |
| Compaction | Summarising a long agent context to free space (`/compact`) |

## What was built and how far it got

The aim is to bring *Dungeons of Daggorath* to modern devices while keeping its original
rules, command language, timing, maps, difficulty and quirks. The [charter](project-instructions.md)
makes preservation the product requirement and orders the work: archaeology, a headless
simulation, presentation, desktop play, touch input, mobile packaging. The primary evidence is
a reconstructed assembly listing at a pinned revision; extracted fixtures, a written scheduler
model and selected ROM captures supply different kinds of check
([provenance ledger](provenance/ledger.md)).

| Claim | Support at `b8afeb8` | Limit |
|---|---|---|
| Native build and registered tests | The Codex session that wrote the first draft of this case study (30 Sept, 22:57) records a run of `make build && make test` with SDL3: `dod` built and 26/26 CTest cases passed. Re-run for this revision on 1 Oct in a cloud container without SDL3 (the non-`docs/` tree is identical to `b8afeb8`): desktop target skipped, 24/24 passed | These tests do not replay every ROM state or stand in for live screen, audio or device use |
| Fixture integrity | `make verify`, both runs: Phase 0b 18, Phase 6 vector 16 and Phase 6 text 22 fixtures, 0 problems | A clean manifest checks recorded artifacts, not the correctness of every historical interpretation |
| Original Mode victory | The committed Phase 5b script and its verifier (`playthrough_power_on_to_winner`) passed in both runs | Deterministic core behaviour under recorded deviations; no claim of ROM-equivalent play ([Phase 5 reconciliation](archaeology/phase-5/reconciliation.md#playthrough-status)) |
| Web | [#48] reports a local browser build and forced-failure storage tests | Local only ([ADR-0011](adr/0011-web-stopgap-delivery.md)). No browser was run for this document; real mobile browsers, offline install and first-tap audio stay open |
| Android | [#52] reports a CI debug APK; [#54] reports the owner's Samsung run of the safe-area layout and crisp movement and turn | No device, emulator or Android conformance suite was run for this document. Orientation changes, background-kill recovery and iOS are open ([ADR-0012](adr/0012-mobile-packaging.md)). The README and `android/README.md` still say no device run occurred |
| Public distribution | Licensing decisions D1–D5 ([#46], [#52]): MIT for the project's own work, copied data listed in `DATA-NOTICE.md`, source-only distribution, private Android builds | The six licensing questions stay open for counsel ([licensing](licensing/README.md)) |

The remaining preservation work is concrete. Track R still has unresolved scheduling,
attack-outcome, visual and animation-timing questions ([capture backlog](planning/capture-backlog.md)).
Phase 9 still needs Android conformance and lifecycle recovery on a device, then iOS
([Phase 9 prompt](prompts/phase-9-mobile-packaging.md)).

### Scale, as logged

| Measure | Value |
|---|---|
| Elapsed | 24 Sept 12:11 to 30 Sept 22:55 |
| Commits / merged PRs / issues | 352 / 31 / 24 |
| Tracked lines: `src/`, tests, tools, docs | 11,704 / 5,455 / 5,307 / 9,580 |
| Claude Code local sessions | 35 interactive, 77 automated review |
| Claude Code cloud sessions behind PRs | 5, producing 10 PRs |
| Claude Code tool calls (local) | about 4,600, of which 2,981 shell |
| Claude Code sub-agent launches (local) | 106 (55 evidence audits, 27 boundary checks) |
| Claude Code output tokens (local sessions) | 5.0 million |
| Codex Phase 5b session | 26 hours, 1,675 tool calls, 10 compactions, 4 models |
| Cursor phases 3–7 chat | 917 tool calls, 65 goal continuations |
| Cursor Phase 1 capture chat (the stuck goal) | 303 tool calls, 134 goal continuations |
| Cursor stack-review chat | 479 tool calls, 16 sub-agents |
| Exported Cursor chats (15 distinct), total tool calls | about 3,400 |

Token and tool counts are measurements of the logs. They are not costs, and they do not
compare tools fairly: the products log different things.

## Timeline

| When | Tool | What happened |
|---|---|---|
| 24 Sept 12:11 | ChatGPT (GPT-5.6 Thinking) | Opening discussion: licence, stack, sequence. Output: project instructions and a Phase 0 archaeology prompt. |
| 24 Sept, before 19:02 | ChatGPT project | Phase 0 archaeology report. The owner corrects it (maps are fixed, not seeded); ChatGPT checks the assembly, patches the report and the instructions, then writes the Phase 0b prompt from the report's own "recommended next milestone". |
| 24 Sept 19:02 | — | Repository initialised with the instructions and Phase 0 prompt, then the Phase 0 report and the 19-line Phase 0b prompt. |
| 24 Sept evening | Claude Cowork (desktop app, Opus 5) | Phase 0b from a one-line instruction: 15 source-extracted fixtures, five traces, timing spec, headless core slice. A second instruction reorganised the repository into a code project and wrote the Phase 1 prompt. Committed 21:01 (74 files); Claude Code CLI wrote the commit message. |
| 25 Sept morning | Cursor `/goal` | Phase 1 implementation ([#1]). Claude Code reviews it as "software-complete, ROM-unverified". |
| 25 Sept 10:05–13:05 | Claude Code | `claude-automation-recommender` run; hooks, audit agents and skills committed (`c9a0401`). Copilot generates `AGENTS.md`. |
| 25 Sept 13:21–17:46 | Claude Code cloud, Codex review | Planning [#12] from a six-sentence prompt: roadmap, ADRs 0001–0008, twelve phase prompts, capture backlog. Phases 2–9, 6a and Track R become issues [#3]–[#11] and [#14]. A Codex review, posted to the PR at 16:41, finds four contract problems; all are fixed in one pass (including a new Phase 6a), and the re-review at 16:54 recommends approval. The review came from the ChatGPT project chat "Review PR Recommendations", not a local Codex session. |
| 25 Sept 14:11–16:44 | Claude Code + Gemini | Cursor is looping on missing firmware. Claude diagnoses, Gemini research identifies options, owner supplies firmware, five ROM captures run. The 377-interrupt divergence is found ([#13]). |
| 25 Sept 17:44 | ChatGPT project | The chat "Discuss Issue 13 Implications" works through counting the 377 build interrupts, records the decision on [#13] and posts its consequences and the rejected alternative. |
| 25 Sept 18:07–18:47 | Codex | Four attempts to review Phase 1 ([#1]) end in interrupted `/review` runs or API 401 errors, an `auth` request is interrupted, and a sixth session only checks connectivity. No review is produced. |
| 25 Sept 19:21–22:13 | Cursor `/goal` | Phases 2–7 in one evening: [#15], then a five-PR stack [#16]–[#20]. 65 automatic goal continuations in the phases 3–7 chat. |
| 25 Sept 23:27 – 26 Sept 00:51 | Cursor `/goal` + sub-agents | Two review goals over the stack: verify every claim by running code, then apply fixes. 16 sub-agents. Cursor's premium-model allowance runs out mid-run and it falls back to Grok 4.6. |
| 26 Sept morning | Cursor | Desktop demo, first hand test ("I see only a black screen"), play-bug plan and fixes ([#21]), port comparison report. |
| 26 Sept 08:07, 11:11 | Codex | Two reviews of the `HSLOW` rounding change find no regression; both note the manifest check still blocked by an unlisted text fixture. |
| 26 Sept (date inferred) | ChatGPT project | The chat "Review Daggorath Cursor workflow" writes a retrospective of the work so far: phase prompts as contracts, acceptance checked on the owning branch, an assisted victory as regression evidence only. Its record gives no date; its 14-test count matches the repository on 26 Sept. It changed nothing in the repository. |
| 26 Sept ~14:40 | Cursor → Claude Code | Cursor credits exhausted. Claude Code picks up from a pasted transcript. |
| 26 Sept 18:38 | Claude Code | Hourly refinement loop starts (cron `17 * * * *`). Runs 1–29 over about 27 hours ([#23], [#31]). |
| 27 Sept 10:12 – 28 Sept 12:05 | Codex | Phase 5b honest playthrough ([#33]): plan at high effort, compact, execute as a goal on a lower model in a dedicated worktree. Eight companion sessions on 27 Sept smoke-test the custom review roles and review the prompt, the recovery code, the planner and the verifier; on 28 Sept at 11:53 three audit roles check `6b5abe6`. The final review was never posted to the PR. |
| 27 Sept 11:25–14:26 | Claude Code cloud (plan mode) | Phase 8 planning [#24]: ADR-0009 (shell and pause), ADR-0010 (render styles), D-16 and D-17. Thirteen owner decisions taken through typed answers and multiple-choice questions; a port study run on the owner's machine from a prompt the session wrote. Ends by writing the Phase 8 goal prompt. |
| 27 Sept 14:28–16:59 | Claude Code cloud | That goal prompt, pasted into a fresh session, produces six headless touch PRs [#25]–[#30] within 55 minutes. SDL3 is missing in the container; the on-screen work is recorded as open. |
| 27 Sept 17:24 – 28 Sept 15:53 | Claude Code | Phase 8.6 SDL wiring in a second worktree, then a touch-overlay loop driven by hand-test feedback ([#32]). |
| 28 Sept 15:13 – 30 Sept 15:43 | Claude Code | Track R ROM captures C-09 to C-21 across four worktrees: [#36], [#41], [#44], [#50]. |
| 28 Sept 15:57–18:14 | Codex | Crisp-vector shading ([#37], [#38]) and the touch-after-load fix ([#40], [#42]), each from a one-paragraph bug report. Shading took a planning session, an implementation session that opened the issue, worked in a new worktree and opened the PR, and two audit sessions; the touch fix took one session doing the same. |
| 28 Sept 18:20 – 30 Sept 13:30 | Claude Code, Codex review | Web stopgap [#48]. Codex's review (29 Sept 15:01) finds the save-durability defect; its second review confirms the fix. |
| 28 Sept 19:00–19:09 | Claude Code cloud | Licensing [#46]: a pasted Gemini analysis, four owner decisions, MIT for own work and `DATA-NOTICE.md` for copied data. |
| 30 Sept 16:07–21:54 | Claude Code cloud, started from the Android app; Copilot and Codex reviews | Phase 9 Android shell [#52], from a two-sentence prompt. The container cannot reach the Android SDK, so CI builds the first APK. Copilot finds a scaling bug. Codex finds a background-time bug at 17:38 without GitHub access; a second Codex session that started at 17:34 checks the Copilot threads and posts the review at 17:47. Both bugs are fixed before merge. |
| 30 Sept 21:33–22:55 | Claude Code | APK installed on the owner's phone; navigation-bar overlap and crisp half-step fixed from live feedback ([#54]). |
| 30 Sept 22:57 | Codex | Surveys the history, PRs and documentation and writes the public-record case study, the first draft of this document. |

Commits per day show the shape: 4, 135, 43, 101, 45, 12, 11. Two days of generation, then
progressively slower, more deliberate work as the remaining problems needed a human looking at
a screen or a ROM capture. The dates locate sessions, commits and merges; they are not
continuous working time.

### Who did what

| Tool | Role | Evidence |
|---|---|---|
| ChatGPT | Framing: licence reading, stack choice, charter, first prompt; in the same project, the [#12] review, the [#13] decision analysis and a workflow retrospective | The shared conversations; the owner's report on five project chats |
| Claude (desktop, CLI, cloud) | Planning and ADRs; harness setup; unblocking; loops; Phase 8, Track R, web, licensing, Android; review | 35 interactive local sessions; 5 cloud sessions behind 10 PRs; 28 commits authored as `Claude` |
| Cursor | Bulk implementation of phases 1–7 under `/goal`; cloud review of its own stack | 129 commits with a Cursor co-author trailer; 11 `cursor/` branches |
| Codex | Phase 5b search; two targeted fixes; independent PR reviews of [#33], [#48] and [#52] (and [#12] from a ChatGPT project chat); the first, public-record draft of this case study | 30 local sessions, 25–30 Sept; two stored review goals; the owner's "based on codex review" instruction in the [#12] cloud session |
| GitHub Copilot | PR review on 6 PRs; `AGENTS.md`; one conflict-resolution plan | Review records on [#15], [#32], [#36], [#41], [#50], [#52]; a second [#52] review refused for quota |
| Gemini | Research only: CoCo firmware sources, web-delivery options, the licensing landscape | Pasted into Claude prompts on 25 and 28 Sept |

The split followed the owner's stated intent. A Cursor subscription with unused allowance was a
few days from expiry, so Cursor got the work that consumes allowance fastest and needs least
judgement: executing well-specified phases. Claude wrote those specifications. The public
record alone shows only part of this: PR signatures, `cursor/` branch names and review
comments establish roles, not how much code any one tool wrote.

### Cloud sessions

Five Claude Code sessions on claude.ai/code produced ten of the 31 merged PRs. Four were started
from the Claude desktop app and one, Phase 9, from the Claude Android app. All ran with automatic
permissions except Phase 8 planning, which ran in plan mode. "Owner turns" counts typed messages;
multiple-choice answers are listed separately.

| PRs | Session span (US Central) | Owner turns | What it produced | Output tokens |
|---|---|---|---|---|
| [#12] | 25 Sept 13:21–17:46 | 2 | Roadmap, ADRs 0001–0008, phase prompts, capture backlog, review checklist, link checker; issues #3–#11 and #14 | 49k |
| [#24] | 27 Sept 11:25–14:26 | 7, plus 5 multiple-choice answers | ADR-0009, ADR-0010, D-16 and D-17, the Phase 8 plan, a port-study prompt, the Phase 8 goal prompt | 57k |
| [#25]–[#30] | 27 Sept 14:27–16:59 | 2 | Six workstream PRs, each audited; 11 audit sub-agents (6 evidence, 5 boundary) | 300k |
| [#46] | 28 Sept 19:00–19:09 | 1, plus 4 multiple-choice answers | `LICENSE`, `DATA-NOTICE.md`, licensing decisions D1–D4, issue [#45] | 17k |
| [#52] | 30 Sept 16:07–21:54 | 2 | Android Gradle shell, ADR-0012, licensing decision D5, CI workflow, two review fixes | 42k |

The spans include long idle stretches while a PR waited for review. The working turns were
short: 6 minutes for the whole planning package of #12, 22 minutes for the first pass
of #25–#30, 5 minutes for #46, 9 minutes for the first pass of #52.

**Planning, [#12].** The whole instruction was:

> review the project and planning documentation. Analyze and extend planning for phases beyond
> phase 1 currently under development. Create GitHub issues and ADRs/planning documentation for
> following phases. Our output should be suitable for use as prompts for coding and review
> agents. You may make a new branch off the phase-1 branch to work in isolation. Our immediate
> goal is to refine and extend project planning to a higher level of maturity and completeness
> so that future sessions have a strong context and defined scope for their work.

Three hours later a Codex review appeared on the PR, and the owner wrote: "yes, watch the PR.
review & respond to the new comment based on codex review." The session accepted all four
contract findings, made Phase 6a a required phase with its own issue, and replied on the PR
about two minutes later. The re-review recommended approval and raised two nits, which the
session fixed from the PR notification without a further owner message.

**Phase 8 planning, [#24].** The opening prompt set the architecture question rather than the
answer: "We will need a strategy for layering our enhancements while preserving the original
and supporting ongoing work on both ends … We will need to resolve architectural questions
before breaking ground implementing the touch UI, we will begin with planning as a goal." The
session worked in plan mode and asked before writing. The owner took the recommended option on
three multiple-choice questions (pause in the shell, crisp rendering by default, menu saves kept
separate from `ZSAVE`), then answered six open questions in one message, beginning:

> 1. whole line like web port
> 2. fly over the sides, keep controls off the bottom to leave room for the status bar. The upper
>    left and right are mostly free space
> 3. clarify, not sure I understand the question.

Question 3 came back as a multiple-choice question about what the floor and pack pickers may
reveal; the owner chose "Only what's visible". Three more decisions followed ("draw sharp in
crisp mode", "mark game pause in the trace log for debug purposes", "use a hidden slot for
auto-save for resuming from background"). The owner then had the session write a read-only
port-study prompt, ran it locally, and attached the report. One decision taken after that
report, drawing dim vectors as dots, was reversed for `crisp` the next day (ADR-0010 addendum,
[#37], [#38]). The session ended on "let's merge the PR then launch a goal prompt to begin
working through the rest."

**Phase 8 workstreams, [#25]–[#30].** The opening message was the goal prompt from #24 under
`/goal`, beginning "Carry out Phase 8 of the Dungeons of Daggorath preservation project to its
completion gate, as a sequence of small PRs to main." The session record shows the owner
switched it to a smaller, cheaper model than the default before starting. It opened six PRs in
55 minutes and launched an audit for each. The audits changed the PRs before they were opened:
the ADR-0009 resolution had claimed menu entries that had no code, and the Phase 8
reconciliation had an all-ticked checklist beside the word "complete" while four of its PRs were
unmerged. The session also merged #25 without being asked, and said so in its report: "Earlier
I merged PR #25 myself without asking — that was a mistake". After that it left the drafts
open and asked. The owner's second message, 77 minutes after that question, settled it:
"continue, let's merge back down to main; verify each step. you may need to rebase, I merged a
bug fix branch into main ahead of this work."

**Licensing, [#46].** The owner pasted a long Gemini analysis of the six open licensing
questions under three lines of instruction: "see @docs/licensing/README.md for open legal
questions. I asked Gemini to explore the landscape and shared the response below. Let's work
through this, make some decisions, then create a new github issue, branch, & PR to capture &
resolve this concern." The session checked the analysis against the repository first and
declined three of its suggestions: a non-commercial licence over data the project cannot
license, an attribution block with an unsourced date and names, and a misnamed recipient of
the handwritten grant. It then asked four questions; the owner chose the recommended option on
each within two minutes (MIT for own work, mark and isolate copied data, source-only
distribution, enhancements opt-in). The evidence audit found three problems in the first
draft, among them an unsourced "1982" date, and they were fixed before the PR opened.

**Phase 9, [#52].** Started from the owner's phone: "While I work on licensing I'd like to
start work on phase 9, mobile apps. Initially I'll test on private android devices, but would
like a path to iOS eventually." The session turned "test on private android devices" into
licensing decision D5 (private devices only, CI uploads nothing) and asked the owner to check
the wording, "since it's your decision on record". The owner's only other message was "Watch
the PR, respond to review comments". From then on GitHub events drove the session: it fixed
Copilot's scaling finding, resolved five threads, fixed the background-time finding from Codex's
review, and reported CI on each push. The owner merged at 21:54; a local session was by
then installing the APK on the owner's phone ([#54]).

## Techniques

The techniques are ranked by how much of the result they explain. The first three kept the
agents moving with little human input; the next five kept their claims honest; the last three
are about the human and the limits.

### 1. Every stage ends by writing the next prompt

The pattern is visible in the first hour. The opening ChatGPT request ends: "We will generate a
project plan to hand off for agentic execution." ChatGPT declined to plan immediately and
recommended a source-archaeology pass first. The owner's second message accepted that and asked
for two artifacts: persistent project instructions and "a prompt for the next useful step".

That request recurs in every tool for the rest of the week:

- Claude Cowork, straight after Phase 0b: "reorganize the prompt files and original
  instructions under docs, generate a project README.md and a prompt file for the next slice."
- Cursor, end of the first Phase 1 chat: "generate markdown instructions describing the
  remaining steps to use as a goal prompt in a fresh context".
- Cursor, after the ROM captures: "update PR, comment on progress, and write a prompt for the
  next goal".
- Claude Code, after unblocking firmware: "update or create relevant issues, PR description,
  and comment on changes. then generate a prompt for remaining work needed to complete phase 1."
- Claude Code, before the playthrough rework: "I also want to generate a prompt to begin work
  on fixing the planner & play through."
- Claude Code cloud, end of Phase 8 planning: "let's merge the PR then launch a goal prompt to
  begin working through the rest." The goal prompt it wrote opened the next cloud session
  almost word for word, and that session produced #25–#30.

The Phase 0b run shows how little the owner had to type once a prompt file existed. The whole
instruction was "read and execute the prompt in file: phase-0b-prompt.md", and the prompt file
itself is 19 lines. The session cloned the pinned listing, extracted fixtures, wrote the
scheduler specification and a C++20 slice with 68 passing checks, and reported four Phase 0
statements the source contradicts. It also stated plainly what it had not done: "Verified
against a retail ROM: nothing." That sentence became Phase 1's first task. The follow-up
reorganisation made the licensing rule mechanical by git-ignoring `*.rom`, `*.ccc`,
`captures/` and `third_party/`.

The prompts were committed, which is why `docs/prompts/` has 18 files. The next session started
with a one-line instruction pointing at a file:

> /goal implement the next phase of the project: docs/prompts/phase-1-conformance-and-creatures.md

The effect is that the expensive thinking happened at the end of a session, when the agent had
full context, and the cheap instruction happened at the start of the next, when it had none.
The prompts were mostly written by an agent that had just done the preceding work, and this was
asked for explicitly each time.

The prompts worked as contracts because of their shape. Planning [#12] split the headless game
into movement, combat, objects and progression, then separated presentation, SDL, touch,
mobile packaging and ROM observation. Each phase prompt named prerequisites, expected
artifacts, an evidence rule, exclusions and an executable completion gate. Routine descriptions
in a prompt were marked as hypotheses to verify against the listing, not evidence. ADRs held
decisions that crossed phases: scheduler order, the presentation contract, save formats, later
the shell's pause. Tasks became portable across agents and sessions while their authority
stayed below the primary sources ([roadmap](planning/roadmap.md),
[prompt conventions](prompts/README.md)).

Much of the project's discipline traces to the first two documents. The architecture diagram,
the `src/` layout, the rule that touch input emits original commands, the provenance classes,
"do not begin by building the mobile UI", and the Original Mode separation all appear in the
ChatGPT output and survive in `docs/project-instructions.md`. ChatGPT also added one thing
unprompted that turned out to matter most: making "historical timing and ordering an explicit
archaeology target".

### 2. Persistent goals for execution, with a stop condition in the prompt

Cursor's `/goal` holds an objective and re-prompts the agent whenever it stops. The phases 3–7
chat contains one owner instruction and 65 automatic continuations before the owner typed
again ("explain status and any complications"). That single goal produced the five-PR stack
[#16]–[#20]. The agent narrated progress as PR comments: [#20] carries 56 short status comments
posted over 71 minutes, each naming what landed and the running check count (789 rising to
about 840).

The goal prompts were short but always carried the same four clauses: branch and PR handling,
commit and push without asking, record decisions in PRs and issues, and use verification
sub-agents. For example:

> /goal implement phase 2, issue #3. create a new branch, commit and push changes as you work,
> create a new Pull Request with a detailed description including validation & verification
> criteria, definition of done, and review notes. Leverage verification sub-agents to check
> work for completeness and correctness.

The same mechanism was used in Codex (`/goal` with a handoff file) and Claude Code
(`/goal continue working through the remaining tracks (C-12 through C-16, …) until complete or
blocked`), including the cloud session that produced #25–#30.

The failure mode also appeared on day one. Cursor's Phase 1 capture goal required ROM captures
and the machine had no CoCo firmware. The goal re-prompted the agent 134 times in that chat,
each time ending on the same blocker. The owner broke the loop by hand: "we seem to be stuck,
summarize what has been done and what's blocked in a troublehsooting prompt". That summary went
to Claude Code. The fix went into the standing rules as "An obstacle is recorded once, in one
place, with its reason. Do not re-check or restate it every turn; mark the item 'not run:
<reason>' and move on." Later goals say "until complete or blocked", and later review goals
say "'untested' is a valid response".

### 3. Scheduled loops with a log as the only memory

Goals suit a task with an end. For open-ended discovery the owner asked for a loop:

> let's design an improvement loop prompt to crawl over the app and original source looking
> for missing or incorrect code in our version. … I'd like each loop to kick off a discovery
> and refinement goal and track progress in a log or memory file. … Each loop should commit and
> push any refinemnts automatically. Ping me with questions if we need a human in the loop.
> … I'd like to run the loop every hour or 90 minutes expecting a 10 - 30 minute session.

Claude Code produced [`planning/refinement-loop.md`](planning/refinement-loop.md) (the per-run
prompt), `refinement-log.md` (state), and one cron job at 17 minutes past each hour. The
prompt's second line is the design: "Every run starts from `docs/planning/refinement-log.md`;
do not rely on conversation history."

Properties that made it work:

- **Bounded runs.** One to three targets, 10–30 minutes. Each run reads the listing and current
  code, classifies each difference as a bug, deviation, quirk or open question, adds a
  source-cited regression for a real bug, runs the full gate, and stops.
- **A coverage map.** The log lists the assembly files and labels as `unreviewed`, matched,
  `gap-open` or `needs-human`, so successive runs walk the listing instead of revisiting it.
- **Asynchronous questions.** Unclear items became numbered questions (Q1–Q7) with a push
  notification. The owner answered in batches hours later: "for Q2 merge & regenerate. Q3,
  disable the playthrough test, will take on the planner rework".
- **Self-audit.** A run that committed without an `evidence-auditor` pass obliged the next run
  to audit that commit first.
- **A guard against collisions.** A dirty tree the loop did not create stops the run.

It ran 29 times and found things no test was looking for: a fire-ring charge counter that
wrapped, redraw jiffies not charged on object commands, the map and examine screens absent from
the window, and a wrong `HSLOW` healing constant. The ring-charge fix invalidated the
existing winning playthrough, which is how Phase 5b came to exist.

The same shape was reused twice with self-paced `/loop` in place of cron: a Phase 5b search
loop and a touch-overlay loop ([touch loop](planning/touch-overlay-loop.md)). The touch loop is
the clearest human-in-the-loop example. The owner hand-tested, wrote a paragraph of
observations, and the next run consumed it as answers and new targets. It also prompted a
question worth keeping: "how can we get you eyes to see with?" The answer was an offscreen SDL
mode (`dod --shots`) that renders scripted taps to screenshots the agent can read.

### 4. Label every claim by its evidence

The project did not translate the assembly mechanically and call the result faithful. Phase 0b
recorded which readings came directly from the listing, which were inferred and which remained
open. It corrected four earlier archaeology claims instead of silently replacing the report:
for example, the `PLAYER` routine drains buffered keys within a turn, the source's direction
token is `BACK` rather than `BACKWARD`, and an overflowing 32-character line dispatches without
a return ([Phase 0b reconciliation](archaeology/phase-0b/reconciliation.md)). Fixtures carry
source locations and extraction methods, and the manifest verifier detects missing or altered
entries ([fixture policy](provenance/ledger.md), [quirks](specification/quirks.md)). These
controls matter because a test derived from a mistaken reading faithfully preserves the
mistake.

Phase 1 shows an external check changing the model. The first implementation was reviewed as
software-complete and ROM-unverified ([initial review](https://github.com/SteveLeve/daggorath/pull/1#issuecomment-5834875583)).
Once the pinned listing was assembled and the cartridge run under MAME, the opening trace
diverged at its initial clock: the ROM had counted 377 interrupts during level construction
([first ROM captures](https://github.com/SteveLeve/daggorath/pull/1#issuecomment-5840073801)).
The owner made that a recorded Original Mode decision ([#13]), the core and reference traces
changed with a documented reason, and the PR kept narrower labels for
captures that had not been rerun ([Phase 1 reconciliation](archaeology/phase-1/reconciliation.md)).
The assembled listing's byte match to catalog 26-3093 strengthens its provenance, but each
behaviour still needs an applicable capture.

This became a repeatable rule: the four evidence labels describe different levels of support,
and only a capture under authenticated original system ROMs, or a documented equivalence
result covering the behaviour, promotes a claim to ROM-observed.
Track R gathers those observations without blocking every gameplay phase, and it records
failed captures too:

- **C-09.** An early PR ([#36]) treated a six-jiffy offset as confirmation of creature queue
  timing. The current [reconciliation](archaeology/track-r/reconciliation.md#c-09--first-creature-move-after-level-entry)
  explains why the chosen anchor was already downstream of queue insertion and leaves that
  timing unresolved.
- **C-11.** A ten-minute idle capture found substantially different task-dispatch counts
  between core and ROM ([#41]).
- **C-10.** Moving a keystroke to an apparent tie changed the ROM schedule and did not
  reproduce the tie ([#44]).

Each observation stays scoped to what was measured; the core was not adjusted to fit a
conjectured mechanism.

### 5. Mechanical guardrails installed on day two

Before most gameplay code existed, the owner ran a recommender skill ("review the project,
recommend code quality automations, custom agents & skills for this project") and committed the
result:

- a pre-edit hook, `guard-evidence.sh`, that blocks edits violating the evidence rules;
- a post-edit C++ formatter and a stop hook that runs `make verify`;
- two read-only audit agents, [`evidence-auditor`](../.claude/agents/evidence-auditor.md) and
  [`boundary-checker`](../.claude/agents/boundary-checker.md);
- skills for fixture changes and phase checks.

The public record shows that these agents exist, not that they ran on every change. The local
logs give a count: `evidence-auditor` was invoked 55 times and `boundary-checker` 27 times
from local Claude Code sessions, and the cloud sessions behind #25–#30, #46 and #52 launched
14 more. The daily
notes show audits finding real problems, such as citations pointing at the wrong deviation
number and append-only reconciliation files being rewritten. They also show the limit:
"unpushed (missing checks: make all, evidence-auditor, boundary-checker)" appears more than
once, and those commits were audited later, not before.

A separate automated security review ran on commits: 77 short non-interactive sessions, each
given a diff and asked for vulnerabilities. It confirmed one, a buffer over-read in
`snoise.cpp`, and raised a candidate bypass in the guard hook itself.

The same rules were mirrored for other tools: `AGENTS.md` for Copilot and Codex, and
`.codex/agents/*.toml` copies of the three audit agents. Codex created its own skill and
reviewer agent for Phase 5b when asked to "scaffold any custom skills or agents you think would
be helpful for achieving the goal".

The architecture is a guardrail of the same kind ([module boundaries](architecture/module-boundaries.md)).
`src/core` owns simulated time, scheduling, RNG, rules and timing-coupled display flags, and
links no SDL or platform library. Presentation derives view state; input adapters turn touch or
keyboard actions into the command and keystroke path; the shell handles pause and snapshots;
the platform draws, plays sound and stores files. Because input reaches the core only as
timestamped keystrokes, `dcli`, desktop SDL, touch input and tests drive the same core, and
host time becomes discrete jiffies, so a late frame cannot skip interrupts
([clock specification](specification/clock-and-scheduler.md), [touch architecture](architecture/touch-input.md)).

### 6. A different model reviews, at the revision that owns the requirement

Review was routinely given to a tool that had not written the code.

| Work by | Reviewed by | Outcome |
|---|---|---|
| Cursor, Phase 1 | Claude Code | "the unstaged changes do not complete Phase 1" — pasted back into Cursor as its next instruction |
| Cursor, Phase 1 PR | Cursor `/multi-model-review` using Grok 4.7 | Claim checklist, then a goal to close it |
| Claude, planning [#12] | Codex, posted to the PR; relayed by the owner ("review & respond to the new comment based on codex review") | Save-format, ownership, seeding and firmware-label problems fixed before any phase ran; re-review recommended approval |
| Cursor, stack [#16]–[#20] | Cursor cloud agent | Per-branch reviews that separated "fixed downstream" from "satisfied here" |
| Codex, Phase 5b [#33] | Codex in a fresh goal, plus three Codex audit roles; then Claude applies notes | Independent replay; hash matched; 3 of 4 notes applied. The review lives in the session log, not on the PR: the automatic approval check rejected posting it |
| Claude, web [#48] | Codex `/goal` review | Found that a failed IndexedDB sync still reported a successful save |
| Claude, Track R | Copilot | Six findings on [#50] |
| Claude, Android [#52] | Copilot, then Codex | Copilot: the game would draw at a fixed size in one corner of a phone screen, plus four stale "not built" notes. Codex: background time still fed the jiffy accumulator. One Codex session found it without GitHub access; another checked the Copilot threads and posted the review |

The planning review is the clearest case of a reviewer finding decisions hidden in harmless
prose. Before any phase ran, it found that save-format wording could put a modern header into
the historical payload, that presentation ownership could reverse the inward dependency, and
that a ban on player-selected seeds might forbid deterministic test construction
([review](https://github.com/SteveLeve/daggorath/pull/12#issuecomment-5840045269)). The documents
were revised before those ambiguities reached later phases.

Two review prompts are worth reusing. The first constrains the reviewer to observation:

> /goal review PR 48, the current branch. add your report to the PR as a comment with analysis
> and recommendations. Anser if the PR is merge ready or if changes are needed. Attempt to
> verify claims if possible, "untested" is a valid response. If additional tools are necessary,
> request additional resources (MCP servers etc). Do not make changes.

The second closes the loop: "The PR has been updated addressing your comments, re-check and
validate." Reviews were relayed by copy and paste between tools, with the PR as the durable
record.

A review also has to be pinned to a revision. [#16]–[#20] formed a dependency stack from combat
to the SDL window, and their passing check counts climbed as later branches gained work. A
later tip's success did not satisfy an earlier PR's acceptance criteria:

- At [#16] the combat tests read only the 88 scaling fixture rows; damage and attack rows were
  first consumed later ([Phase 3 branch review](https://github.com/SteveLeve/daggorath/pull/16#issuecomment-5842652969)).
- [#17]'s `INCANT` path could index beyond its 18-row object table; the seven special rows
  arrived on [#20] ([Phase 4 branch review](https://github.com/SteveLeve/daggorath/pull/17#issuecomment-5842656645)).
- [#18] called a narrow historical save payload a complete suspend snapshot until later
  documentation corrected the claim.

The stack was merged in dependency order after the high-severity inherited defect was fixed on
the later branch, with some medium-severity phase requirements recorded as open
([merge-ready review](https://github.com/SteveLeve/daggorath/pull/20#issuecomment-5842670889)).
A passing suite describes the checks present at one revision; an issue or phase gate describes
a larger obligation. A useful review names the commit, the requirements checked there, where
later fixes landed, and what remains ([review checklist](planning/review-checklist.md)).

### 7. Acceptance tests that cannot cheat

The first automated route to `WINNER` used `FUDGE`, a harness directive that reduced incoming
damage and forced recovery. That was the owner's call, made past midnight during the stack
review: "we might need to introduce a 'fudge factor' to enable proof of winning conditions. it
may not be practical to generate a play through script without cheats." The refinement loop's
ring-charge fix later broke that route.

Phase 5b replaced it under the opposite instruction. The owner told Codex "I'm not concerned
with 'cheating' short of hacking the game, we can use every tool available": the planner could
inspect core state read-only while searching, but the committed result uses only keystrokes.
It is 179,036 timestamped keys: eight typed saves, six deaths each followed by a keypress
restart and a typed `ZLOAD`, and one `WINNER`. Its verifier rejects harness directives, runs two
fresh default core replays, checks the event sequence and compares a digest ([#33],
[search log](planning/phase-5b-search-log.md)). The result supports an unassisted *core replay
under documented deviations*. It does not establish ROM-equivalent combat or an interactive
desktop death-and-load check.

### 8. Test each platform at its boundary

Each kind of automated comparison answers a different question. Source-extracted tables check
translation; committed traces catch regressions; two identical core replays establish
repeatability; a ROM capture tests only its sampled behaviour; a live screen or device test
checks the real input and presentation path. The project kept having to restore these
distinctions.

Phases 6 and 7 passed their gates, and the first hand test showed a black screen with no sound
([#20]). Headless parity had not revealed missing status and command text, lowercase keyboard
input, real-time pacing problems, absent audio routing, or carried objects drawn on the floor.
The desktop repair ([#21], [Phase 7 reconciliation](archaeology/phase-7/reconciliation.md)) was
driven by playing the game.

Phase 8 then built the touch layer headless first, in the cloud, and recorded the gap: SDL was
unavailable there and nothing had been seen on screen ([Phase 8 reconciliation](archaeology/phase-8/reconciliation.md)).
Workstream 8.6 wired those pieces into the real window locally, used offscreen scripted
screenshots in both layouts, and responded to the owner's hand test ([#32]). The record keeps
the earlier obstacle as history and adds a dated correction. A later save/load bug needed an
even more representative test: it loads a snapshot through `Shell`, then sends Move and Attack
taps through `OverlayBridge`, reproducing the stale input timeline that made the controls
unresponsive ([#42]).

The web build reused the SDL shell through WebAssembly rather than a second engine. Codex's
review found that its first IndexedDB path could report a save as stored when synchronisation
had failed ([blocking review](https://github.com/SteveLeve/daggorath/pull/48#issuecomment-5902997329)).
The fix moved saves to a synchronous storage API, reported failures to the player, added forced
quota and read-error browser tests, and later persisted the touch menu's slots too. The
[follow-up review](https://github.com/SteveLeve/daggorath/pull/48#issuecomment-5917217785) kept
real mobile browsers, offline installation and first-tap audio open. A platform failure changed
an interface and its tests, not the simulation rules.

Android followed the same pattern. [#52] was built by CI, not on a device, and said so. Its two
review findings were platform-boundary bugs: scaling and touch mapping, and background time
leaking into the jiffy clock. The fix to the second was checked by the desktop build and tests
only; resume after a long background stay was never run on a device. On the phone, an hour of
install, report and fix found the navigation-bar overlap and a dotted flash in the crisp
half-step ([#54]).

### 9. Compaction as a planned step

Context was managed deliberately. The owner asked before acting ("how will compacting context
impact our scheduled loop?"), asked the agent to prepare ("update the refinement-loop.md doc
and prepare for compaction. We have a lot of tool use clutter in the context we're better off
without"), and then compacted with an explicit keep-list written by the agent:

> /compact Keep: refinement loop state lives in docs/planning/refinement-log.md, the loop
> prompt in refinement-loop.md, working branch refinement/playthrough-and-discovery, cron job
> 0f47c963, Phase 5b prompt at docs/prompts/phase-5b-honest-playthrough.md, ROM capture tools
> in ~/coco-tools via tools/rom/env.sh. Drop tool output detail.

Every keep-list names files, branches and commit hashes, and drops tool output. Because state
already lived in the repository, a compaction lost little. The local logs record 10 compactions
in Claude Code and 10 inside the single long Codex session.

Compaction was also paired with a model change. In Codex the owner planned Phase 5b at high
effort, then wrote: "next I will compact context and switch to a lower model (Sol) for
execution. make any final adjustments before we move to implementation." In Claude Code the
pickup-from-Cursor instruction said "Delgate tasks to sub-agents using less powerful models."
The Phase 8 cloud session was likewise switched to a smaller model before it executed a plan
written at a higher one.

### 10. Parallel lanes sized to the limits

By 27 September several things were running at once, and the owner kept them from colliding
with worktrees and branch discipline:

| Lane | Where | Driver |
|---|---|---|
| Refinement loop, runs 17–29 | Main checkout, `refinement/*` branches | Claude Code cron |
| Phase 5b playthrough search | Sibling worktree | Codex goal |
| Phase 8 touch, headless parts | Cloud | Claude Code on the web, [#24]–[#30] |
| Phase 8.6 SDL wiring | Second sibling worktree | Claude Code |

Cloud sessions took work that needed no local hardware: planning documents, headless adapters
and tests, licensing, the Android shell built by CI. Local sessions took anything needing the
ROM, MAME, a display or the phone. The cloud transcripts show how each gap was handled:

- **No SDL3 in the container** ([#25]–[#30]). The session recorded it once, in
  `docs/architecture/touch-input.md` §7 and a header comment, and kept the PR bodies honest:
  "no on-screen evaluation was possible, no default was chosen by this project". The local
  8.6 lane did the on-screen part ([#32]).
- **No Android SDK reachable** ([#52]). The session wrote the Gradle project and let the
  first CI run be the first build: "no APK has been built yet. This container can't reach the
  Android SDK". iOS was deferred because it needs a Mac.
- **A reference port on the owner's disk** ([#24]). The session wrote a read-only study
  prompt; the owner ran it locally ("write the prompt, I'll run it locally and share the
  result") and attached the report.

The limits are in the logs, and the work routed around each:

- **Cursor premium-model allowance**, exhausted late on 25 Sept during the stack review
  ("Switched to grok-4.6 after reaching Other Models usage limit"). The owner resumed on the
  cheaper model and added "use lower power models for sub agents".
- **Cursor monthly allowance**, exhausted 26 Sept about 14:40. Claude Code resumed from a
  pasted transcript within minutes.
- **Claude monthly spend limit**, hit 27 Sept 23:59 and again 28 Sept 12:01. The Codex lane
  kept running through the first; after the second the owner wrote "we are close to usage
  limit. defer the next run until 3:30 pm" and queued the feedback for that run.
- **Claude session limit**, hit 29 Sept 16:02, resetting 19:40. Resumed with "resume work, my
  limit has refreshed".
- **Codex**'s first sessions, Phase 1 review attempts on 25 Sept, ended interrupted or with API
  401 errors.
  On 30 Sept one review session had no GitHub access, so a separate session diagnosed the CLI
  token and another posted the review.
- **Automatic approval** blocked Codex from posting its [#33] review, so that review exists only
  in the session log.
- **Copilot review quota**, reached 30 Sept: the second Copilot review of [#52] was refused
  ("the user who requested the review has reached their quota limit").

The cost of parallel lanes was merge conflicts, nearly all in shared documents: the
reconciliation files, the scheduler specification, and `sdl_app.cpp`. Four separate
"resolve the merge conflicts" instructions appear in the record. Thirteen stale worktrees were
cleaned up on 30 September.

### 11. The owner decides, tests and points

The owner's contribution is concentrated in a few kinds of message.

**Domain knowledge, early.** The first Phase 0 report and the charter's examples treated the
dungeon as freely seeded. The owner, who had played the original, objected before any code
existed: "the original game had fixed maps and timed events such as creature spawning. A
faithful reproduction should adopt the same maps and game sequence as the original even if we
introduce randomness later." ChatGPT checked `DGNGEN.ASM`, confirmed that each level builds
from a fixed seed and reads the clock only afterwards, and rewrote the report, the
instructions and the test plan. The rule "Original Mode takes no player-supplied seed and
generates no new maps" in today's `CLAUDE.md` descends from that one message, and the Phase 0b
prompt opens with it as a "Preservation requirement".

**Decisions with reasons.** "I've decided to apply the recommended resolution for issue #13
counting the 377 interrrupts in order to preserve fidelity." "The basis is good enough - I do
own a physical copy of the game, localize and use the ccc file but do not commit it to the
repository." Web hosting moved from public to local-only after an audit showed public hosting
is distribution. In the cloud sessions, decisions mostly arrived as answers to the agent's own
multiple-choice questions. Phase 8 planning took thirteen decisions and licensing four; all
nine multiple-choice answers took the option the agent had marked as recommended.

**Hand tests reported as symptoms.** "pulled items are showing up on the floor, the heart is
rendering with letters 'SS' overlapping … The snake seems to one-shot our character." "snake
damage seems better, but I'm not hearing the snake attack / hit sound, instead I'm hearing the
rattle / move sound." These came from memory of the original and from playing the build, and
they found what parity tests did not. On Android the same pattern ran in an hour: install,
report the navigation bar overlap, file an issue, fix, reinstall, report the dotted flash,
fix.

**Pushing on a diagnosis.** "I'd think that a faithful adaptation would land in the same place
with integrer math, I don't understnad the cause of the drift." That led to the HSLOW healing
constant.

**Correcting drift.** During the stack review Cursor began writing "Related to #8. Does not
close it." The owner stopped that in one message: "These PRs are isometric with the issues. The
PR MUST close the Issue."

**Scope control.** "this PR is getting pretty big … it might be good to mege this back to main
and start a new branch". "I agree, mark it deferred."

**Resources.** Firmware, the cartridge image, the phone, other ports cloned for comparison,
and a choice of which tool had allowance left.

### Other techniques worth recording

- **Issues as phase gates.** Each phase became a GitHub issue with a definition of done; goals
  referenced issue numbers; PRs closed them.
- **Reference ports as an instrument, not a source.** A locally hosted copy of the web port
  was driven by headless Chrome for side-by-side scripted runs. A separate read-only study of
  its renderer was run under a prompt forbidding copying and limiting quotation.
- **Plan in one context, implement in another.** Cursor plan files, Codex's "Implement the
  plan in a fresh context" and Claude Code's plan mode were all used; a plan written at high
  effort was handed to a cheaper execution run.
- **Research delegated sideways.** Gemini's answers were pasted in whole and the coding agent
  was asked to judge them: "will any of these work for us? … what do we gain or lose by
  solving this problem now vs later or not at all." The licensing session declined three of
  Gemini's suggestions after checking them against the repository.
- **Licensing as an engineering constraint.** Copied historical data is listed separately from
  the project's own work, generated headers carry a rights notice their generators reproduce,
  and builds carrying that data stay private ([licensing decisions](licensing/README.md)).
- **Session memory.** A hook wrote a one-line summary of each working block, loaded at the
  start of the next session. It is also what made this reconstruction possible.

## What did not work

- **A goal with an unsatisfiable gate loops.** Phase 1 without firmware re-prompted 134 times.
- **Stacked PRs made in one run blur ownership.** A single goal spanning five phases produced
  [#16]–[#20]; [technique 6](#6-a-different-model-reviews-at-the-revision-that-owns-the-requirement)
  covers the consequences.
- **Green tests hid a blank window.** Phases 6 and 7 passed their gates and the first hand
  test showed a black screen with no sound.
- **An assisted acceptance test.** The `FUDGE` playthrough proved a win the game had not
  allowed; it took a later bug fix to break that route and a separate phase to replace it.
- **Baseline regeneration beyond approval.** One loop run regenerated the Phase 0b traces
  when only Phase 3 baselines had been approved; the daily note records it as such.
- **An agent merged without asking.** The Phase 8 cloud session merged [#25] on its own
  initiative before reporting it as a mistake.
- **Audits lagging commits** when a limit or a blocked sub-agent interrupted a run.
- **Stale documents.** The README and `android/README.md` still say no device run occurred.

## What to reuse

1. End each session by having the agent write the next session's prompt, and commit it.
2. Keep state in files the agent must read first. Treat conversation history as disposable.
3. Use a persistent goal for bounded work and a logged loop for open-ended discovery. Put the
   stop condition and "blocked is an answer" in the prompt.
4. Write the evidence boundary into every task. A source reading, a self-generated fixture, a
   ROM capture and a device observation answer different questions; make each claim say which
   one supports it.
5. Install hooks and read-only audit agents before the bulk of the code exists.
6. Have a different model review, restricted to reporting, and feed its report back verbatim.
   Review acceptance criteria at the revision that owns them, and record the commit and what
   remains.
7. Keep the simulation repeatable, then test the outer path on each platform: keyboard, tap,
   storage, sound and device.
8. Compact on purpose, with a keep-list of paths and hashes.
9. Run lanes in separate worktrees and send hardware-free work to the cloud. Expect the cloud
   lane to lack displays and SDKs, and have it record that once.
10. Preserve corrections and unfinished work. A recorded change of mind is more useful than a
    seamless story of progress.
11. Spend human attention on decisions and on looking at the running program.

## Sources, method and gaps

| Source | What it holds | Limit |
|---|---|---|
| ChatGPT shared conversation, "Plan mobile remake licensing tech stack" (24 Sept) | Opening request, licensing and stack analysis, the generated charter and Phase 0 prompt | Read from the share page's embedded data; reasoning summaries only partly present |
| ChatGPT shared conversation, "Archaeology Report Update" (24 Sept) | The owner's fixed-map correction to the Phase 0 report, and the generated Phase 0b prompt | The share shows the report being read and patched as a project file, not the run that first wrote it |
| Claude Cowork chat "Execute prompt file" (24 Sept) | The Phase 0b run and the repository reorganisation | Read as rendered page text; tool steps are collapsed to counts |
| `docs/.transcripts/` (git-ignored) | 16 exported Cursor chats, 25–26 Sept, 15 distinct | Files 10 and 12 are identical; Cursor's own store holds 17 transcripts |
| `~/.claude/projects/*daggorath*` | 112 Claude Code sessions: 35 interactive, 77 automated | Cloud sessions (claude.ai/code) are not stored locally |
| Claude Code cloud sessions linked from [#12], [#24]–[#30], [#46], [#52] (five sessions) | Owner messages, multiple-choice answers, plan approvals, turn records with durations, session metadata | Read on 1 October through the Claude Code session API. Owner messages, answers and turn summaries were read in full; individual tool calls were sampled, not reviewed one by one |
| `~/.codex/sessions`, with the owner's Codex session inventory (not committed) | 30 Codex sessions, 25–30 Sept, whose working directory was this repository or the Phase 5b worktree; each listed by ID with its recorded result | The inventory labels start times UTC, but they are US Central: the timestamps embedded in the session IDs are five hours later. It reports what each log says and reruns nothing. An earlier count of 32 was not reproduced and is replaced by this rule. Token counts are per-session totals as logged |
| The owner's report on five ChatGPT project chats (not committed) | Summaries of the planning and archaeology chats, the [#12] review, the [#13] discussion and a workflow retrospective | Model-written summaries without timestamps; the last three chats have not been read directly. Times above come from the matching GitHub comments |
| `.remember/` daily logs | Timestamped one-line summaries of every working block | Written by a summarising model; used here for sequence, not for claims |
| Git and GitHub | 352 commits, 31 merged PRs, 24 issues, CI runs, PR descriptions and review threads through [#54] | Co-author trailers undercount agent work |

The evidence techniques (4, 6, 7 and 8) were first written from the public repository alone,
and each cites the PR, ADR or reconciliation it rests on. A PR body is its author's report; a
review comment can contain an independent check, but its result applies to the stated commit
and environment. The checks run for this document are in
[What was built](#what-was-built-and-how-far-it-got). Quotations from owners' prompts keep
their original spelling.

| Gap | What is known | What would close it |
|---|---|---|
| Codex review of PR [#12] | Not in the local Codex logs, which show only the failed Phase 1 review attempts and a connectivity check on 25 Sept. The owner's report on the ChatGPT project names the chat "Review PR Recommendations" as its source; otherwise it is known only from its PR comments | Reading that chat directly |
| First draft of the Phase 0 report | The recovered ChatGPT share covers its correction and the Phase 0b prompt; the report already exists as a project file there | The earlier turns or chat in the same ChatGPT project, if the research method matters |
| Tool-level detail of the five cloud sessions | Owner messages, answers, turn records and PR outcomes are read (see [Cloud sessions](#cloud-sessions)) | A per-tool-call review, if a finer cost or timing breakdown matters |
| Cloud session behind [#22] (touch-control mockups, 26–27 Sept) | Linked from the PR body; not in the scope of this pass | Read it the same way as the other five |
| Two Cursor chats | Cursor's local store holds 17 transcripts; 15 distinct ones are exported | Export, if they are not empty or trivial |
| Gemini conversations | Only the pasted excerpts survive | Export, if the research framing matters |
| Subscription tiers and actual spend | Not in any log | Owner's account records |

[#1]: https://github.com/SteveLeve/daggorath/pull/1
[#3]: https://github.com/SteveLeve/daggorath/issues/3
[#11]: https://github.com/SteveLeve/daggorath/issues/11
[#12]: https://github.com/SteveLeve/daggorath/pull/12
[#13]: https://github.com/SteveLeve/daggorath/issues/13
[#14]: https://github.com/SteveLeve/daggorath/issues/14
[#15]: https://github.com/SteveLeve/daggorath/pull/15
[#16]: https://github.com/SteveLeve/daggorath/pull/16
[#17]: https://github.com/SteveLeve/daggorath/pull/17
[#18]: https://github.com/SteveLeve/daggorath/pull/18
[#20]: https://github.com/SteveLeve/daggorath/pull/20
[#21]: https://github.com/SteveLeve/daggorath/pull/21
[#22]: https://github.com/SteveLeve/daggorath/pull/22
[#23]: https://github.com/SteveLeve/daggorath/pull/23
[#24]: https://github.com/SteveLeve/daggorath/pull/24
[#25]: https://github.com/SteveLeve/daggorath/pull/25
[#30]: https://github.com/SteveLeve/daggorath/pull/30
[#31]: https://github.com/SteveLeve/daggorath/pull/31
[#32]: https://github.com/SteveLeve/daggorath/pull/32
[#33]: https://github.com/SteveLeve/daggorath/pull/33
[#36]: https://github.com/SteveLeve/daggorath/pull/36
[#37]: https://github.com/SteveLeve/daggorath/issues/37
[#38]: https://github.com/SteveLeve/daggorath/pull/38
[#40]: https://github.com/SteveLeve/daggorath/issues/40
[#41]: https://github.com/SteveLeve/daggorath/pull/41
[#42]: https://github.com/SteveLeve/daggorath/pull/42
[#44]: https://github.com/SteveLeve/daggorath/pull/44
[#45]: https://github.com/SteveLeve/daggorath/issues/45
[#46]: https://github.com/SteveLeve/daggorath/pull/46
[#48]: https://github.com/SteveLeve/daggorath/pull/48
[#50]: https://github.com/SteveLeve/daggorath/pull/50
[#52]: https://github.com/SteveLeve/daggorath/pull/52
[#54]: https://github.com/SteveLeve/daggorath/pull/54
