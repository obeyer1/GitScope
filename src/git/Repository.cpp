#include "git/Repository.h"

#include "git/GitException.h"

#include <algorithm>
#include <utility>

namespace gitscope::git {

namespace {

std::string oidToHex(const git_oid* oid)
{
    char buf[GIT_OID_HEXSZ + 1];
    git_oid_tostr(buf, sizeof(buf), oid);
    return std::string(buf);
}

CommitInfo makeCommitInfo(git_commit* commit)
{
    CommitInfo info;
    info.id = oidToHex(git_commit_id(commit));
    info.shortId = info.id.substr(0, 7);

    const unsigned int parents = git_commit_parentcount(commit);
    info.parentIds.reserve(parents);
    for (unsigned int i = 0; i < parents; ++i)
        info.parentIds.push_back(oidToHex(git_commit_parent_id(commit, i)));

    const char* summary = git_commit_summary(commit);
    info.summary = summary != nullptr ? summary : "";

    const git_signature* author = git_commit_author(commit);
    if (author != nullptr) {
        info.authorName = author->name != nullptr ? author->name : "";
        info.authorEmail = author->email != nullptr ? author->email : "";
        info.authorTime = static_cast<int64_t>(author->when.time);
    }
    return info;
}

char deltaStatusChar(git_delta_t status)
{
    switch (status) {
    case GIT_DELTA_ADDED:
        return 'A';
    case GIT_DELTA_DELETED:
        return 'D';
    case GIT_DELTA_MODIFIED:
        return 'M';
    case GIT_DELTA_RENAMED:
        return 'R';
    case GIT_DELTA_COPIED:
        return 'C';
    case GIT_DELTA_TYPECHANGE:
        return 'T';
    default:
        return '?';
    }
}

} // namespace

LibGit2::LibGit2()
{
    check(git_libgit2_init(), "initializing libgit2");
    // Hardening: never read system-wide or per-user git configuration.
    // Repository-local config is still honored (read-only), but a hostile
    // machine-wide config cannot influence how repositories are parsed.
    for (int level : {GIT_CONFIG_LEVEL_SYSTEM, GIT_CONFIG_LEVEL_GLOBAL, GIT_CONFIG_LEVEL_XDG,
                      GIT_CONFIG_LEVEL_PROGRAMDATA}) {
        check(git_libgit2_opts(GIT_OPT_SET_SEARCH_PATH, level, ""), "isolating git config");
    }
}

LibGit2::~LibGit2()
{
    git_libgit2_shutdown();
}

const char* LibGit2::versionString()
{
    return LIBGIT2_VERSION;
}

Repository::Repository(RepositoryPtr repo) : repo_(std::move(repo)) {}

Repository Repository::open(const std::string& path)
{
    git_repository* raw = nullptr;
    check(git_repository_open_ext(&raw, path.c_str(), 0, nullptr), "opening repository");
    return Repository(RepositoryPtr(raw));
}

RepoSummary Repository::summary() const
{
    RepoSummary s;
    const char* gitDir = git_repository_path(repo_.get());
    s.gitDir = gitDir != nullptr ? gitDir : "";
    const char* workDir = git_repository_workdir(repo_.get());
    s.workDir = workDir != nullptr ? workDir : "";
    s.bare = git_repository_is_bare(repo_.get()) == 1;
    s.detached = git_repository_head_detached(repo_.get()) == 1;

    git_reference* rawHead = nullptr;
    const int err = git_repository_head(&rawHead, repo_.get());
    if (err == 0) {
        ReferencePtr head(rawHead);
        if (s.detached) {
            const git_oid* oid = git_reference_target(head.get());
            s.headName = oid != nullptr ? oidToHex(oid).substr(0, 7) : "detached";
        } else {
            const char* shorthand = git_reference_shorthand(head.get());
            s.headName = shorthand != nullptr ? shorthand : "";
        }
    } else if (err == GIT_EUNBORNBRANCH || err == GIT_ENOTFOUND) {
        s.headName = "(no commits yet)";
    } else {
        check(err, "reading HEAD");
    }
    return s;
}

std::vector<BranchInfo> Repository::branches() const
{
    std::vector<BranchInfo> result;

    git_branch_iterator* rawIter = nullptr;
    check(git_branch_iterator_new(&rawIter, repo_.get(), GIT_BRANCH_ALL), "listing branches");
    BranchIteratorPtr iter(rawIter);

    git_reference* rawRef = nullptr;
    git_branch_t type = GIT_BRANCH_LOCAL;
    int err = 0;
    while ((err = git_branch_next(&rawRef, &type, iter.get())) == 0) {
        ReferencePtr ref(rawRef);

        const char* name = nullptr;
        if (git_branch_name(&name, ref.get()) != 0 || name == nullptr)
            continue;

        BranchInfo info;
        info.shortName = name;
        const char* refName = git_reference_name(ref.get());
        info.refName = refName != nullptr ? refName : "";
        info.isRemote = type == GIT_BRANCH_REMOTE;
        info.isHead = git_branch_is_head(ref.get()) == 1;

        // Symbolic refs (e.g. origin/HEAD) must be resolved to have a target.
        const git_oid* target = git_reference_target(ref.get());
        if (target == nullptr) {
            git_reference* rawResolved = nullptr;
            if (git_reference_resolve(&rawResolved, ref.get()) != 0)
                continue;
            ReferencePtr resolved(rawResolved);
            target = git_reference_target(resolved.get());
            if (target == nullptr)
                continue;
            info.targetId = oidToHex(target);
        } else {
            info.targetId = oidToHex(target);
        }
        result.push_back(std::move(info));
    }
    if (err != GIT_ITEROVER)
        check(err, "iterating branches");

    std::stable_sort(result.begin(), result.end(), [](const BranchInfo& a, const BranchInfo& b) {
        if (a.isRemote != b.isRemote)
            return !a.isRemote;
        return a.shortName < b.shortName;
    });
    return result;
}

std::vector<TagInfo> Repository::tags() const
{
    std::vector<TagInfo> result;

    git_reference_iterator* rawIter = nullptr;
    check(git_reference_iterator_glob_new(&rawIter, repo_.get(), "refs/tags/*"), "listing tags");
    ReferenceIteratorPtr iter(rawIter);

    git_reference* rawRef = nullptr;
    int err = 0;
    while ((err = git_reference_next(&rawRef, iter.get())) == 0) {
        ReferencePtr ref(rawRef);

        const char* refName = git_reference_name(ref.get());
        if (refName == nullptr)
            continue;

        TagInfo info;
        info.refName = refName;
        const char* shorthand = git_reference_shorthand(ref.get());
        info.shortName = shorthand != nullptr ? shorthand : info.refName;

        // Peel annotated tags through to the commit; skip tags that point at
        // trees or blobs since they have no place in the commit graph.
        git_object* rawObj = nullptr;
        if (git_reference_peel(&rawObj, ref.get(), GIT_OBJECT_COMMIT) != 0)
            continue;
        ObjectPtr obj(rawObj);
        info.targetId = oidToHex(git_object_id(obj.get()));
        result.push_back(std::move(info));
    }
    if (err != GIT_ITEROVER)
        check(err, "iterating tags");

    std::sort(result.begin(), result.end(),
              [](const TagInfo& a, const TagInfo& b) { return a.shortName < b.shortName; });
    return result;
}

DecorationMap Repository::decorations() const
{
    DecorationMap map;
    for (const BranchInfo& b : branches()) {
        const char type = b.isHead ? 'H' : (b.isRemote ? 'R' : 'L');
        map[b.targetId].push_back({b.shortName, type});
    }
    for (const TagInfo& t : tags())
        map[t.targetId].push_back({t.shortName, 'T'});

    if (git_repository_head_detached(repo_.get()) == 1) {
        git_reference* rawHead = nullptr;
        if (git_repository_head(&rawHead, repo_.get()) == 0) {
            ReferencePtr head(rawHead);
            const git_oid* oid = git_reference_target(head.get());
            if (oid != nullptr) {
                auto& labels = map[oidToHex(oid)];
                labels.insert(labels.begin(), {"HEAD", 'D'});
            }
        }
    }
    return map;
}

std::vector<CommitInfo> Repository::log(const std::string& refName, std::size_t maxCount) const
{
    git_revwalk* rawWalk = nullptr;
    check(git_revwalk_new(&rawWalk, repo_.get()), "creating revision walker");
    RevwalkPtr walk(rawWalk);
    check(git_revwalk_sorting(walk.get(), GIT_SORT_TOPOLOGICAL | GIT_SORT_TIME),
          "setting walk order");

    if (refName.empty()) {
        check(git_revwalk_push_glob(walk.get(), "refs/heads/*"), "walking local branches");
        check(git_revwalk_push_glob(walk.get(), "refs/remotes/*"), "walking remote branches");
        check(git_revwalk_push_glob(walk.get(), "refs/tags/*"), "walking tags");
        // A detached HEAD is not covered by the globs above; an unborn HEAD
        // (fresh repository) is fine to ignore.
        const int headErr = git_revwalk_push_head(walk.get());
        if (headErr != 0 && headErr != GIT_EUNBORNBRANCH && headErr != GIT_ENOTFOUND)
            check(headErr, "walking HEAD");
    } else {
        check(git_revwalk_push_ref(walk.get(), refName.c_str()), "walking reference");
    }

    std::vector<CommitInfo> commits;
    git_oid oid;
    while (commits.size() < maxCount && git_revwalk_next(&oid, walk.get()) == 0) {
        git_commit* rawCommit = nullptr;
        if (git_commit_lookup(&rawCommit, repo_.get(), &oid) != 0)
            continue; // shallow clones may reference missing commits
        CommitPtr commit(rawCommit);
        commits.push_back(makeCommitInfo(commit.get()));
    }
    return commits;
}

CommitDetails Repository::details(const std::string& commitId) const
{
    git_oid oid;
    check(git_oid_fromstr(&oid, commitId.c_str()), "parsing commit id");

    git_commit* rawCommit = nullptr;
    check(git_commit_lookup(&rawCommit, repo_.get(), &oid), "looking up commit");
    CommitPtr commit(rawCommit);

    CommitDetails result;
    result.info = makeCommitInfo(commit.get());
    const char* message = git_commit_message(commit.get());
    result.fullMessage = message != nullptr ? message : "";
    const git_signature* committer = git_commit_committer(commit.get());
    if (committer != nullptr) {
        result.committerName = committer->name != nullptr ? committer->name : "";
        result.committerEmail = committer->email != nullptr ? committer->email : "";
        result.commitTime = static_cast<int64_t>(committer->when.time);
    }

    git_tree* rawTree = nullptr;
    check(git_commit_tree(&rawTree, commit.get()), "reading commit tree");
    TreePtr tree(rawTree);

    TreePtr parentTree; // stays null for root commits -> diff vs. empty tree
    if (git_commit_parentcount(commit.get()) > 0) {
        git_commit* rawParent = nullptr;
        check(git_commit_parent(&rawParent, commit.get(), 0), "reading first parent");
        CommitPtr parent(rawParent);
        git_tree* rawParentTree = nullptr;
        check(git_commit_tree(&rawParentTree, parent.get()), "reading parent tree");
        parentTree.reset(rawParentTree);
    }

    git_diff_options opts;
    check(git_diff_options_init(&opts, GIT_DIFF_OPTIONS_VERSION), "initializing diff options");
    git_diff* rawDiff = nullptr;
    check(git_diff_tree_to_tree(&rawDiff, repo_.get(), parentTree.get(), tree.get(), &opts),
          "computing diff");
    DiffPtr diff(rawDiff);
    check(git_diff_find_similar(diff.get(), nullptr), "detecting renames");

    const std::size_t deltas = git_diff_num_deltas(diff.get());
    result.files.reserve(deltas);
    for (std::size_t i = 0; i < deltas; ++i) {
        const git_diff_delta* delta = git_diff_get_delta(diff.get(), i);
        if (delta == nullptr)
            continue;

        FileDiff file;
        file.status = deltaStatusChar(delta->status);
        file.path = delta->new_file.path != nullptr ? delta->new_file.path : "";
        file.oldPath = delta->old_file.path != nullptr ? delta->old_file.path : "";
        if (file.path.empty())
            file.path = file.oldPath;
        file.binary = (delta->flags & GIT_DIFF_FLAG_BINARY) != 0;

        git_patch* rawPatch = nullptr;
        if (git_patch_from_diff(&rawPatch, diff.get(), i) == 0 && rawPatch != nullptr) {
            PatchPtr patch(rawPatch);
            std::size_t context = 0;
            std::size_t additions = 0;
            std::size_t deletions = 0;
            if (git_patch_line_stats(&context, &additions, &deletions, patch.get()) == 0) {
                file.additions = static_cast<int>(additions);
                file.deletions = static_cast<int>(deletions);
            }
            if (!file.binary) {
                GitBuf buf;
                if (git_patch_to_buf(buf.ptr(), patch.get()) == 0 && buf.data() != nullptr) {
                    // Cap pathological patches so the UI stays responsive.
                    constexpr std::size_t kMaxPatchBytes = 2 * 1024 * 1024;
                    if (buf.size() > kMaxPatchBytes) {
                        file.patch.assign(buf.data(), kMaxPatchBytes);
                        file.patch += "\n… patch truncated (";
                        file.patch += std::to_string(buf.size());
                        file.patch += " bytes total) …\n";
                    } else {
                        file.patch.assign(buf.data(), buf.size());
                    }
                }
            }
        }
        result.files.push_back(std::move(file));
    }
    return result;
}

} // namespace gitscope::git
