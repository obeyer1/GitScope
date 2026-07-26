#include "git/GraphBuilder.h"

#include <algorithm>

namespace gitscope::git {

namespace {

// Lanes hold the commit id they are waiting to reach; "" marks a free slot.
// Free slots are reused (not compacted) so lane indices stay stable from one
// row to the next, which keeps the rendered lines vertical.

int takeFreeLane(std::vector<std::string>& lanes, std::vector<int>& laneColors, int& nextColor,
                 const std::string& id)
{
    for (std::size_t i = 0; i < lanes.size(); ++i) {
        if (lanes[i].empty()) {
            lanes[i] = id;
            laneColors[i] = nextColor++;
            return static_cast<int>(i);
        }
    }
    lanes.push_back(id);
    laneColors.push_back(nextColor++);
    return static_cast<int>(lanes.size()) - 1;
}

int findLane(const std::vector<std::string>& lanes, const std::string& id, int skip)
{
    for (std::size_t i = 0; i < lanes.size(); ++i) {
        if (static_cast<int>(i) != skip && lanes[i] == id)
            return static_cast<int>(i);
    }
    return -1;
}

} // namespace

std::vector<GraphRow> GraphBuilder::build(const std::vector<CommitInfo>& commits)
{
    std::vector<GraphRow> rows;
    rows.reserve(commits.size());

    std::vector<std::string> lanes;
    std::vector<int> laneColors;
    int nextColor = 0;

    for (const CommitInfo& commit : commits) {
        GraphRow row;

        // Lanes whose expected commit is this one flow into the node.
        std::vector<int> incoming;
        for (std::size_t i = 0; i < lanes.size(); ++i) {
            if (lanes[i] == commit.id)
                incoming.push_back(static_cast<int>(i));
        }

        int lane = 0;
        if (incoming.empty())
            lane = takeFreeLane(lanes, laneColors, nextColor, commit.id); // branch tip
        else
            lane = incoming.front();
        row.commitLane = lane;
        row.colorIndex = laneColors[lane];

        for (int in : incoming)
            row.segments.push_back({in, kNode, laneColors[in]});

        // Occupied lanes not touching this node continue straight through.
        for (std::size_t i = 0; i < lanes.size(); ++i) {
            const int laneIdx = static_cast<int>(i);
            if (laneIdx == lane || lanes[i].empty())
                continue;
            if (std::find(incoming.begin(), incoming.end(), laneIdx) != incoming.end())
                continue;
            row.segments.push_back({laneIdx, laneIdx, laneColors[i]});
        }

        // Merged-away lanes close here; the node's lane continues as the
        // first parent. Remaining parents join an existing lane that already
        // expects them, or open a new one.
        for (int in : incoming) {
            if (in != lane)
                lanes[in].clear();
        }
        if (commit.parentIds.empty()) {
            lanes[lane].clear();
        } else {
            lanes[lane] = commit.parentIds.front();
            row.segments.push_back({kNode, lane, laneColors[lane]});
            for (std::size_t p = 1; p < commit.parentIds.size(); ++p) {
                const std::string& parentId = commit.parentIds[p];
                int parentLane = findLane(lanes, parentId, lane);
                if (parentLane < 0)
                    parentLane = takeFreeLane(lanes, laneColors, nextColor, parentId);
                row.segments.push_back({kNode, parentLane, laneColors[parentLane]});
            }
        }

        // Drop trailing free lanes so rows report a tight lane count.
        while (!lanes.empty() && lanes.back().empty()) {
            lanes.pop_back();
            laneColors.pop_back();
        }

        int maxLane = row.commitLane;
        for (const GraphSegment& seg : row.segments)
            maxLane = std::max({maxLane, seg.topLane, seg.bottomLane});
        row.laneCount = maxLane + 1;

        rows.push_back(std::move(row));
    }
    return rows;
}

} // namespace gitscope::git
