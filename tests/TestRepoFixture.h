#pragma once

// Test-only helpers that CREATE a throwaway repository with libgit2 write
// APIs, so the read-only production code has something real to inspect.
// Production code (src/) never calls any of these mutating functions —
// see CONTRIBUTING.md for the read-only rule.

#include "TestAssert.h"

#include <git2.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#define REQUIRE_OK(expr)                                                                           \
    do {                                                                                           \
        const int rc_ = (expr);                                                                    \
        if (rc_ < 0) {                                                                             \
            const git_error* err_ = git_error_last();                                              \
            std::fprintf(stderr, "FAILED: %s -> %d (%s) at %s:%d\n", #expr, rc_,                   \
                         err_ != nullptr && err_->message != nullptr ? err_->message : "?",        \
                         __FILE__, __LINE__);                                                      \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (0)

// Writes `content` to `fileName` in the work tree, stages it, and commits.
// parentHexes lists parent commit ids (empty for a root commit). Returns the
// new commit id as hex. updateRef is e.g. "HEAD" or "refs/heads/feature".
inline std::string writeCommit(git_repository* repo, const char* updateRef,
                               const std::string& fileName, const std::string& content,
                               const std::string& message,
                               const std::vector<std::string>& parentHexes, git_time_t when)
{
    namespace fs = std::filesystem;
    const char* workdir = git_repository_workdir(repo);
    REQUIRE(workdir != nullptr);
    {
        std::ofstream out(fs::path(workdir) / fileName, std::ios::trunc | std::ios::binary);
        REQUIRE(out.good());
        out << content;
    }

    git_index* index = nullptr;
    REQUIRE_OK(git_repository_index(&index, repo));
    REQUIRE_OK(git_index_add_bypath(index, fileName.c_str()));
    REQUIRE_OK(git_index_write(index));
    git_oid treeId;
    REQUIRE_OK(git_index_write_tree(&treeId, index));
    git_index_free(index);

    git_tree* tree = nullptr;
    REQUIRE_OK(git_tree_lookup(&tree, repo, &treeId));
    git_signature* sig = nullptr;
    REQUIRE_OK(git_signature_new(&sig, "Test Author", "author@example.com", when, 0));

    std::vector<git_commit*> parents;
    for (const std::string& hex : parentHexes) {
        git_oid parentId;
        REQUIRE_OK(git_oid_fromstr(&parentId, hex.c_str()));
        git_commit* parent = nullptr;
        REQUIRE_OK(git_commit_lookup(&parent, repo, &parentId));
        parents.push_back(parent);
    }

    git_oid commitId;
    REQUIRE_OK(git_commit_create(&commitId, repo, updateRef, sig, sig, nullptr, message.c_str(),
                                 tree, parents.size(),
                                 const_cast<const git_commit**>(parents.data())));

    for (git_commit* parent : parents)
        git_commit_free(parent);
    git_signature_free(sig);
    git_tree_free(tree);

    char buf[GIT_OID_HEXSZ + 1];
    git_oid_tostr(buf, sizeof(buf), &commitId);
    return buf;
}
