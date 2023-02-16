//
// Created by josh on 2/15/23.
//
#include <asset.h>
#include <catch2/catch_test_macros.hpp>
using namespace df;

class TestAsset : public Asset {
public:
    TestAsset() { name = "test asset"; }
};

TEST_CASE("Asset Manager")
{
    auto registry = AssetRegistry();
    SECTION("Insert and Get")
    {
        REQUIRE_NOTHROW(registry.insert(new TestAsset()));
        REQUIRE(registry.get<TestAsset>("test asset")->getName() == "test asset");
    }
    registry.destroy();
}