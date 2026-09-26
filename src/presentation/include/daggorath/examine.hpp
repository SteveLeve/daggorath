// PEXAM.ASM EXAMIN projection. Logical character cells, not pixels.
#pragma once
#include <string>
#include <vector>

namespace dag {

struct ExamineSnapshot {
    bool creature = false;
    std::vector<std::string> floor;
    std::vector<std::string> bag;
    int torch_index = -1;  // inverse name in the bag, or -1
};

struct ExamineProjection {
    std::string text;
};

ExamineProjection project_examine(const ExamineSnapshot& snap);

}  // namespace dag
