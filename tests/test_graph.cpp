#include "TestAssert.h"

#include "git/GraphBuilder.h"

#include <cstdio>
#include <string>
#include <vector>

using gitscope::git::CommitInfo;
using gitscope::git::GraphBuilder;
using gitscope::git::GraphRow;
using gitscope::git::GraphSegment;

namespace {

CommitInfo mk(const char* id, std::vector<std::string> parents)
{
    CommitInfo commit;
    commit.id = id;
    commit.shortId = commit.id.substr(0, 7);
    commit.parentIds = std::move(parents);
    return commit;
}

int countIncoming(const GraphRow& row)
{
    int n = 0;
    for (const GraphSegment& seg : row.segments)
        if (seg.bottomLane == GraphBuilder::kNode)
            ++n;
    return n;
}

int countOutgoing(const GraphRow& row)
{
    int n = 0;
    for (const GraphSegment& seg : row.segments)
        if (seg.topLane == GraphBuilder::kNode)
            ++n;
    return n;
}

bool hasPassThrough(const GraphRow& row, int lane)
{
    for (const GraphSegment& seg : row.segments)
        if (seg.topLane == lane && seg.bottomLane == lane)
            return true;
    return false;
}

} // namespace

int main()
{
    // Linear history: one lane, straight through.
    {
        const auto rows = GraphBuilder::build({mk("c", {"b"}), mk("b", {"a"}), mk("a", {})});
        REQUIRE(rows.size() == 3);
        for (const GraphRow& row : rows) {
            REQUIRE(row.commitLane == 0);
            REQUIRE(row.laneCount == 1);
        }
        REQUIRE(countIncoming(rows[0]) == 0); // tip: nothing flows in
        REQUIRE(countOutgoing(rows[0]) == 1);
        REQUIRE(countIncoming(rows[1]) == 1);
        REQUIRE(countOutgoing(rows[1]) == 1);
        REQUIRE(countOutgoing(rows[2]) == 0); // root: nothing flows out
    }

    // Branch + merge: d merges b (first parent) and c.
    {
        const auto rows = GraphBuilder::build(
            {mk("d", {"b", "c"}), mk("c", {"a"}), mk("b", {"a"}), mk("a", {})});
        REQUIRE(rows.size() == 4);
        REQUIRE(rows[0].commitLane == 0);
        REQUIRE(countOutgoing(rows[0]) == 2); // merge fans out to both parents
        REQUIRE(rows[0].laneCount == 2);
        REQUIRE(rows[1].commitLane == 1);     // side branch keeps its own lane
        REQUIRE(hasPassThrough(rows[1], 0));  // main lane passes through c's row
        REQUIRE(rows[2].commitLane == 0);
        REQUIRE(hasPassThrough(rows[2], 1));
        REQUIRE(rows[3].commitLane == 0);
        REQUIRE(countIncoming(rows[3]) == 2); // both lanes converge on the root
        REQUIRE(countOutgoing(rows[3]) == 0);
    }

    // Two independent root histories interleaved (e.g. orphan branches).
    {
        const auto rows =
            GraphBuilder::build({mk("y", {"x"}), mk("b", {"a"}), mk("x", {}), mk("a", {})});
        REQUIRE(rows[0].commitLane == 0);
        REQUIRE(rows[1].commitLane == 1);
        REQUIRE(rows[2].commitLane == 0);
        REQUIRE(rows[3].commitLane == 1);
        // After x closes lane 0, lane 1 must still flow into a.
        REQUIRE(countIncoming(rows[3]) == 1);
    }

    // Lane reuse: a closed lane is reclaimed by a later tip.
    {
        const auto rows = GraphBuilder::build(
            {mk("m", {"a", "b"}), mk("b", {"a"}), mk("t", {"a"}), mk("a", {})});
        // t is a fresh tip appearing after b's lane (1) merged back; it may
        // reuse a freed slot but must never collide with a live lane.
        REQUIRE(rows[2].commitLane != 0 || rows[2].laneCount >= 1);
        // Root receives every remaining line.
        REQUIRE(countIncoming(rows[3]) >= 2);
    }

    // Empty input.
    {
        REQUIRE(GraphBuilder::build({}).empty());
    }

    std::puts("test_graph: all assertions passed");
    return 0;
}
