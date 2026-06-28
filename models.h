#ifndef MODELS_H
#define MODELS_H

#include <vector>
#include <string>

// Константы для статических массивов
constexpr int MAX_DAYS = 6;        // ПН-СБ (индексы 0-5)
constexpr int MAX_PAIRS = 7;       // 1-7 пары (индексы 0-6)

// Типы аудиторий
constexpr int ROOM_LECTURE = 1;    // Лекционная
constexpr int ROOM_PRACTICE = 2;   // Практическая
constexpr int ROOM_SPORTS = 8;     // Спортивный зал

// Временной слот
struct Slot {
    int day;        // 0-5 (ПН=0, ВТ=1, ..., СБ=5)
    int pairNumber; // 0-6 (1-я пара = 0, 7-я пара = 6)
    
    int toIndex() const { 
        return day * MAX_PAIRS + pairNumber; 
    }
    
    bool operator<(const Slot& other) const {
        return toIndex() < other.toIndex();
    }
    
    bool operator==(const Slot& other) const {
        return day == other.day && pairNumber == other.pairNumber;
    }
    
    bool operator!=(const Slot& other) const {
        return !(*this == other);
    }
};

// Учебная группа
struct Group {
    int id;
    int studentsCount;
    std::string name;
};

// Преподаватель
struct Teacher {
    int id;
    std::string fullName;
    bool availability[MAX_DAYS][MAX_PAIRS];
    
    Teacher() {
        for(int d = 0; d < MAX_DAYS; ++d)
            for(int p = 0; p < MAX_PAIRS; ++p)
                availability[d][p] = true;
    }
    
    // Добавьте этот конструктор:
    Teacher(int id_, std::string name) : id(id_), fullName(name) {
        for(int d = 0; d < MAX_DAYS; ++d)
            for(int p = 0; p < MAX_PAIRS; ++p)
                availability[d][p] = true;
    }
    
    bool isAvailable(int day, int pair) const {
        if (day < 0 || day >= MAX_DAYS || pair < 0 || pair >= MAX_PAIRS)
            return false;
        return availability[day][pair];
    }
};

// Аудитория
struct Room {
    int id;
    int buildingId;
    int capacity;
    int typeMask;
    std::string number;
    
    bool isSuitable(int requiredTypeMask) const {
        return (typeMask & requiredTypeMask) != 0;
    }
};

// Событие
struct Event {
    int id;
    int teacherId;
    int groupId;
    int requiredRoomTypeMask;
    int duration;
};

// Ген хромосомы
struct Gene {
    int eventId;
    int roomId;
    Slot slot;
};

using Chromosome = std::vector<Gene>;

// Индексы для быстрого поиска
struct ConflictIndex {
    std::vector<std::vector<std::vector<int>>> teacherSchedule;
    std::vector<std::vector<std::vector<int>>> groupSchedule;
    std::vector<std::vector<std::vector<int>>> roomSchedule;
    std::vector<std::vector<int>> groupGenes;
    std::vector<std::vector<int>> teacherGenes;
    
    void initialize(int numTeachers, int numGroups, int numRooms) {
        teacherSchedule.assign(numTeachers, 
            std::vector<std::vector<int>>(MAX_DAYS, std::vector<int>(MAX_PAIRS, -1)));
        
        groupSchedule.assign(numGroups,
            std::vector<std::vector<int>>(MAX_DAYS, std::vector<int>(MAX_PAIRS, -1)));
        
        roomSchedule.assign(numRooms,
            std::vector<std::vector<int>>(MAX_DAYS, std::vector<int>(MAX_PAIRS, -1)));
        
        groupGenes.resize(numGroups);
        teacherGenes.resize(numTeachers);
    }
    
    void buildFromChromosome(const Chromosome& chromosome, 
                             const std::vector<Event>& events) {
        for(auto& t : teacherSchedule)
            for(auto& d : t)
                for(auto& p : d) p = -1;
        
        for(auto& g : groupSchedule)
            for(auto& d : g)
                for(auto& p : d) p = -1;
        
        for(auto& r : roomSchedule)
            for(auto& d : r)
                for(auto& p : d) p = -1;
        
        for(auto& gg : groupGenes) gg.clear();
        for(auto& tg : teacherGenes) tg.clear();
        
        for(size_t geneIdx = 0; geneIdx < chromosome.size(); ++geneIdx) {
            const Gene& gene = chromosome[geneIdx];
            const Event& event = events[gene.eventId];
            
            teacherSchedule[event.teacherId][gene.slot.day][gene.slot.pairNumber] = geneIdx;
            teacherGenes[event.teacherId].push_back(geneIdx);
            
            groupSchedule[event.groupId][gene.slot.day][gene.slot.pairNumber] = geneIdx;
            groupGenes[event.groupId].push_back(geneIdx);
            
            roomSchedule[gene.roomId][gene.slot.day][gene.slot.pairNumber] = geneIdx;
        }
    }
};

#endif // MODELS_H