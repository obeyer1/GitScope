#pragma once

#include "git/RaiiHandles.h"
#include "git/Types.h"

#include <cstddef>
#include <string>
#include <vector>

namespace gitscope::git {

// Process-wide libgit2 lifetime + hardening. Instantiate exactly once, before
// any Repository is opened, and keep it alive for the life of the process.
class LibGit2 {
public:
    LibGit2();
    ~LibGit2();
    LibGit2(const LibGit2&) = delete;
    LibGit2& operator=(const LibGit2&) = delete;

    static const char* versionString();
};

// Read-only handle to a local repository. Only inspection APIs are ever
// called: GitScope never mutates the repository, its index, its working
// tree, or any configuration. All methods throw GitException on failure.
class Repository {
public:
    // Opens the repository containing `path` (discovers the root upward).
    static Repository open(const std::string& path);

    RepoSummary summary() const;
    std::vector<BranchInfo> branches() const; // locals first, then remotes
    std::vector<TagInfo> tags() const;
    DecorationMap decorations() const;

    // Walks history in topological order (children before parents), which is
    // the order GraphBuilder requires. Empty refName = all branches, tags,
    // and HEAD. Returns at most maxCount commits.
    std::vector<CommitInfo> log(const std::string& refName, std::size_t maxCount) const;

    // Full metadata and per-file diffs (vs. first parent; roots diff against
    // the empty tree). Renames/copies are detected.
    CommitDetails details(const std::string& commitId) const;

private:
    explicit Repository(RepositoryPtr repo);

    RepositoryPtr repo_;
};

} // namespace gitscope::git
