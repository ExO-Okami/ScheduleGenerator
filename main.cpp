#ifdef _WIN32
#include <windows.h>
#endif

#include "genetic_algorithm.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

// ===== Генерация тестовых данных =====
void generateTestData(std::vector<Group>& groups,
                      std::vector<Teacher>& teachers,
                      std::vector<Room>& rooms,
                      std::vector<Event>& events)
{
    // === 10 ГРУПП ===
    for (int i = 0; i < 10; ++i) {
        groups.push_back({i, 20 + (i % 5) * 3, "ИТ-" + std::to_string(101 + i)});
    }

    // === 12 ПРЕПОДАВАТЕЛЕЙ ===
    std::vector<std::string> teacherNames = {
        "математика Иванов И.И.", "физика Петров П.П.", "математика Сидоров С.С.", "физика Кузнецов А.А.",
        "программирование Смирнов В.В.", "программирование Попов Д.Д.", "базы данных Васильев Е.Е.", "базы данных Зайцев Ж.Ж.",
        "алгоритмы Новиков З.З.", "алгоритмы Морозов И.И.", "сети Волков К.К.", "физра Лебедев Л.Л."
    };
    for (int i = 0; i < 12; ++i) {
        teachers.push_back({i, teacherNames[i]});
    }

    // === 18 АУДИТОРИЙ (3 корпуса) ===
    // Корпус 1: 6 аудиторий
    rooms.push_back({0, 1, 30, ROOM_LECTURE,  "А-101"});
    rooms.push_back({1, 1, 30, ROOM_PRACTICE, "А-102"});
    rooms.push_back({2, 1, 40, ROOM_LECTURE,  "А-201"});
    rooms.push_back({3, 1, 40, ROOM_PRACTICE, "А-202"});
    rooms.push_back({4, 1, 50, ROOM_LECTURE,  "А-301"});
    rooms.push_back({5, 1, 50, ROOM_PRACTICE, "А-302"});
    
    // Корпус 2: 6 аудиторий
    rooms.push_back({6, 2, 30, ROOM_LECTURE,  "Б-101"});
    rooms.push_back({7, 2, 30, ROOM_PRACTICE, "Б-102"});
    rooms.push_back({8, 2, 40, ROOM_LECTURE,  "Б-201"});
    rooms.push_back({9, 2, 40, ROOM_PRACTICE, "Б-202"});
    rooms.push_back({10, 2, 50, ROOM_LECTURE, "Б-301"});
    rooms.push_back({11, 2, 50, ROOM_PRACTICE, "Б-302"});
    
    // Корпус 3: 6 аудиторий (включая спортзал)
    rooms.push_back({12, 3, 30, ROOM_LECTURE,  "В-101"});
    rooms.push_back({13, 3, 30, ROOM_PRACTICE, "В-102"});
    rooms.push_back({14, 3, 40, ROOM_LECTURE,  "В-201"});
    rooms.push_back({15, 3, 40, ROOM_PRACTICE, "В-202"});
    rooms.push_back({16, 3, 100, ROOM_LECTURE, "В-301"});  // Большая лекционная
    rooms.push_back({17, 3, 60, ROOM_SPORTS,   "В-Спорт"});

    // === СОБЫТИЯ (занятия) ===
    // Каждая группа изучает 7 предметов (4 преподавателя)
    // 7 предметов × 2 типа (лекция + практика) = 14 занятий на группу
    // 10 групп × 14 = 140 событий
    
    int eid = 0;
    auto add = [&](int tid, int gid, int rtype, int dur) {
        events.push_back({eid++, tid, gid, rtype, dur});
    };

    // Для каждой группы создаём 14 занятий
    for (int g = 0; g < 10; ++g) {
        // Предмет 1: Математика (препод 0, 2)
        add(0, g, ROOM_LECTURE,  1);  // Лекция
        add(2, g, ROOM_PRACTICE, 2);  // Практика
        
        // Предмет 2: Физика (препод 1, 3)
        add(1, g, ROOM_LECTURE,  1);
        add(3, g, ROOM_PRACTICE, 2);
        
        // Предмет 3: Программирование (препод 4, 5)
        add(4, g, ROOM_LECTURE,  1);
        add(5, g, ROOM_PRACTICE, 2);
        
        // Предмет 4: Базы данных (препод 6, 7)
        add(6, g, ROOM_LECTURE,  1);
        add(7, g, ROOM_PRACTICE, 2);
        
        // Предмет 5: Алгоритмы (препод 8, 9)
        add(8, g, ROOM_LECTURE,  1);
        add(9, g, ROOM_PRACTICE, 2);
        
        // Предмет 6: Сети (препод 10)
        add(10, g, ROOM_LECTURE, 1);
        add(10, g, ROOM_PRACTICE, 1);
        
        // Предмет 7: Физкультура (препод 11, спортзал)
        add(11, g, ROOM_SPORTS, 2);
    }
}

