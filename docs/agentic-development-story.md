# The agentic development story

*A companion to the [project case study](case-study.md). That study deliberately used only the
public repository. This one reconstructs how the work was actually driven, from the private
record: the originating ChatGPT conversation, Cursor transcripts, local Claude Code and Codex
session logs, and the daily session-memory notes. Snapshot: `main` at `b8afeb8`, written
1 October 2026. Times are US Central.*

## Summary

One person took *Dungeons of Daggorath* from a planning chat to a deterministic C++20 core, an
SDL3 desktop build, a local WebAssembly PWA and a debug Android APK running on a phone in about
six and a half days (24 September 12:11 to 30 September 22:55). The work used ordinary
subscriptions to ChatGPT/Codex, Claude, Cursor and GitHub Copilot, plus Gemini for research. No
agent framework was written.

The owner wrote almost no code and very few long prompts. Across roughly 110 interactive
sessions the typical instruction is one or two sentences. What the owner did supply was
decisions, hand tests, and a steady insistence that each agent write the *next* agent's
instructions. The techniques below are ranked by how much of the result they explain.

## Sources and their limits

| Source | What it holds | Limit |
|---|---|---|
| ChatGPT shared conversation, "Plan mobile remake licensing tech stack" (24 Sept) | Opening request, licensing and stack analysis, the generated charter and Phase 0 prompt | Read from the share page's embedded data; reasoning summaries only partly present |
| `docs/.transcripts/` (git-ignored) | 10 exported Cursor chats, 25–26 Sept | Files 01 and 02 are identical; there is no file 08; Cursor's own store holds 17 transcripts, so about 7 are not exported |
| `~/.claude/projects/*daggorath*` | 112 Claude Code sessions: 35 interactive, 77 automated | Cloud sessions (claude.ai/code) are not stored locally |
| `~/.codex/sessions` | 32 Codex sessions that touch the project | Token counts are per-session totals as logged |
| `.remember/` daily logs | Timestamped one-line summaries of every working block | Written by a summarising model; used here for sequence, not for claims |
| Git and GitHub | 352 commits, 31 merged PRs, 24 issues, CI runs | Co-author trailers undercount agent work |

