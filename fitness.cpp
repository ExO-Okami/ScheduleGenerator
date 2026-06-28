#include "fitness.h"
#include <algorithm>

int calculateFitnessValue(const Chromosome& chromosome,
                          const std::vector<Event>& events,
                          const std::vector<Group>& groups,
                          const std::vector<Teacher>& teachers,
                          const std::vector<Room>& rooms,
                          const ConflictIndex& conflictIndex,
                          FitnessDetails* details)
{
    (void)events;
    (void)groups;
    (void)teachers;

    FitnessDetails d{};

    const int PENALTY_GROUP_GAP = 50;
    const int PENALTY_BUILDING_TRANSFER = 100;

    // === ТОЛЬКО МЯГКИЕ ШТРАФЫ ===

    // "Окна" у групп
    for (size_t gId = 0; gId < conflictIndex.groupGenes.size(); ++gId) {
        const auto& geneIdxs = conflictIndex.groupGenes[gId];
        if (geneIdxs.size() < 2) continue;

        std::vector<Slot> slots;
        slots.reserve(geneIdxs.size());
        for (int idx : geneIdxs) slots.push_back(chromosome[idx].slot);
        std::sort(slots.begin(), slots.end());

        for (size_t i = 1; i < slots.size(); ++i) {
            if (slots[i].day == slots[i-1].day) {
                int gap = slots[i].pairNumber - slots[i-1].pairNumber - 1;
                if (gap > 0) {
                    d.groupGaps += gap;
                    d.softPenalty += gap * PENALTY_GROUP_GAP;
                }
            }
        }
    }

    // Переезды между корпусами (если две пары подряд в разных корпусах)
    for (size_t gId = 0; gId < conflictIndex.groupGenes.size(); ++gId) {
        const auto& geneIdxs = conflictIndex.groupGenes[gId];
        if (geneIdxs.size() < 2) continue;

        // Сортируем по времени
        std::vector<std::pair<Slot, int>> slotRoom;
        for (int idx : geneIdxs) {
            slotRoom.push_back({chromosome[idx].slot, chromosome[idx].roomId});
        }
        std::sort(slotRoom.begin(), slotRoom.end());

        for (size_t i = 1; i < slotRoom.size(); ++i) {
            if (slotRoom[i].first.day == slotRoom[i-1].first.day &&
                slotRoom[i].first.pairNumber == slotRoom[i-1].first.pairNumber + 1) {
                // Две пары подряд
                const Room& r1 = rooms[slotRoom[i-1].second];
                const Room& r2 = rooms[slotRoom[i].second];
                if (r1.buildingId != r2.buildingId) {
                    d.buildingTransfers++;
                    d.softPenalty += PENALTY_BUILDING_TRANSFER;
                }
            }
        }
    }

    d.total = d.softPenalty;
    if (details) *details = d;

    // Возвращаем фитнес: 1000000 - штрафы
    return 1000000 - d.total;
}