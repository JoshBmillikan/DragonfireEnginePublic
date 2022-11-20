//
// Created by josh on 11/19/22.
//

#include "material.h"
#include <file.h>
#include <nlohmann/json.hpp>
#include "pipeline.h"

namespace df {

std::vector<Asset*> Material::Loader::load(const char* filename)
{
    File file(filename);
    nlohmann::json json = file.readToString();
    file.close();



    return {};
}
}   // namespace df