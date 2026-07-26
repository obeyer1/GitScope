#pragma once

#include <git2.h>

#include <cstddef>
#include <memory>

// Owning smart-pointer aliases for every libgit2 object GitScope touches.
// Never hold a raw owning pointer outside this file.

namespace gitscope::git {

namespace detail {

struct RepositoryDeleter {
    void operator()(git_repository* p) const { git_repository_free(p); }
};
struct ReferenceDeleter {
    void operator()(git_reference* p) const { git_reference_free(p); }
};
struct CommitDeleter {
    void operator()(git_commit* p) const { git_commit_free(p); }
};
struct TreeDeleter {
    void operator()(git_tree* p) const { git_tree_free(p); }
};
struct DiffDeleter {
    void operator()(git_diff* p) const { git_diff_free(p); }
};
struct PatchDeleter {
    void operator()(git_patch* p) const { git_patch_free(p); }
};
struct RevwalkDeleter {
    void operator()(git_revwalk* p) const { git_revwalk_free(p); }
};
struct BranchIteratorDeleter {
    void operator()(git_branch_iterator* p) const { git_branch_iterator_free(p); }
};
struct ReferenceIteratorDeleter {
    void operator()(git_reference_iterator* p) const { git_reference_iterator_free(p); }
};
struct ObjectDeleter {
    void operator()(git_object* p) const { git_object_free(p); }
};

} // namespace detail

using RepositoryPtr = std::unique_ptr<git_repository, detail::RepositoryDeleter>;
using ReferencePtr = std::unique_ptr<git_reference, detail::ReferenceDeleter>;
using CommitPtr = std::unique_ptr<git_commit, detail::CommitDeleter>;
using TreePtr = std::unique_ptr<git_tree, detail::TreeDeleter>;
using DiffPtr = std::unique_ptr<git_diff, detail::DiffDeleter>;
using PatchPtr = std::unique_ptr<git_patch, detail::PatchDeleter>;
using RevwalkPtr = std::unique_ptr<git_revwalk, detail::RevwalkDeleter>;
using BranchIteratorPtr = std::unique_ptr<git_branch_iterator, detail::BranchIteratorDeleter>;
using ReferenceIteratorPtr =
    std::unique_ptr<git_reference_iterator, detail::ReferenceIteratorDeleter>;
using ObjectPtr = std::unique_ptr<git_object, detail::ObjectDeleter>;

// git_buf is a value type owning heap memory; this wraps its dispose call.
class GitBuf {
public:
    GitBuf() = default;
    ~GitBuf() { git_buf_dispose(&buf_); }
    GitBuf(const GitBuf&) = delete;
    GitBuf& operator=(const GitBuf&) = delete;

    git_buf* ptr() { return &buf_; }
    const char* data() const { return buf_.ptr; }
    std::size_t size() const { return buf_.size; }

private:
    git_buf buf_ = GIT_BUF_INIT;
};

} // namespace gitscope::git
