//
// Created by josh on 2/6/23.
//

#pragma once

namespace df::ui {
class Widget {
public:
protected:
    glm::vec2 position, extent;
    std::vector<std::unique_ptr<Widget>> children;

public:
    [[nodiscard]] glm::vec2 getPosition() const noexcept { return position; }
    [[nodiscard]] glm::vec2 getExtent() const noexcept { return extent; }
};

class Panel : public Widget {};

class Label : public Widget {
protected:
    std::string txt;

public:
    std::string& getText() noexcept { return txt; }
};

}   // namespace df::ui