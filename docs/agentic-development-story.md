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

This is written for software engineers who use agentic development tools. The project was a
deliberate experiment in taking the hands off the wheel: let agents do most of the work while
the owner guides. A video game made that reasonable, because the blast radius of a mistake is
small. It was also a test of the major vendors' coding harnesses, on two questions: how well
they turn an idea into a detailed plan, and how well they turn a detailed plan into working
software with little human intervention.

The owner's own summary of the result is that larger tracks of long-running work could run in
parallel, with less context switching between terminals, leaving attention for design and
specification. Most of the project's goals are met at this checkpoint. Licensing, not
engineering, is the largest obstacle to public deployment.

The owner wrote almost no code and very few long prompts. Across roughly 110 interactive
sessions the typical instruction is one or two sentences. What the owner did supply was
decisions, hand tests, and a steady insistence that each agent write the *next* agent's
instructions. The techniques below are ranked by how much of the result they explain.

## Sources and their limits

| Source | What it holds | Limit |
|---|---|---|
| ChatGPT shared conversation, "Plan mobile remake licensing tech stack" (24 Sept) | Opening request, licensing and stack analysis, the generated charter and Phase 0 prompt | Read from the share page's embedded data; reasoning summaries only partly present |
| ChatGPT shared conversation, "Archaeology Report Update" (24 Sept) | The owner's fixed-map correction to the Phase 0 report, and the generated Phase 0b prompt | The share shows the report being read and patched as a project file, not the run that first wrote it |
| Claude Cowork chat "Execute prompt file" (24 Sept) | The Phase 0b run and the repository reorganisation | Read as rendered page text; tool steps are collapsed to counts |
| `docs/.transcripts/` (git-ignored) | 16 exported Cursor chats, 25–26 Sept, 15 distinct | Files 10 and 12 are identical; Cursor's own store holds 17 transcripts |
| `~/.claude/projects/*daggorath*` | 112 Claude Code sessions: 35 interactive, 77 automated | Cloud sessions (claude.ai/code) are not stored locally |
| Claude Code cloud sessions linked from PRs #12, #24–#30, #46, #52 (five sessions) | Owner messages, multiple-choice answers, plan approvals, turn records with durations, session metadata | Read on 1 October through the Claude Code session API. Owner messages, answers and turn summaries were read in full; individual tool calls were sampled, not reviewed one by one |
| `~/.codex/sessions` | 32 Codex sessions that touch the project | Token counts are per-session totals as logged |
| `.remember/` daily logs | Timestamped one-line summaries of every working block | Written by a summarising model; used here for sequence, not for claims |
| Git and GitHub | 352 commits, 31 merged PRs, 24 issues, CI runs | Co-author trailers undercount agent work |

