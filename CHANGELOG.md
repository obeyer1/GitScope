# Changelog

All notable changes to GitScope are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and GitScope adheres
to [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Fixed

- The commit-graph column is now capped at 20 visible lanes. Repositories
  with many concurrent branches previously produced a graph column thousands
  of pixels wide that pushed all other history columns out of view.
- Long unbreakable commit-message lines no longer force the main window to
  grow wider than the screen: the message body wraps (`white-space:
  pre-wrap`) and the details header no longer imposes a content-derived
  minimum width.

## [0.1.0] - 2026-07-27

### Added

- Initial release.
- Read-only repository browsing via libgit2: branches, remotes, tags,
  commit history with topological ordering.
- Colored commit-graph lane rendering with merge/branch visualization.
- Ref decoration chips (HEAD, local/remote branches, tags, detached HEAD).
- Per-file unified diff viewer with rename detection, add/remove
  highlighting, and per-file stats.
- History filtering by message, author, and hash.
- Repository discovery from any subdirectory; drag & drop to open; optional
  path argument on the command line.
- Security hardening: git config search-path isolation, no network, no
  subprocesses, no hooks.
- Headless unit tests for the core library; CI for Linux and macOS.
