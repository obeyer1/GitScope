#pragma once

#include "git/Types.h"

#include <vector>

namespace gitscope::git {

// One drawable line inside a commit row. Lane indices are graph columns;
// GraphBuilder::kNode as an endpoint means the row's own commit node.
struct GraphSegment {
    int topLane = 0;    // kNode -> segment starts at the commit node
    int bottomLane = 0; // kNode -> segment ends at the commit node
    int colorIndex = 0;
};

struct GraphRow {
    int commitLane = 0;
    int colorIndex = 0;
    std::vector<GraphSegment> segments;
    int laneCount = 1; // number of lane columns this row spans
};

// Assigns graph lanes to commits that are already in topological order
// (every commit appears before its parents), which is what Repository::log
// produces. Pure function: no libgit2, no Qt — unit tested in isolation.
class GraphBuilder {
public:
    static constexpr int kNode = -1;

    static std::vector<GraphRow> build(const std::vector<CommitInfo>& commits);
};

} // namespace gitscope::git
