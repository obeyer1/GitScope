#include "TestAssert.h"
#include "TestRepoFixture.h"

#include "git/GraphBuilder.h"
#include "git/Repository.h"

#include <cstdio>
#include <filesystem>
#include <string>

using namespace gitscope::git;
namespace fs = std::filesystem;

namespace {

std::size_t indexOf(const std::vector<CommitInfo>& commits, const std::string& id)
{
    for (std::size_t i = 0; i < commits.size(); ++i)
        if (commits[i].id == id)
            return i;
    REQUIRE(false && "commit not found");
    return 0;
}

} // namespace

int main()
{
    // Same init + hardening path as the application; clearing the global
    // config also makes the default branch name deterministic ("master").
    LibGit2 lib;

    const fs::path dir = fs::temp_directory_path() / "gitscope-test-repo";
    std::error_code ec;
    fs::remove_all(dir, ec);

    // ---- Build the fixture (write APIs live only in tests) ----------------
    git_repository* raw = nullptr;
    REQUIRE_OK(git_repository_init(&raw, dir.string().c_str(), 0));

    git_time_t t = 1700000000;
    const std::string c1 = writeCommit(raw, "HEAD", "a.txt", "one\n", "initial commit", {}, t += 60);
    const std::string c2 =
        writeCommit(raw, "HEAD", "a.txt", "one\ntwo\n", "add second line", {c1}, t += 60);

    {
        git_oid oid;
        REQUIRE_OK(git_oid_fromstr(&oid, c1.c_str()));
        git_commit* base = nullptr;
        REQUIRE_OK(git_commit_lookup(&base, raw, &oid));
        git_reference* branch = nullptr;
        REQUIRE_OK(git_branch_create(&branch, raw, "feature", base, 0));
        git_reference_free(branch);
        git_commit_free(base);
    }
    const std::string c3 =
        writeCommit(raw, "refs/heads/feature", "b.txt", "feature\n", "feature work", {c1}, t += 60);
    const std::string m = writeCommit(raw, "HEAD", "a.txt", "one\ntwo\n",
                                      "merge feature into master", {c2, c3}, t += 60);

    {
        git_oid oid;
        REQUIRE_OK(git_oid_fromstr(&oid, c2.c_str()));
        git_object* obj = nullptr;
        REQUIRE_OK(git_object_lookup(&obj, raw, &oid, GIT_OBJECT_COMMIT));
        git_oid tagId;
        REQUIRE_OK(git_tag_create_lightweight(&tagId, raw, "v0.1", obj, 0));
        git_object_free(obj);
    }
    git_repository_free(raw);

    // ---- Exercise the production (read-only) code -------------------------
    fs::create_directories(dir / "nested" / "deep");
    Repository repo = Repository::open((dir / "nested" / "deep").string()); // discovers upward

    const RepoSummary summary = repo.summary();
    REQUIRE(!summary.bare);
    REQUIRE(!summary.detached);
    REQUIRE(summary.headName == "master");
    REQUIRE(!summary.workDir.empty());

    const auto branches = repo.branches();
    REQUIRE(branches.size() == 2);
    REQUIRE(branches[0].shortName == "feature");
    REQUIRE(!branches[0].isHead);
    REQUIRE(branches[0].targetId == c3);
    REQUIRE(branches[1].shortName == "master");
    REQUIRE(branches[1].isHead);
    REQUIRE(branches[1].targetId == m);

    const auto tags = repo.tags();
    REQUIRE(tags.size() == 1);
    REQUIRE(tags[0].shortName == "v0.1");
    REQUIRE(tags[0].targetId == c2);

    // Full history across all refs, topologically ordered.
    const auto commits = repo.log("", 100);
    REQUIRE(commits.size() == 4);
    REQUIRE(commits[0].id == m);
    REQUIRE(commits[0].parentIds.size() == 2);
    REQUIRE(commits[0].summary == "merge feature into master");
    REQUIRE(commits[0].authorName == "Test Author");
    REQUIRE(commits.back().id == c1);
    for (const CommitInfo& commit : commits)
        for (const std::string& parent : commit.parentIds)
            REQUIRE(indexOf(commits, commit.id) < indexOf(commits, parent));

    // Scoped walks.
    REQUIRE(repo.log("refs/heads/master", 100).size() == 4); // merge pulls in feature side
    REQUIRE(repo.log("refs/heads/feature", 100).size() == 2);
    REQUIRE(repo.log("", 2).size() == 2); // maxCount honored

    // Decorations: HEAD branch, side branch, tag.
    const DecorationMap decorations = repo.decorations();
    REQUIRE(decorations.count(m) == 1);
    REQUIRE(decorations.at(m).front().label == "master");
    REQUIRE(decorations.at(m).front().type == 'H');
    REQUIRE(decorations.count(c3) == 1);
    REQUIRE(decorations.at(c3).front().type == 'L');
    REQUIRE(decorations.count(c2) == 1);
    REQUIRE(decorations.at(c2).front().label == "v0.1");
    REQUIRE(decorations.at(c2).front().type == 'T');

    // Diff of an ordinary commit: one modified file with one added line.
    const CommitDetails d2 = repo.details(c2);
    REQUIRE(d2.files.size() == 1);
    REQUIRE(d2.files[0].path == "a.txt");
    REQUIRE(d2.files[0].status == 'M');
    REQUIRE(d2.files[0].additions == 1);
    REQUIRE(d2.files[0].deletions == 0);
    REQUIRE(d2.files[0].patch.find("+two") != std::string::npos);
    REQUIRE(d2.fullMessage.rfind("add second line", 0) == 0);
    REQUIRE(d2.committerName == "Test Author");

    // Root commit diffs against the empty tree.
    const CommitDetails d1 = repo.details(c1);
    REQUIRE(d1.files.size() == 1);
    REQUIRE(d1.files[0].status == 'A');
    REQUIRE(d1.files[0].patch.find("+one") != std::string::npos);

    // Graph over the real history: the merge row fans out into two parents.
    const auto rows = GraphBuilder::build(commits);
    REQUIRE(rows.size() == commits.size());
    int outgoing = 0;
    for (const GraphSegment& seg : rows[0].segments)
        if (seg.topLane == GraphBuilder::kNode)
            ++outgoing;
    REQUIRE(outgoing == 2);

    fs::remove_all(dir, ec);
    std::puts("test_repository: all assertions passed");
    return 0;
}
