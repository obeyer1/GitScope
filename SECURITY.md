# Security Policy

## Threat model

GitScope's core promise is that it is **safe to open any local repository,
including untrusted ones**. The application:

- performs **no mutating Git operations** — it never writes to the object
  database, references, index, working tree, or configuration;
- performs **no network I/O** of any kind;
- **never spawns subprocesses** (it does not shell out to `git`);
- **never runs Git hooks** (hooks only fire on mutating operations);
- **isolates configuration**: at startup the system, global, XDG, and
  ProgramData git config search paths are cleared via
  `git_libgit2_opts(GIT_OPT_SET_SEARCH_PATH, …)`, so machine-wide config can
  not alter how repositories are parsed. Repository-local config is honored
  read-only;
- keeps libgit2's **repository ownership validation** (the `safe.directory`
  equivalent) enabled.

Content from repositories (commit messages, author names, file paths, diff
text) is always treated as untrusted data and rendered as plain text — it is
never interpreted as markup, executed, or used to build shell commands.

### Out of scope

- Denial of service via pathologically large repositories (the UI may become
  slow; history and patch sizes are capped, but crafted inputs may still be
  expensive to render).
- Vulnerabilities in dependencies (Qt, libgit2). Please report those
  upstream; we track and adopt fixed versions.

## Supported versions

Only the latest release receives security fixes.

## Reporting a vulnerability

Please use **GitHub → Security → Report a vulnerability** (private advisory)
on this repository. Do not open a public issue for security reports.

You can expect an initial response within 7 days. Please include a minimal
reproduction (ideally a small repository) when possible.
