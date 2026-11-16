# Taiga — Agent Instructions

This is an open-source desktop anime tracking application (C++, Qt 6, C++23). It syncs watch progress with AniList, Kitsu, and MyAnimeList.

## The Lauren Method comes first

Before any code change, apply the trust-pyramid-workflow: deterministic guardrails over prose, verify before ship. Fix the codebase first, then automate. Never scale slop.

## C/C++ Linting with clang-tidy

Automatically run the `clang-tidy-review` skill whenever you finish writing or modifying C/C++ files before reporting completion.

Never run `clang-tidy` on the entire repo; only lint modified C/C++ files from `git diff --name-only HEAD`.

Always verify `build\compile_commands.json` exists first. If missing on Windows, generate it using Ninja (`cmake -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`), because standard Visual Studio MSBuild generators do not export `compile_commands.json`.

Strictly obey the repo's `.clang-tidy` config; never override with `-checks=*`.

Never run `clang-tidy --fix` blindly. Review warnings first, apply clean targeted fixes, and verify the project compiles and passes tests afterward.

Redirect long `clang-tidy` outputs to a temporary log file and summarize the top warnings instead of dumping raw logs into context.