Three things could not be recovered and are listed under [Gaps](#gaps-and-what-would-close-them):
the session that produced the Phase 0 report, the session that built the Phase 0b core, and the
cloud sessions behind ten PRs.

Token and tool counts below are measurements of the local logs. They are not costs, and they
do not compare tools fairly: the products log different things.

## Timeline

| When | Tool | What happened |
|---|---|---|
| 24 Sept 12:11 | ChatGPT (GPT-5.6 Thinking) | Opening discussion: licence, stack, sequence. Output: project instructions and a Phase 0 archaeology prompt. |
| 24 Sept 19:02 | — | Repository initialised with those two files, then the Phase 0 report and a 19-line Phase 0b prompt. |
| 24 Sept 21:01 | Claude (session not recovered) | Phase 0b: 15 source-extracted fixtures, five traces, timing spec, headless core slice, 74 files. Claude Code CLI wrote the commit message. |
| 25 Sept morning | Cursor `/goal` | Phase 1 implementation. Claude Code reviews it as "software-complete, ROM-unverified". |
| 25 Sept 10:05–13:05 | Claude Code | `claude-automation-recommender` run; hooks, audit agents and skills committed (`c9a0401`). Copilot generates `AGENTS.md`. |
| 25 Sept 13:32 | Claude Code cloud | Planning PR #12: roadmap, ADRs 0001–0008, twelve phase prompts, capture backlog. Phases become issues #2–#11. |
| 25 Sept 14:11–16:44 | Claude Code + Gemini | Cursor is looping on missing firmware. Claude diagnoses, Gemini research identifies options, owner supplies firmware, five ROM captures run. The 377-interrupt divergence is found. |
| 25 Sept 19:21–22:13 | Cursor `/goal` | Phases 2–7 in one evening: PR #15, then a five-PR stack #16–#20. 65 automatic goal continuations in the phases 3–7 chat. |
| 26 Sept morning | Cursor | Desktop demo, first hand test ("I see only a black screen"), play-bug plan and fixes, port comparison report. |
| 26 Sept ~14:40 | Cursor → Claude Code | Cursor credits exhausted. Claude Code picks up from a pasted transcript. |
| 26 Sept 18:38 | Claude Code | Hourly refinement loop starts (cron `17 * * * *`). Runs 1–29 over about 27 hours. |
| 27 Sept 10:12 – 28 Sept 12:05 | Codex | Phase 5b honest playthrough: plan at high effort, compact, execute as a goal on a lower model in a dedicated worktree. |
| 27 Sept 11:50–16:58 | Claude Code cloud | Phase 8 planning PR #24, then six headless touch PRs #25–#30 opened within 55 minutes. |
| 27 Sept 17:24 – 28 Sept 15:53 | Claude Code | Phase 8.6 SDL wiring in a second worktree, then a touch-overlay loop driven by hand-test feedback. PR #32. |
| 28 Sept 15:13 – 30 Sept 15:43 | Claude Code | Track R ROM captures C-09 to C-21 across four worktrees and PRs #36, #41, #44, #50. |
| 28 Sept 16:06–18:14 | Codex | Crisp-vector shading (#38) and the touch-after-load fix (#42), each from a one-paragraph bug report. |
| 28 Sept 18:20 – 30 Sept 13:30 | Claude Code, Codex review | Web stopgap PR #48. Codex's review finds the save-durability defect. |
| 30 Sept 16:15–21:54 | Claude Code cloud, Copilot review | Phase 9 Android shell, PR #52. CI builds the APK. |
| 30 Sept 21:33–22:55 | Claude Code | APK installed on the owner's phone; navigation-bar overlap and crisp half-step fixed from live feedback, PR #54. |

Commits per day show the shape: 4, 135, 43, 101, 45, 12, 11. Two days of generation, then
progressively slower, more deliberate work as the remaining problems needed a human looking at
a screen or a ROM capture.

## Who did what

| Tool | Role | Evidence |
|---|---|---|
| ChatGPT | Framing: licence reading, stack choice, charter, first prompt | The shared conversation |
| Claude (desktop, CLI, cloud) | Planning and ADRs; harness setup; unblocking; loops; Phase 8, Track R, web, Android; review | 35 interactive local sessions, 10 cloud PRs, 28 commits authored as `Claude` |
| Cursor | Bulk implementation of phases 1–7 under `/goal`; cloud review of its own stack | 129 commits with a Cursor co-author trailer; 11 `cursor/` branches |
| Codex | Phase 5b search; two targeted fixes; independent PR reviews; the case study | 32 sessions; two stored review goals |
| GitHub Copilot | PR review on 6 PRs; `AGENTS.md`; one conflict-resolution plan | Review records on #15, #32, #36, #41, #50, #52 |
| Gemini | Research only: CoCo firmware sources, web-delivery options | Pasted into Claude prompts on 25 and 28 Sept |

The split followed the owner's stated intent. A Cursor subscription with unused allowance was a
few days from expiry, so Cursor got the work that consumes allowance fastest and needs least
judgement: executing well-specified phases. Claude wrote those specifications.

## Technique 1: every stage ends by writing the next prompt

The pattern is visible in the first hour. The opening ChatGPT request ends: "We will generate a
project plan to hand off for agentic execution." ChatGPT declined to plan immediately and
recommended a source-archaeology pass first. The owner's second message accepted that and asked
for two artifacts: persistent project instructions and "a prompt for the next useful step".

That request recurs in every tool for the rest of the week:

- Cursor, end of the first Phase 1 chat: "generate markdown instructions describing the
  remaining steps to use as a goal prompt in a fresh context".
- Cursor, after the ROM captures: "update PR, comment on progress, and write a prompt for the
  next goal".
- Claude Code, after unblocking firmware: "update or create relevant issues, PR description,
  and comment on changes. then generate a prompt for remaining work needed to complete phase 1."
- Claude Code, before the playthrough rework: "I also want to generate a prompt to begin work
  on fixing the planner & play through."

The prompts were committed, which is why `docs/prompts/` has 18 files. The next session started
with a one-line instruction pointing at a file:

> /goal implement the next phase of the project: docs/prompts/phase-1-conformance-and-creatures.md

The effect is that the expensive thinking happened at the end of a session, when the agent had
full context, and the cheap instruction happened at the start of the next, when it had none.
The case study describes these prompts as transferable task contracts. The private record adds
that they were mostly written by an agent that had just done the preceding work, and that this
was asked for explicitly each time.

Much of the project's discipline traces to the first two documents. The architecture diagram,
the `src/` layout, the rule that touch input emits original commands, the provenance classes,
"do not begin by building the mobile UI", and the Original Mode separation all appear in the
ChatGPT output and survive in `docs/project-instructions.md`. ChatGPT also added one thing
unprompted that turned out to matter most: making "historical timing and ordering an explicit
archaeology target".

## Technique 2: persistent goals for execution, with a stop condition in the prompt

Cursor's `/goal` holds an objective and re-prompts the agent whenever it stops. The phases 3–7
chat contains one owner instruction and 65 automatic continuations before the owner typed
again ("explain status and any complications"). That single goal produced the five-PR stack
#16–#20. The agent narrated progress as PR comments: PR #20 carries 56 short status comments
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
blocked`).

The failure mode also appeared on day one. Cursor's Phase 1 goal required ROM captures, the
machine had no CoCo firmware, and the agent looped, re-reporting the same blocker. The owner
had Cursor summarise its state into a troubleshooting prompt and handed that to Claude Code.
The fix went into the standing rules as "An obstacle is recorded once, in one place, with its
reason. Do not re-check or restate it every turn; mark the item 'not run: <reason>' and move
on." Later goals say "until complete or blocked", and later review goals say "'untested' is a
valid response".

## Technique 3: scheduled loops with a log as the only memory

Goals suit a task with an end. For open-ended discovery the owner asked for a loop:

> let's design an improvement loop prompt to crawl over the app and original source looking
> for missing or incorrect code in our version. … I'd like each loop to kick off a discovery
> and refinement goal and track progress in a log or memory file. … Each loop should commit and
> push any refinemnts automatically. Ping me with questions if we need a human in the loop.
> … I'd like to run the loop every hour or 90 minutes expecting a 10 - 30 minute session.

Claude Code produced `docs/planning/refinement-loop.md` (the per-run prompt),
`refinement-log.md` (state), and one cron job at 17 minutes past each hour. The prompt's second
line is the design: "Every run starts from `docs/planning/refinement-log.md`; do not rely on
conversation history."

Properties that made it work:

- **Bounded runs.** One to three targets, 10–30 minutes, full gate, then stop.
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
loop and a touch-overlay loop. The touch loop is the clearest human-in-the-loop example. The
owner hand-tested, wrote a paragraph of observations, and the next run consumed it as answers
and new targets. It also prompted a question worth keeping: "how can we get you eyes to see
with?" The answer was an offscreen SDL mode (`dod --shots`) that renders scripted taps to
screenshots the agent can read.

## Technique 4: compaction as a planned step

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

## Technique 5: mechanical guardrails installed on day two

Before most gameplay code existed, the owner ran a recommender skill ("review the project,
recommend code quality automations, custom agents & skills for this project") and committed the
result:

- a pre-edit hook, `guard-evidence.sh`, that blocks edits violating the evidence rules;
- a post-edit C++ formatter and a stop hook that runs `make verify`;
- two read-only audit agents, `evidence-auditor` and `boundary-checker`;
- skills for fixture changes and phase checks.

The case study notes that the agents' existence "does not prove they ran on every change". The
local logs give a count: `evidence-auditor` was invoked 55 times and `boundary-checker` 27
times from Claude Code sessions. The daily notes show audits finding real problems, such as
citations pointing at the wrong deviation number and append-only reconciliation files being
rewritten. They also show the limit: "unpushed (missing checks: make all, evidence-auditor,
boundary-checker)" appears more than once, and those commits were audited later, not before.

A separate automated security review ran on commits: 77 short non-interactive sessions, each
given a diff and asked for vulnerabilities. It confirmed one, a buffer over-read in
`snoise.cpp`, and raised a candidate bypass in the guard hook itself.

The same rules were mirrored for other tools: `AGENTS.md` for Copilot and Codex, and
`.codex/agents/*.toml` copies of the three audit agents. Codex created its own skill and
reviewer agent for Phase 5b when asked to "scaffold any custom skills or agents you think would
be helpful for achieving the goal".

## Technique 6: a different model reviews

Review was routinely given to a tool that had not written the code.

| Work by | Reviewed by | Outcome |
|---|---|---|
| Cursor, Phase 1 | Claude Code | "the unstaged changes do not complete Phase 1" — pasted back into Cursor as its next instruction |
| Cursor, Phase 1 PR | Cursor `/multi-model-review` using Grok 4.7 | Claim checklist, then a goal to close it |
| Claude, planning PR #12 | Separate review pass | Save-format, ownership and seeding ambiguities fixed before any phase ran |
| Cursor, stack #16–#20 | Cursor cloud agent | Per-branch reviews that separated "fixed downstream" from "satisfied here" |
| Codex, Phase 5b PR #33 | Codex in a fresh goal, then Claude applies notes | Independent replay; hash matched; 3 of 4 notes applied |
| Claude, web PR #48 | Codex `/goal` review | Found that a failed IndexedDB sync still reported a successful save |
| Claude, Track R and Android | Copilot | Six findings on #50; a background-time accumulator bug on #52 |

Two review prompts are worth reusing. The first constrains the reviewer to observation:

> /goal review PR 48, the current branch. add your report to the PR as a comment with analysis
> and recommendations. Anser if the PR is merge ready or if changes are needed. Attempt to
> verify claims if possible, "untested" is a valid response. If additional tools are necessary,
> request additional resources (MCP servers etc). Do not make changes.

The second closes the loop: "The PR has been updated addressing your comments, re-check and
validate." Reviews were relayed by copy and paste between tools, with the PR as the durable
record.

## Technique 7: parallel lanes sized to the limits

By 27 September three things were running at once, and the owner kept them from colliding with
worktrees and branch discipline:

| Lane | Where | Driver |
|---|---|---|
| Refinement loop, runs 17–29 | Main checkout, `refinement/*` branches | Claude Code cron |
| Phase 5b playthrough search | Sibling worktree | Codex goal |
| Phase 8 touch, headless parts | Cloud | Claude Code on the web, PRs #24–#30 |
| Phase 8.6 SDL wiring | Second sibling worktree | Claude Code |

Cloud sessions took work that needed no local hardware: planning documents, headless adapters
and tests, licensing, the Android shell built by CI. Local sessions took anything needing the
ROM, MAME, a display or the phone. When a cloud session lacked SDL it recorded that as an
obstacle and the local lane did the on-screen part.

The limits are in the logs, and the work routed around each:

- **Cursor monthly allowance**, exhausted 26 Sept about 14:40. Claude Code resumed from a
  pasted transcript within minutes.
- **Claude monthly spend limit**, hit 27 Sept 23:59 and again 28 Sept 12:01. The Codex lane
  kept running through the first; after the second the owner wrote "we are close to usage
  limit. defer the next run until 3:30 pm" and queued the feedback for that run.
- **Claude session limit**, hit 29 Sept 16:02, resetting 19:40. Resumed with "resume work, my
  limit has refreshed".
- **Codex** needed API troubleshooting before its first use on 25 Sept and lost GitHub access briefly on
  30 Sept.

The cost of parallel lanes was merge conflicts, nearly all in shared documents: the
reconciliation files, the scheduler specification, and `sdl_app.cpp`. Four separate
"resolve the merge conflicts" instructions appear in the record. Thirteen stale worktrees were
cleaned up on 30 September.

## Technique 8: the owner decides, tests and points

The owner's contribution is concentrated in a few kinds of message.

**Decisions with reasons.** "I've decided to apply the recommended resolution for issue #13
counting the 377 interrrupts in order to preserve fidelity." "The basis is good enough - I do
own a physical copy of the game, localize and use the ccc file but do not commit it to the
repository." Web hosting moved from public to local-only after an audit showed public hosting
is distribution.

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

**Scope control.** "this PR is getting pretty big … it might be good to mege this back to main
and start a new branch". "I agree, mark it deferred."

**Resources.** Firmware, the cartridge image, the phone, other ports cloned for comparison,
and a choice of which tool had allowance left.

## Other techniques worth recording

- **Issues as phase gates.** Each phase became a GitHub issue with a definition of done; goals
  referenced issue numbers; PRs closed them.
- **Reference ports as an instrument, not a source.** A locally hosted copy of the web port
  was driven by headless Chrome for side-by-side scripted runs. A separate read-only study of
  its renderer was run under a prompt forbidding copying and limiting quotation.
- **Plan in one context, implement in another.** Cursor plan files and Codex's "Implement the
  plan in a fresh context" were both used; a plan written at high effort was handed to a
  cheaper execution run.
- **Tools for the search, not just the answer.** For Phase 5b the owner told Codex "I'm not
  concerned with 'cheating' short of hacking the game, we can use every tool available". The
  planner could inspect core state read-only; the committed result uses only keystrokes.
- **Research delegated sideways.** Gemini's answers were pasted in whole and the coding agent
  was asked to judge them: "will any of these work for us? … what do we gain or lose by
  solving this problem now vs later or not at all."
- **Session memory.** A hook wrote a one-line summary of each working block, loaded at the
  start of the next session. It is also what made this reconstruction possible.

## Scale, as logged

| Measure | Value |
|---|---|
| Elapsed | 24 Sept 12:11 to 30 Sept 22:55 |
| Commits / merged PRs / issues | 352 / 31 / 24 |
| Tracked lines: `src/`, tests, tools, docs | 11,704 / 5,455 / 5,307 / 9,580 |
| Claude Code local sessions | 35 interactive, 77 automated review |
| Claude Code tool calls | about 4,600, of which 2,981 shell |
| Claude Code sub-agent launches | 106 (55 evidence audits, 27 boundary checks) |
| Claude Code output tokens (local sessions) | 5.0 million |
| Codex Phase 5b session | 26 hours, 1,675 tool calls, 10 compactions, 4 models |
| Cursor phases 3–7 chat | 917 tool calls, 65 goal continuations |
| Exported Cursor chats, total tool calls | about 2,200 |

## What did not work

- **A goal with an unsatisfiable gate loops.** Phase 1 without firmware is the example.
- **Stacked PRs made in one run blur ownership.** The case study's Technique 3 covers the
  consequences; the cause was a single goal spanning five phases.
- **Green tests hid a blank window.** Phases 6 and 7 passed their gates and the first hand
  test showed a black screen with no sound.
- **An assisted acceptance test.** The first "winning playthrough" used `FUDGE` directives.
  It took a later bug fix to break it and a separate phase to replace it honestly.
- **Baseline regeneration beyond approval.** One loop run regenerated the Phase 0b traces
  when only Phase 3 baselines had been approved; the daily note records it as such.
- **Audits lagging commits** when a limit or a blocked sub-agent interrupted a run.
- **Stale documents.** The README and `android/README.md` still say no device run occurred.

## Gaps and what would close them

| Gap | What is known | What would close it |
|---|---|---|
| Phase 0 archaeology report | Committed 24 Sept 19:02 with a "Research snapshot" date; no local session produced it | Export of the chat that ran the Phase 0 prompt (ChatGPT project or Claude) |
| Phase 0b evidence pack and core slice | 74 files committed 21:01; the earliest local Claude Code session only writes the commit message | Export of the Claude desktop session from the evening of 24 Sept |
| Cloud sessions for PRs #12, #24–#30, #46, #52 | Session links are in the PR bodies | Export of those claude.ai/code sessions |
| Seven Cursor chats | Present in Cursor's local store, not in `docs/.transcripts/` | Export, including the missing file 08 and the "Gameplay issues and fixes" chat |
| Gemini conversations | Only the pasted excerpts survive | Export, if the research framing matters |
| Subscription tiers and actual spend | Not in any log | Owner's account records |

## What to reuse

1. End each session by having the agent write the next session's prompt, and commit it.
2. Keep state in files the agent must read first. Treat conversation history as disposable.
3. Use a persistent goal for bounded work and a logged loop for open-ended discovery. Put the
   stop condition and "blocked is an answer" in the prompt.
4. Compact on purpose, with a keep-list of paths and hashes.
5. Install hooks and read-only audit agents before the bulk of the code exists.
6. Have a different model review, restricted to reporting, and feed its report back verbatim.
7. Run lanes in separate worktrees and send hardware-free work to the cloud.
8. Spend human attention on decisions and on looking at the running program.
