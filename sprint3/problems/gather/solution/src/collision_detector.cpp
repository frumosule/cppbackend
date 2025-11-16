#include "collision_detector.h"
#include <cassert>
#include <vector>
#include <algorithm>
#include <cmath>

namespace collision_detector {

// Эта функция была дана в уроке — её нужно реализовать здесь,
// чтобы линковщик нашёл символ.
CollectionResult TryCollectPoint(geom::Point2D a, geom::Point2D b, geom::Point2D c) {
    // Проверяем, что движение ненулевое
    assert(b.x != a.x || b.y != a.y);

    // Вектор a->c
    double u_x = c.x - a.x;
    double u_y = c.y - a.y;
    // Вектор a->b
    double v_x = b.x - a.x;
    double v_y = b.y - a.y;

    // Скалярное произведение и длины в квадрате
    double u_dot_v = u_x * v_x + u_y * v_y;
    double v_len2  = v_x * v_x + v_y * v_y;
    double u_len2  = u_x * u_x + u_y * u_y;

    // Проекция и квадрат расстояния до прямой
    double proj_ratio  = u_dot_v / v_len2;
    double sq_distance = u_len2 - (u_dot_v * u_dot_v) / v_len2;

    return CollectionResult{ sq_distance, proj_ratio };
}

std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider) {
    std::vector<GatheringEvent> events;

    const size_t gatherersCount = provider.GatherersCount();
    const size_t itemsCount     = provider.ItemsCount();

    for (size_t gid = 0; gid < gatherersCount; ++gid) {
        Gatherer g = provider.GetGatherer(gid);

        // Пропускаем нулевое перемещение
        if (g.start_pos.x == g.end_pos.x && g.start_pos.y == g.end_pos.y) {
            continue;
        }

        for (size_t iid = 0; iid < itemsCount; ++iid) {
            Item item = provider.GetItem(iid);

            // Рассчитываем, насколько близко к траектории
            CollectionResult res = TryCollectPoint(g.start_pos, g.end_pos, item.position);

            // Сумма «радиусов» (половин ширин)
            double radius_sum = g.width + item.width;

            // Если попали в сегмент и расстояние в пределах
            if (res.proj_ratio >= 0.0 && res.proj_ratio <= 1.0 &&
                res.sq_distance <= radius_sum * radius_sum) 
            {
                events.push_back(GatheringEvent{
                    /*item_id*/     iid,
                    /*gatherer_id*/ gid,
                    /*sq_distance*/ res.sq_distance,
                    /*time*/        res.proj_ratio
                });
            }
        }
    }

    // Сортируем по времени, затем по id собира­теля, затем по id предмета
    std::sort(events.begin(), events.end(),
        [](auto const& a, auto const& b) {
            if (a.time != b.time)
                return a.time < b.time;
            if (a.gatherer_id != b.gatherer_id)
                return a.gatherer_id < b.gatherer_id;
            return a.item_id < b.item_id;
        });

    return events;
}

}  // namespace collision_detector