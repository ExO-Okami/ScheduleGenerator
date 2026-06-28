#ifndef FITNESS_H
#define FITNESS_H

#include "models.h"

// Детализация штрафов — для мониторинга
struct FitnessDetails {
    int total = 0;
    int hardPenalty = 0;    // Конфликты (критично)
    int softPenalty = 0;    // Окна, переезды, неудобства

    // Детализация по типам нарушений
    int teacherConflicts = 0;
    int groupConflicts = 0;
    int roomConflicts = 0;
    int capacityViolations = 0;
    int roomTypeViolations = 0;
    int availabilityViolations = 0;
    int groupGaps = 0;          // "Окна" у групп
    int buildingTransfers = 0;  // Переезды между корпусами без перерыва
};

// Основная функция оценки
int calculateFitnessValue(const Chromosome& chromosome,
                          const std::vector<Event>& events,
                          const std::vector<Group>& groups,
                          const std::vector<Teacher>& teachers,
                          const std::vector<Room>& rooms,
                          const ConflictIndex& conflictIndex,
                          FitnessDetails* details = nullptr);

#endif // FITNESS_H