# Flawy

Flawy is a code-only, multiplatform C++20 game engine. The engine is a single static library target (`Flawy::Flawy`) that carries all its dependencies. `Flawy-Sandbox/` is a console host application. Future hosts, such as a visual editor, must only need to link `Flawy::Flawy`.

The user communicates in Hungarian. Answer in Hungarian. Code, identifiers and agent prompts stay in English.

## Key documents

- `docs/toolset.md`: dependencies, pinned versions, and how to add a new one
- `docs/architecture.md`: folder layout, modules, link visibility, how to build
- `docs/coding-standards.md`: binding code rules (no comments, OOP, naming)

## Build

```
scripts/install.bat              # fetches pinned dependencies into vendor/ (wraps install.ps1; install.sh on Linux/macOS)
cmake --preset vs2026
cmake --build --preset vs2026-debug
build/vs2026/bin/Debug/Flawy-Sandbox.exe
```

Dependencies come only from `scripts/dependencies.txt` through the install scripts. No package managers, no FetchContent, no git submodules in this repo. Never edit `vendor/`.

## Agentic workflow (mandatory for every engine task)

Every code task goes through three agents:

1. **`engine-planner`** (Fable) analyses the request and the code, and returns a plan.
   - If the plan contains BLOCKING open questions, ask the user before continuing.
   - For large or architectural tasks, show the user a short summary of the plan before implementation.
2. **`engine-implementor`** (Opus) implements the plan and verifies the build.
3. **`engine-reviewer`** (Opus) reviews the diff and returns APPROVED or CHANGES_REQUESTED.
   - On CHANGES_REQUESTED, send the findings back to `engine-implementor` (continue the same agent through SendMessage when possible), then review again.
   - Allow at most 3 review rounds. If the review is still not approved after 3 rounds, stop and report the remaining findings to the user.

Pass each agent the full context it needs: the user's request, the plan, and the review findings. Agents do not see this conversation. After approval, report to the user in Hungarian: what was done, which files changed, and the build result. Commit only when the user asks.

Trivial non-code edits (docs typos, this file) may be done directly without the pipeline.

## Non-negotiable code rules (details in docs/coding-standards.md)

- No comments in Flawy source code.
- Hungarian notation for all variables, parameters and data members (`m_bRunning`, `s_spLogger`, `dDeltaSeconds`, `oSpecification`).
- Methods and types are PascalCase.
- Clean OOP: single-responsibility classes, interfaces at module boundaries, RAII, pimpl for backend types, no hidden globals.
- The build is warning-free (warnings are errors).
