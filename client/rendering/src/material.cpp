//
// Created by josh on 11/19/22.
//

#include "material.h"

namespace df {

Material::~Material()
{
    device.destroy(pipeline);
    device.destroy(layout);
}
}   // namespace df