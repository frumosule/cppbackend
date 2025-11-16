#define _USE_MATH_DEFINES

#include <catch2/catch_test_macros.hpp>

#include "../src/collision_detector.h"

using namespace collision_detector;

class TestingProvider : public collision_detector::ItemGathererProvider {
public:
	TestingProvider() = default;

	TestingProvider(std::vector<Item>&& items, std::vector<Gatherer>&& gatherers)
		:items_(std::move(items)), gatherers_(std::move(gatherers)) {
	}

	size_t ItemsCount() const override
	{
		return items_.size();
	}

	Item GetItem(size_t idx) const override
	{
		return items_.at(idx);
	}

	size_t GatherersCount() const override
	{
		return gatherers_.size();
	}

	Gatherer GetGatherer(size_t idx) const override
	{
		return gatherers_.at(idx);
	}

private:
	std::vector<Item> items_;
	std::vector<Gatherer> gatherers_;
};

struct BadProviders {
	TestingProvider empty_provider;
	TestingProvider no_items_provider{ {}, {{}, {}, {}} };
	TestingProvider no_gatherers_provider{ {{}, {}, {}}, {} };
};

struct GoodProviders {
	TestingProvider one_gatherer_provider{
		{
			{{0,1}, 2.}, 
			{{-2,0}, 1.},
			{{5,0}, 0.2},
			{{11,0}, 0.9}
		}, 
		{
			{{0, 0}, {10, 0}, 1.}
		} 
	};
	TestingProvider one_item_provider{ 
		{
			{{0,0}, 1.}
		}, 
		{
			{{0, 0}, {10, 0}, 1.},
			{{0, 10}, {0, 0}, 1.},
			{{-10, 0}, {0, 0}, 1.}
		} 
	};
};

TEST_CASE_METHOD(BadProviders, "Test bad providers with no events") {
	CHECK(FindGatherEvents(empty_provider).size() == 0);
	CHECK(FindGatherEvents(no_items_provider).size() == 0);
	CHECK(FindGatherEvents(no_gatherers_provider).size() == 0);
}

TEST_CASE_METHOD(GoodProviders, "Test good providers") {
	auto one_gatherer_result = FindGatherEvents(one_gatherer_provider);
	CHECK(one_gatherer_result.size() == 2);
	CHECK(one_gatherer_result[0].gatherer_id == 0);
	CHECK(one_gatherer_result[0].item_id == 0);
	CHECK(one_gatherer_result[1].item_id == 2);
	auto one_item_result = FindGatherEvents(one_item_provider);
	CHECK(one_item_result.size() == 3);
	CHECK(one_item_result[0].gatherer_id == 0);
	CHECK(one_item_result[1].gatherer_id == 1);
	CHECK(one_item_result[2].gatherer_id == 2);
}

// Напишите здесь тесты для функции collision_detector::FindGatherEvents