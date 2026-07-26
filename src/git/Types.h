#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace gitscope::git {

struct CommitInfo {
    std::string id;                     // full 40-char hex
    std::string shortId;                // 7-char abbreviation for display
    std::vector<std::string> parentIds; // full hex, first parent first
    std::string summary;
    std::string authorName;
    std::string authorEmail;
    int64_t authorTime = 0; // seconds since epoch (UTC)
};

struct BranchInfo {
    std::string shortName; // e.g. "main" or "origin/main"
    std::string refName;   // e.g. "refs/heads/main"
    std::string targetId;  // commit the branch points at (hex)
    bool isRemote = false;
    bool isHead = false;
};

struct TagInfo {
    std::string shortName;
    std::string refName;
    std::string targetId; // peeled commit id (hex)
};

// Ref label attached to a commit row. type: 'H' checked-out branch,
// 'L' local branch, 'R' remote branch, 'T' tag, 'D' detached HEAD.
struct RefDecoration {
    std::string label;
    char type = 'L';
};
using DecorationMap = std::map<std::string, std::vector<RefDecoration>>;

struct FileDiff {
    std::string path;    // new path
    std::string oldPath; // differs from `path` for renames/copies
    char status = 'M';   // A/M/D/R/C/T
    bool binary = false;
    int additions = 0;
    int deletions = 0;
    std::string patch; // unified diff text (empty for binary files)
};

struct CommitDetails {
    CommitInfo info;
    std::string fullMessage;
    std::string committerName;
    std::string committerEmail;
    int64_t commitTime = 0;
    std::vector<FileDiff> files; // vs. first parent (empty tree for roots)
};

struct RepoSummary {
    std::string gitDir;
    std::string workDir;  // empty for bare repositories
    std::string headName; // branch short name, or short hash when detached
    bool detached = false;
    bool bare = false;
};

} // namespace gitscope::git