// ===== Красивый вывод расписания =====
void printSchedule(const Chromosome& best,
                   const std::vector<Event>& events,
                   const std::vector<Group>& groups,
                   const std::vector<Teacher>& teachers,
                   const std::vector<Room>& rooms)
{
    const char* dayNames[] = {"ПН", "ВТ", "СР", "ЧТ", "ПТ", "СБ"};

    std::cout << "\n========== ЛУЧШЕЕ РАСПИСАНИЕ ==========\n";
    for (const auto& g : best) {
        const Event& e = events[g.eventId];
        const Group& gr = groups[e.groupId];
        const Teacher& t = teachers[e.teacherId];
        const Room& r = rooms[g.roomId];
        std::cout << std::setw(8) << gr.name
                  << " | " << dayNames[g.slot.day] << " " << (g.slot.pairNumber + 1) << "-я пара"
                  << " | " << std::setw(14) << t.fullName
                  << " | ауд. " << r.number << " (корп. " << r.buildingId << ")\n";
    }
    std::cout << "========================================\n";
}

// ===== Точка входа =====
int main()
{
    #ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
    #endif

    std::vector<Group> groups;
    std::vector<Teacher> teachers;
    std::vector<Room> rooms;
    std::vector<Event> events;

    generateTestData(groups, teachers, rooms, events);

    std::cout << "Задача: " << events.size() << " занятий, "
              << groups.size() << " групп, "
              << teachers.size() << " преподавателей, "
              << rooms.size() << " аудиторий.\n\n";

    // Заголовок таблицы мониторинга
    std::cout << std::left
              << std::setw(6)  << "Покол "
              << std::setw(10) << "Лучший "
              << std::setw(10) << "Средний "
              << std::setw(10) << "Худший "
              << std::setw(10) << "Жёстк. "
              << std::setw(10) << "Мягк. "
              << std::setw(10) << "Конфл.Пр "
              << std::setw(10) << "Конфл.Гр "
              << std::setw(10) << "Конфл.Ауд "
              << std::setw(8)  << "Окна "
              << "\n";
    std::cout << std::string(100, '-') << "\n";

    GeneticAlgorithm::Config cfg;
    cfg.populationSize = 500;
    cfg.maxGenerations = 3000;
    cfg.crossoverRate = 0.85;
    cfg.mutationRate = 0.15;
    cfg.tournamentSize = 3;
    cfg.elitismRate = 0.05;
    cfg.reportEvery = 50;

    GeneticAlgorithm ga(events, groups, teachers, rooms, GAConfig{});

    // Колбэк мониторинга
    ga.setMonitorCallback([](const GenerationStats& s) {
        std::cout << std::left
                  << std::setw(6)  << s.generation
                  << std::setw(10) << s.bestFitness
                  << std::setw(10) << s.avgFitness
                  << std::setw(10) << s.worstFitness
                  << std::setw(10) << s.bestDetails.hardPenalty
                  << std::setw(10) << s.bestDetails.softPenalty
                  << std::setw(10) << s.bestDetails.teacherConflicts
                  << std::setw(10) << s.bestDetails.groupConflicts
                  << std::setw(10) << s.bestDetails.roomConflicts
                  << std::setw(8)  << s.bestDetails.groupGaps
                  << "\n";
    });

    Chromosome best = ga.run();

    // Финальный отчёт
    std::cout << "\n========== ИТОГ ==========\n";
    std::cout << "Лучший фитнес: " << ga.getBestFitness() << "\n";
    const auto& d = ga.getBestDetails();
    std::cout << "Жёсткие штрафы: " << d.hardPenalty
              << " (конф. преп: " << d.teacherConflicts
              << ", конф. групп: " << d.groupConflicts
              << ", конф. ауд: " << d.roomConflicts << ")\n";
    std::cout << "Мягкие штрафы:  " << d.softPenalty
              << " (окна: " << d.groupGaps
              << ", переезды: " << d.buildingTransfers << ")\n";

    printSchedule(best, events, groups, teachers, rooms);

    return 0;
}