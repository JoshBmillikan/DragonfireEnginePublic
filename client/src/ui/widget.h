//
// Created by josh on 2/6/23.
//

#pragma once

namespace df::ui {
class Widget {
public:
protected:
    glm::vec2 position;

public:
    [[nodiscard]] glm::vec2 getPosition() const noexcept { return position; }
};

class Label : public Widget {
protected:
    std::string txt;

public:
    std::string& getText() noexcept { return txt; }
};


}   // namespace df::ui