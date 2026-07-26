# Contributing to GitScope

Thanks for your interest in improving GitScope!

## Ground rules

1. **GitScope is read-only, local-only, forever.** Pull requests must not
   introduce:
   - mutating libgit2 calls in `src/` (anything that writes objects, refs,
     the index, the working tree, or config — e.g. `git_*_create`,
     `git_checkout_*`, `git_reset_*`, `git_index_add*`, `git_config_set_*`);
     the test fixture in `tests/` is the only place allowed to write, and
     only to throwaway temp repositories;
   - network access (no fetch/push/clone, no Qt Network);
   - subprocess execution.
2. **Keep the core Qt-free.** Everything under `src/git/` must compile
   without Qt so it stays testable headless. UI code lives in `src/ui/`.
3. **Add tests** for changes to `src/git/` (graph layout, repository
   access). UI-only changes don't require tests but should be exercised
   manually.

## Building

See the [README](README.md#building-from-source). The short version:

```bash
cmake --preset debug          # add -DCMAKE_PREFIX_PATH="$(brew --prefix)" on macOS
cmake --build --preset debug
ctest --preset debug
```

## Style

- C++20, 4-space indent, 100-column limit — enforced by `.clang-format`
  (`clang-format -i` before committing).
- RAII everywhere: never store a raw owning libgit2 pointer; add a deleter to
  `src/git/RaiiHandles.h` if you wrap a new object type.
- Errors from libgit2 are converted to `GitException` via `check()` at the
  call site; UI code catches at the action boundary and shows a message box.

## Pull requests

- Keep PRs focused; separate refactors from behavior changes.
- Describe **what** and **why** in the PR body; link related issues.
- CI (Linux + macOS build and tests) must pass.

## Reporting bugs

Open a GitHub issue with your OS, Qt/libgit2 versions, and reproduction
steps. For anything security-sensitive, see [SECURITY.md](SECURITY.md).