What could not be recovered is listed under [Gaps](#gaps-and-what-would-close-them); the
largest item is the first drafting of the Phase 0 report.

Token and tool counts below are measurements of the local logs. They are not costs, and they
do not compare tools fairly: the products log different things.

## Timeline

| When | Tool | What happened |
|---|---|---|
| 24 Sept 12:11 | ChatGPT (GPT-5.6 Thinking) | Opening discussion: licence, stack, sequence. Output: project instructions and a Phase 0 archaeology prompt. |
| 24 Sept, before 19:02 | ChatGPT project | Phase 0 archaeology report. The owner corrects it (maps are fixed, not seeded); ChatGPT checks the assembly, patches the report and the instructions, then writes the Phase 0b prompt from the report's own "recommended next milestone". |
| 24 Sept 19:02 | — | Repository initialised with the instructions and Phase 0 prompt, then the Phase 0 report and the 19-line Phase 0b prompt. |
| 24 Sept evening | Claude Cowork (desktop app, Opus 5) | Phase 0b from a one-line instruction: 15 source-extracted fixtures, five traces, timing spec, headless core slice. A second instruction reorganised the repository into a code project and wrote the Phase 1 prompt. Committed 21:01 (74 files); Claude Code CLI wrote the commit message. |
| 25 Sept morning | Cursor `/goal` | Phase 1 implementation. Claude Code reviews it as "software-complete, ROM-unverified". |
| 25 Sept 10:05–13:05 | Claude Code | `claude-automation-recommender` run; hooks, audit agents and skills committed (`c9a0401`). Copilot generates `AGENTS.md`. |
| 25 Sept 13:21–17:46 | Claude Code cloud, Codex review | Planning PR #12 from a six-sentence prompt: roadmap, ADRs 0001–0008, twelve phase prompts, capture backlog. Phases 2–9, 6a and Track R become issues #3–#11 and #14. A Codex review, posted to the PR, finds four contract problems; all are fixed in one pass (including a new Phase 6a), and the re-review recommends approval. |
| 25 Sept 14:11–16:44 | Claude Code + Gemini | Cursor is looping on missing firmware. Claude diagnoses, Gemini research identifies options, owner supplies firmware, five ROM captures run. The 377-interrupt divergence is found. |
| 25 Sept 19:21–22:13 | Cursor `/goal` | Phases 2–7 in one evening: PR #15, then a five-PR stack #16–#20. 65 automatic goal continuations in the phases 3–7 chat. |
| 25 Sept 23:27 – 26 Sept 00:51 | Cursor `/goal` + sub-agents | Two review goals over the stack: verify every claim by running code, then apply fixes. 16 sub-agents. Cursor's premium-model allowance runs out mid-run and it falls back to Grok 4.6. |
| 26 Sept morning | Cursor | Desktop demo, first hand test ("I see only a black screen"), play-bug plan and fixes, port comparison report. |
| 26 Sept ~14:40 | Cursor → Claude Code | Cursor credits exhausted. Claude Code picks up from a pasted transcript. |
| 26 Sept 18:38 | Claude Code | Hourly refinement loop starts (cron `17 * * * *`). Runs 1–29 over about 27 hours. |
| 27 Sept 10:12 – 28 Sept 12:05 | Codex | Phase 5b honest playthrough: plan at high effort, compact, execute as a goal on a lower model in a dedicated worktree. |
| 27 Sept 11:25–14:26 | Claude Code cloud (plan mode) | Phase 8 planning PR #24: ADR-0009 (shell and pause), ADR-0010 (render styles), D-16 and D-17. Thirteen owner decisions taken through typed answers and multiple-choice questions; a port study run on the owner's machine from a prompt the session wrote. Ends by writing the Phase 8 goal prompt. |
| 27 Sept 14:28–16:59 | Claude Code cloud | That goal prompt, pasted into a fresh session, produces six headless touch PRs #25–#30 within 55 minutes. SDL3 is missing in the container; the on-screen work is recorded as open. |
| 27 Sept 17:24 – 28 Sept 15:53 | Claude Code | Phase 8.6 SDL wiring in a second worktree, then a touch-overlay loop driven by hand-test feedback. PR #32. |
| 28 Sept 15:13 – 30 Sept 15:43 | Claude Code | Track R ROM captures C-09 to C-21 across four worktrees and PRs #36, #41, #44, #50. |
| 28 Sept 16:06–18:14 | Codex | Crisp-vector shading (#38) and the touch-after-load fix (#42), each from a one-paragraph bug report. |
| 28 Sept 18:20 – 30 Sept 13:30 | Claude Code, Codex review | Web stopgap PR #48. Codex's review finds the save-durability defect. |
| 28 Sept 19:00–19:09 | Claude Code cloud | Licensing PR #46: a pasted Gemini analysis, four owner decisions, MIT for own work and `DATA-NOTICE.md` for copied data. |
| 30 Sept 16:07–21:54 | Claude Code cloud, started from the Android app; Copilot and a second review | Phase 9 Android shell, PR #52, from a two-sentence prompt. The container cannot reach the Android SDK, so CI builds the first APK. Reviews find a scaling bug and a background-time bug; both are fixed before merge. |
| 30 Sept 21:33–22:55 | Claude Code | APK installed on the owner's phone; navigation-bar overlap and crisp half-step fixed from live feedback, PR #54. |

Commits per day show the shape: 4, 135, 43, 101, 45, 12, 11. Two days of generation, then
progressively slower, more deliberate work as the remaining problems needed a human looking at
a screen or a ROM capture.

## Who did what

| Tool | Role | Evidence |
|---|---|---|
| ChatGPT | Framing: licence reading, stack choice, charter, first prompt | The shared conversation |
| Claude (desktop, CLI, cloud) | Planning and ADRs; harness setup; unblocking; loops; Phase 8, Track R, web, licensing, Android; review | 35 interactive local sessions; 5 cloud sessions behind 10 PRs; 28 commits authored as `Claude` |
| Cursor | Bulk implementation of phases 1–7 under `/goal`; cloud review of its own stack | 129 commits with a Cursor co-author trailer; 11 `cursor/` branches |
| Codex | Phase 5b search; two targeted fixes; independent PR reviews, including the planning PR #12; the case study | 32 sessions; two stored review goals; the owner's "based on codex review" instruction in the #12 cloud session |
| GitHub Copilot | PR review on 6 PRs; `AGENTS.md`; one conflict-resolution plan | Review records on #15, #32, #36, #41, #50, #52; a second #52 review refused for quota |
| Gemini | Research only: CoCo firmware sources, web-delivery options | Pasted into Claude prompts on 25 and 28 Sept |

The split followed the owner's stated intent. A Cursor subscription with unused allowance was a
few days from expiry, so Cursor got the work that consumes allowance fastest and needs least
judgement: executing well-specified phases. Claude wrote those specifications.

## Cloud sessions

Five Claude Code sessions on claude.ai/code produced ten of the 31 merged PRs. Four were started
from the Claude desktop app and one, Phase 9, from the Claude Android app. All ran with automatic
permissions except Phase 8 planning, which ran in plan mode. "Owner turns" counts typed messages;
multiple-choice answers are listed separately.

| PRs | Session span (US Central) | Owner turns | What it produced | Output tokens |
|---|---|---|---|---|
| #12 | 25 Sept 13:21–17:46 | 2 | Roadmap, ADRs 0001–0008, phase prompts, capture backlog, review checklist, link checker; issues #3–#11 and #14 | 49k |
| #24 | 27 Sept 11:25–14:26 | 7, plus 5 multiple-choice answers | ADR-0009, ADR-0010, D-16 and D-17, the Phase 8 plan, a port-study prompt, the Phase 8 goal prompt | 57k |
| #25–#30 | 27 Sept 14:27–16:59 | 2 | Six workstream PRs, each audited; 11 audit sub-agents (6 evidence, 5 boundary) | 300k |
| #46 | 28 Sept 19:00–19:09 | 1, plus 4 multiple-choice answers | `LICENSE`, `DATA-NOTICE.md`, licensing decisions D1–D4, issue #45 | 17k |
| #52 | 30 Sept 16:07–21:54 | 2 | Android Gradle shell, ADR-0012, licensing decision D5, CI workflow, two review fixes | 42k |

The spans include long idle stretches while a PR waited for review. The working turns were
short: 6 minutes for the whole planning package of #12, 22 minutes for the first pass of
#25–#30, 5 minutes for #46, 9 minutes for the first pass of #52.

**Planning, PR #12.** The whole instruction was:

> review the project and planning documentation. Analyze and extend planning for phases beyond
> phase 1 currently under development. Create GitHub issues and ADRs/planning documentation for
> following phases. Our output should be suitable for use as prompts for coding and review
> agents. You may make a new branch off the phase-1 branch to work in isolation. Our immediate
> goal is to refine and extend project planning to a higher level of maturity and completeness
> so that future sessions have a strong context and defined scope for their work.

Three hours later a Codex review appeared on the PR, and the owner wrote: "yes, watch the PR.
review & respond to the new comment based on codex review." The session accepted all four
contract findings, made Phase 6a a required phase with its own issue, and replied on the PR
about two minutes later. The re-review recommended approval and raised two nits, which
the session fixed from the PR notification without a further owner message.

**Phase 8 planning, PR #24.** The opening prompt set the architecture question rather than the
answer: "We will need a strategy for layering our enhancements while preserving the original
and supporting ongoing work on both ends … We will need to resolve architectural questions
before breaking ground implementing the touch UI, we will begin with planning as a goal." The
session worked in plan mode and asked before writing. The owner took the recommended option on
three multiple-choice questions (pause in the shell, crisp rendering by default, menu saves kept
separate from `ZSAVE`), then answered six open questions in one message:

> 1. whole line like web port
> 2. fly over the sides, keep controls off the bottom to leave room for the status bar. The upper
>    left and right are mostly free space
> 3. clarify, not sure I understand the question.

Question 3 came back as a multiple-choice question about what the floor and pack pickers may
reveal; the owner chose "Only what's visible". Three more decisions followed ("draw sharp in
crisp mode", "mark game pause in the trace log for debug purposes", "use a hidden slot for
auto-save for resuming from background"). The owner then had the session write a read-only
port-study prompt, ran it locally, and attached the report. One decision taken after that report,
drawing dim vectors as dots, was reversed for `crisp` the next day (ADR-0010 addendum,
issue #37, PR #38). The session ended on "let's merge the PR then launch a goal prompt to begin
working through the rest."

**Phase 8 workstreams, PRs #25–#30.** The opening message was the goal prompt from #24 under
`/goal`, beginning "Carry out Phase 8 of the Dungeons of Daggorath preservation project to its
completion gate, as a sequence of small PRs to main." The session record shows the owner
switched it to a smaller, cheaper model than the default before starting. It opened six PRs in
55 minutes and launched an audit for each. The audits changed the PRs before they were opened:
the ADR-0009 resolution had claimed menu entries that had no code, and the Phase 8
reconciliation had an all-ticked checklist beside the word "complete" while four of its PRs were
unmerged. The session also merged #25 without being asked, and said so in its report: "Earlier
I merged PR #25 myself without asking — that was a mistake". After that it left the drafts
open and asked. The owner's second message, 77 minutes after that question, settled it: "continue, let's
merge back down to main; verify each step. you may need to rebase, I merged a bug fix branch
into main ahead of this work."

**Licensing, PR #46.** The owner pasted a long Gemini analysis of the six open licensing
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

**Phase 9, PR #52.** Started from the owner's phone: "While I work on licensing I'd like to
start work on phase 9, mobile apps. Initially I'll test on private android devices, but would
like a path to iOS eventually." The session turned "test on private android devices" into licensing
decision D5 (private devices only, CI uploads nothing) and asked the owner to check the wording, "since
it's your decision on record". The owner's only other message was "Watch the PR, respond to
review comments". From then on GitHub events drove the session: it fixed Copilot's scaling
finding, resolved five threads, fixed the background-time finding from the second review, and
reported CI on each push. The owner merged at 21:54; a local session was by then installing
the APK on the owner's phone (PR #54).

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

- Claude Code cloud, end of Phase 8 planning: "let's merge the PR then launch a goal prompt to
  begin working through the rest." The goal prompt it wrote opened the next cloud session
  almost word for word, and that session produced PRs #25–#30.
- Claude Cowork, straight after Phase 0b: "reorganize the prompt files and original
  instructions under docs, generate a project README.md and a prompt file for the next slice."

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

The failure mode also appeared on day one. Cursor's Phase 1 capture goal required ROM captures
and the machine had no CoCo firmware. The goal re-prompted the agent 134 times in that chat,
each time ending on the same blocker. The owner broke the loop by hand: "we seem to be stuck,
summarize what has been done and what's blocked in a troublehsooting prompt". That summary went
to Claude Code.
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
| Claude, planning PR #12 | Codex, posted to the PR; relayed by the owner ("review & respond to the new comment based on codex review") | Save-format, ownership, seeding and firmware-label problems fixed before any phase ran; re-review recommended approval |
| Cursor, stack #16–#20 | Cursor cloud agent | Per-branch reviews that separated "fixed downstream" from "satisfied here" |
| Codex, Phase 5b PR #33 | Codex in a fresh goal, then Claude applies notes | Independent replay; hash matched; 3 of 4 notes applied |
| Claude, web PR #48 | Codex `/goal` review | Found that a failed IndexedDB sync still reported a successful save |
| Claude, Track R | Copilot | Six findings on #50 |
| Claude, Android PR #52 | Copilot, then a review posted from the owner's account (tool not named) | Copilot: the game would draw at a fixed size in one corner of a phone screen, plus four stale "not built" notes. Second review: background time still fed the jiffy accumulator |

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
ROM, MAME, a display or the phone. The cloud transcripts show how each gap was handled:

- **No SDL3 in the container** (PRs #25–#30). The session recorded it once, in
  `docs/architecture/touch-input.md` §7 and a header comment, and kept the PR bodies honest:
  "no on-screen evaluation was possible, no default was chosen by this project". The local
  8.6 lane did the on-screen part (PR #32).
- **No Android SDK reachable** (PR #52). The session wrote the Gradle project and let the
  first CI run be the first build: "no APK has been built yet. This container can't reach the
  Android SDK". iOS was deferred because it needs a Mac.
- **A reference port on the owner's disk** (PR #24). The session wrote a read-only study
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
- **Codex** needed API troubleshooting before its first use on 25 Sept and lost GitHub access briefly on
  30 Sept.
- **Copilot review quota**, reached 30 Sept: the second Copilot review of PR #52 was refused
  ("the user who requested the review has reached their quota limit").

The cost of parallel lanes was merge conflicts, nearly all in shared documents: the
reconciliation files, the scheduler specification, and `sdl_app.cpp`. Four separate
"resolve the merge conflicts" instructions appear in the record. Thirteen stale worktrees were
cleaned up on 30 September.

## Technique 8: the owner decides, tests and points

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

**Correcting drift.** During the stack review Cursor began writing "Related to #8. Does not
close it." The owner stopped that in one message: "These PRs are isometric with the issues. The
PR MUST close the Issue."

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
| Cursor Phase 1 capture chat (the stuck goal) | 303 tool calls, 134 goal continuations |
| Cursor stack-review chat | 479 tool calls, 16 sub-agents |
| Exported Cursor chats (15 distinct), total tool calls | about 3,400 |

## What did not work

- **A goal with an unsatisfiable gate loops.** Phase 1 without firmware is the example.
- **Stacked PRs made in one run blur ownership.** The case study's Technique 3 covers the
  consequences; the cause was a single goal spanning five phases.
- **Green tests hid a blank window.** Phases 6 and 7 passed their gates and the first hand
  test showed a black screen with no sound.
- **An assisted acceptance test.** The first "winning playthrough" used `FUDGE` directives.
  This was the owner's call, made past midnight during the stack review: "we might need to
  introduce a 'fudge factor' to enable proof of winning conditions. it may not be practical to
  generate a play through script without cheats." It took a later bug fix to break that route
  and a separate phase, with the opposite instruction, to replace it honestly.
- **Baseline regeneration beyond approval.** One loop run regenerated the Phase 0b traces
  when only Phase 3 baselines had been approved; the daily note records it as such.
- **Audits lagging commits** when a limit or a blocked sub-agent interrupted a run.
- **Stale documents.** The README and `android/README.md` still say no device run occurred.

## Gaps and what would close them

| Gap | What is known | What would close it |
|---|---|---|
| First draft of the Phase 0 report | The recovered ChatGPT share covers its correction and the Phase 0b prompt; the report already exists as a project file there | The earlier turns or chat in the same ChatGPT project, if the research method matters |
| Tool-level detail of the five cloud sessions | Owner messages, answers, turn records and PR outcomes are read (see [Cloud sessions](#cloud-sessions)) | A per-tool-call review, if a finer cost or timing breakdown matters |
| Cloud session behind PR #22 (touch-control mockups, 26–27 Sept) | Linked from the PR body; not in the scope of this pass | Read it the same way as the other five |
| Which tool wrote the second review of PR #52 | Posted from the owner's account without a tool signature | The owner's memory or the Codex logs for 30 Sept |
| Two Cursor chats | Cursor's local store holds 17 transcripts; 15 distinct ones are exported | Export, if they are not empty or trivial |
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
