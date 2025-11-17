#include "collision_detector.h"

#include <cassert>
#include <algorithm>
#include <cmath>

namespace collision_detector {

CollectionResult TryCollectPoint(geom::Point2D a, geom::Point2D b, geom::Point2D c) {
    assert(b.x != a.x || b.y != a.y);
    const double u_x = c.x - a.x;
    const double u_y = c.y - a.y;
    const double v_x = b.x - a.x;
    const double v_y = b.y - a.y;
    const double u_dot_v = u_x * v_x + u_y * v_y;
    const double u_len2 = u_x * u_x + u_y * u_y;
    const double v_len2 = v_x * v_x + v_y * v_y;
    const double proj_ratio = u_dot_v / v_len2;
    const double sq_distance = u_len2 - (u_dot_v * u_dot_v) / v_len2;

    return CollectionResult(sq_distance, proj_ratio);
}

std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider) {
    std::vector<GatheringEvent> events;
    
    for (size_t gatherer_id = 0; gatherer_id < provider.GatherersCount(); ++gatherer_id) {
        auto gatherer = provider.GetGatherer(gatherer_id);
        
        if (gatherer.start_pos.x == gatherer.end_pos.x && 
            gatherer.start_pos.y == gatherer.end_pos.y) {
            continue;
        }
        
        for (size_t item_id = 0; item_id < provider.ItemsCount(); ++item_id) {
            auto item = provider.GetItem(item_id);
            
            auto result = TryCollectPoint(gatherer.start_pos, gatherer.end_pos, item.position);
            
            double gatherer_radius = gatherer.width / 2.0;
            double item_radius = item.width / 2.0;
            double total_radius = gatherer_radius + item_radius;
            
            if (result.IsCollected(total_radius)) {
                events.emplace_back(item_id, gatherer_id, result.sq_distance, result.proj_ratio);
            }
        }
    }
    
    std::sort(events.begin(), events.end(), 
              [](const GatheringEvent& a, const GatheringEvent& b) {
                  if (a.time != b.time) {
                      return a.time < b.time;
                  }
                  if (a.gatherer_id != b.gatherer_id) {
                      return a.gatherer_id < b.gatherer_id;
                  }
                  return a.item_id < b.item_id;
              });
    
    return events;
}

}  