#include "genetic_algorithm.h"
#include <algorithm>
#include <numeric>
#include <iostream>

GeneticAlgorithm::GeneticAlgorithm(const std::vector<Event>& events,
                                   const std::vector<Group>& groups,
                                   const std::vector<Teacher>& teachers,
                                   const std::vector<Room>& rooms,
                                   const Config& config)
    : events_(events), groups_(groups), teachers_(teachers), rooms_(rooms),
      config_(config), rng_(std::random_device{}()), probDist_(0.0, 1.0)
{
    conflictIndex_.initialize(teachers_.size(), groups_.size(), rooms_.size());
}

Chromosome GeneticAlgorithm::run() {
    initializePopulation();

    for (int generation = 0; generation < config_.maxGenerations; ++generation) {
        evaluatePopulation(generation);

        if (monitor_ && (generation % config_.reportEvery == 0 ||
                         generation == config_.maxGenerations - 1)) {
            GenerationStats stats;
            stats.generation = generation;
            stats.bestFitness = bestFitness_;
            stats.bestDetails = bestDetails_;

            int sum = 0, mn = fitnessValues_[0], mx = fitnessValues_[0];
            for (int f : fitnessValues_) { sum += f; mn = std::min(mn, f); mx = std::max(mx, f); }
            stats.avgFitness = sum / (int)fitnessValues_.size();
            stats.worstFitness = mn;

            monitor_(stats);
        }

        if (bestFitness_ >= 1000000) {
            std::cout << "[GA] Идеальное решение найдено на поколении " << generation << "!\n";
            break;
        }

        std::vector<Chromosome> newPopulation;
        applyElitism(newPopulation);

        while (newPopulation.size() < (size_t)config_.populationSize) {
            Chromosome p1 = tournamentSelection();
            Chromosome p2 = tournamentSelection();
            auto [c1, c2] = crossover(p1, p2);
            mutate(c1);
            mutate(c2);
            newPopulation.push_back(c1);
            if (newPopulation.size() < (size_t)config_.populationSize) newPopulation.push_back(c2);
        }
        population_ = std::move(newPopulation);
    }
    
    return bestChromosome_;
}

int GeneticAlgorithm::calculateFitness(const Chromosome& chromosome, FitnessDetails* details)
{
    conflictIndex_.buildFromChromosome(chromosome, events_);
    return calculateFitnessValue(chromosome, events_, groups_, teachers_, rooms_,
                                 conflictIndex_, details);
}

void GeneticAlgorithm::evaluatePopulation(int /*generation*/)
{
    fitnessValues_.resize(population_.size());
    detailsCache_.resize(population_.size());

    int currentBest = -1;
    int currentBestIdx = -1;

    for (size_t i = 0; i < population_.size(); ++i) {
        fitnessValues_[i] = calculateFitness(population_[i], &detailsCache_[i]);
        if (fitnessValues_[i] > currentBest) {
            currentBest = fitnessValues_[i];
            currentBestIdx = (int)i;
        }
    }

    if (currentBest > bestFitness_) {
        bestFitness_ = currentBest;
        bestChromosome_ = population_[currentBestIdx];
        bestDetails_ = detailsCache_[currentBestIdx];
    }
}

Chromosome GeneticAlgorithm::tournamentSelection() {
    int bestIdx = -1, bestFit = -1;
    for (int i = 0; i < config_.tournamentSize; ++i) {
        int idx = std::uniform_int_distribution<>(0, (int)population_.size() - 1)(rng_);
        if (fitnessValues_[idx] > bestFit) { bestFit = fitnessValues_[idx]; bestIdx = idx; }
    }
    return population_[bestIdx];
}

std::pair<Chromosome, Chromosome> GeneticAlgorithm::crossover(const Chromosome& p1, const Chromosome& p2) {
    if (probDist_(rng_) > config_.crossoverRate) return {p1, p2};
    Chromosome c1, c2;
    c1.reserve(p1.size()); c2.reserve(p2.size());
    for (size_t i = 0; i < p1.size(); ++i) {
        if (probDist_(rng_) < 0.5) { c1.push_back(p1[i]); c2.push_back(p2[i]); }
        else                        { c1.push_back(p2[i]); c2.push_back(p1[i]); }
    }
    return {c1, c2};
}

void GeneticAlgorithm::mutate(Chromosome& chromosome) {
    for (size_t i = 0; i < chromosome.size(); ++i) {
        if (probDist_(rng_) < config_.mutationRate) {
            Gene& gene = chromosome[i];
            const Event& event = events_[gene.eventId];
            
            bool moved = false;
            int attempts = 0;
            const int MAX_ATTEMPTS = 50;
            
            while (!moved && attempts < MAX_ATTEMPTS) {
                attempts++;
                
                Slot trialSlot = getRandomSlot();
                int trialRoom = getRandomRoom();
                
                // Проверяем доступность преподавателя
                if (!teachers_[event.teacherId].isAvailable(trialSlot.day, trialSlot.pairNumber)) {
                    continue;
                }
                
                // Проверяем, не создаст ли это конфликтов
                bool hasConflict = false;
                
                for (size_t j = 0; j < chromosome.size(); ++j) {
                    if (j == i) continue;
                    
                    const Gene& other = chromosome[j];
                    if (other.slot == trialSlot) {
                        const Event& otherEvent = events_[other.eventId];
                        
                        if (other.roomId == trialRoom ||
                            otherEvent.teacherId == event.teacherId ||
                            otherEvent.groupId == event.groupId) {
                            hasConflict = true;
                            break;
                        }
                    }
                }
                
                if (!hasConflict) {
                    gene.slot = trialSlot;
                    gene.roomId = trialRoom;
                    moved = true;
                }
            }
        }
    }
}

void GeneticAlgorithm::applyElitism(std::vector<Chromosome>& np) {
    int elite = std::max(1, (int)(config_.populationSize * config_.elitismRate));
    std::vector<std::pair<int,int>> fi;
    fi.reserve(population_.size());
    for (size_t i = 0; i < population_.size(); ++i) fi.emplace_back(fitnessValues_[i], (int)i);
    std::sort(fi.begin(), fi.end(), [](auto& a, auto& b){ return a.first > b.first; });
    for (int i = 0; i < elite; ++i) np.push_back(population_[fi[i].second]);
}

Slot GeneticAlgorithm::getRandomSlot() {
    Slot s;
    s.day = std::uniform_int_distribution<>(0, MAX_DAYS - 1)(rng_);
    s.pairNumber = std::uniform_int_distribution<>(0, MAX_PAIRS - 1)(rng_);
    return s;
}

int GeneticAlgorithm::getRandomRoom() {
    return std::uniform_int_distribution<>(0, (int)rooms_.size() - 1)(rng_);
}

void GeneticAlgorithm::initializePopulation() {
    population_.resize(config_.populationSize);
    for (auto& c : population_) c = createRandomChromosome();
}

Chromosome GeneticAlgorithm::createRandomChromosome() {
    Chromosome chromosome;
    chromosome.reserve(events_.size());
    
    // Вспомогательная структура для отслеживания занятых слотов
    // [day][pair] = список (eventId, roomId, groupId, teacherId)
    struct OccupiedSlot {
        int eventId;
        int roomId;
        int groupId;
        int teacherId;
    };
    std::vector<std::vector<std::vector<OccupiedSlot>>> occupied(
        MAX_DAYS, std::vector<std::vector<OccupiedSlot>>(MAX_PAIRS));
    
    for (const auto& event : events_) {
        Gene gene;
        gene.eventId = event.id;
        
        bool placed = false;
        int attempts = 0;
        const int MAX_ATTEMPTS = 100;
        
        while (!placed && attempts < MAX_ATTEMPTS) {
            attempts++;
            
            Slot trialSlot = getRandomSlot();
            int trialRoom = getRandomRoom();
            
            // Проверяем доступность преподавателя
            if (!teachers_[event.teacherId].isAvailable(trialSlot.day, trialSlot.pairNumber)) {
                continue;
            }
            
            // Проверяем, не создаст ли это конфликтов
            bool hasConflict = false;
            
            for (const auto& occ : occupied[trialSlot.day][trialSlot.pairNumber]) {
                // Конфликт аудитории?
                if (occ.roomId == trialRoom) {
                    hasConflict = true;
                    break;
                }
                
                // Конфликт преподавателя?
                if (occ.teacherId == event.teacherId) {
                    hasConflict = true;
                    break;
                }
                
                // Конфликт группы?
                if (occ.groupId == event.groupId) {
                    hasConflict = true;
                    break;
                }
            }
            
            if (!hasConflict) {
                // Нашли валидное место!
                gene.slot = trialSlot;
                gene.roomId = trialRoom;
                chromosome.push_back(gene);
                
                // Запоминаем, что этот слот занят
                occupied[trialSlot.day][trialSlot.pairNumber].push_back({
                    event.id, trialRoom, event.groupId, event.teacherId
                });
                
                placed = true;
            }
        }
        
        if (!placed) {
            // Если не нашли валидное место за 100 попыток — ставим случайное
            // (это создаст конфликт, но ГА его исправит через мутацию)
            gene.slot = getRandomSlot();
            gene.roomId = getRandomRoom();
            chromosome.push_back(gene);
        }
    }
    
    return chromosome;
}

void GeneticAlgorithm::localSearch(Chromosome& chromosome) {
    std::cout << "[GA] Запуск локального поиска...\n";
    
    int currentFitness = calculateFitness(chromosome, nullptr);
    bool improved = true;
    int iterations = 0;
    
    while (improved && iterations < 100) {
        improved = false;
        iterations++;
        
        // Для каждого гена пробуем все возможные слоты и аудитории
        for (size_t i = 0; i < chromosome.size(); ++i) {
            Gene bestGene = chromosome[i];
            int bestFitness = currentFitness;
            
            // Пробуем все слоты (6 дней × 7 пар = 42 варианта)
            for (int day = 0; day < MAX_DAYS; ++day) {
                for (int pair = 0; pair < MAX_PAIRS; ++pair) {
                    // Пробуем все подходящие аудитории
                    for (size_t r = 0; r < rooms_.size(); ++r) {
                        Gene trialGene = chromosome[i];
                        trialGene.slot = {day, pair};
                        trialGene.roomId = (int)r;
                        
                        // Временно заменяем ген
                        Gene original = chromosome[i];
                        chromosome[i] = trialGene;
                        
                        int newFitness = calculateFitness(chromosome, nullptr);
                        
                        if (newFitness > bestFitness) {
                            bestFitness = newFitness;
                            bestGene = trialGene;
                            improved = true;
                        }
                        
                        // Возвращаем оригинал
                        chromosome[i] = original;
                    }
                }
            }
            
            // Применяем лучшее найденное
            if (bestFitness > currentFitness) {
                chromosome[i] = bestGene;
                currentFitness = bestFitness;
            }
        }
        
        std::cout << "  Итерация " << iterations << ": фитнес = " << currentFitness << "\n";
    }
    
    std::cout << "[GA] Локальный поиск завершён. Фитнес: " << currentFitness << "\n";
}

void GeneticAlgorithm::repairConflicts(Chromosome& chromosome) {
    std::cout << "[GA] Запуск ремонта конфликтов...\n";
    
    bool hasConflicts = true;
    int repairAttempts = 0;
    
    while (hasConflicts && repairAttempts < 50) {
        hasConflicts = false;
        repairAttempts++;
        
        // Перестраиваем индексы
        conflictIndex_.buildFromChromosome(chromosome, events_);
        
        // Ищем конфликты аудиторий
        for (size_t r = 0; r < rooms_.size(); ++r) {
            for (int day = 0; day < MAX_DAYS; ++day) {
                for (int pair = 0; pair < MAX_PAIRS; ++pair) {
                    // Считаем, сколько занятий в этой аудитории в это время
                    int count = 0;
                    std::vector<size_t> conflictGenes;
                    
                    for (size_t i = 0; i < chromosome.size(); ++i) {
                        if (chromosome[i].roomId == (int)r && 
                            chromosome[i].slot.day == day && 
                            chromosome[i].slot.pairNumber == pair) {
                            count++;
                            conflictGenes.push_back(i);
                        }
                    }
                    
                    // Если больше 1 занятия — конфликт!
                    if (count > 1) {
                        hasConflicts = true;
                        
                        // Оставляем первое занятие, остальные перемещаем
                        for (size_t k = 1; k < conflictGenes.size(); ++k) {
                            size_t geneIdx = conflictGenes[k];
                            Gene& gene = chromosome[geneIdx];
                            const Event& event = events_[gene.eventId];
                            
                            // Ищем свободное место
                            bool moved = false;
                            
                            // Пробуем все слоты
                            for (int d = 0; d < MAX_DAYS && !moved; ++d) {
                                for (int p = 0; p < MAX_PAIRS && !moved; ++p) {
                                    // Проверяем доступность преподавателя
                                    if (!teachers_[event.teacherId].isAvailable(d, p)) continue;
                                    
                                    // Проверяем, свободна ли аудитория
                                    bool roomFree = true;
                                    for (size_t i = 0; i < chromosome.size(); ++i) {
                                        if (i == geneIdx) continue;
                                        if (chromosome[i].roomId == gene.roomId &&
                                            chromosome[i].slot.day == d &&
                                            chromosome[i].slot.pairNumber == p) {
                                            roomFree = false;
                                            break;
                                        }
                                    }
                                    
                                    if (!roomFree) continue;
                                    
                                    // Проверяем, свободен ли преподаватель
                                    bool teacherFree = true;
                                    for (size_t i = 0; i < chromosome.size(); ++i) {
                                        if (i == geneIdx) continue;
                                        if (events_[chromosome[i].eventId].teacherId == event.teacherId &&
                                            chromosome[i].slot.day == d &&
                                            chromosome[i].slot.pairNumber == p) {
                                            teacherFree = false;
                                            break;
                                        }
                                    }
                                    
                                    if (!teacherFree) continue;
                                    
                                    // Проверяем, свободна ли группа
                                    bool groupFree = true;
                                    for (size_t i = 0; i < chromosome.size(); ++i) {
                                        if (i == geneIdx) continue;
                                        if (events_[chromosome[i].eventId].groupId == event.groupId &&
                                            chromosome[i].slot.day == d &&
                                            chromosome[i].slot.pairNumber == p) {
                                            groupFree = false;
                                            break;
                                        }
                                    }
                                    
                                    if (!groupFree) continue;
                                    
                                    // Нашли свободное место!
                                    gene.slot = {d, p};
                                    moved = true;
                                    std::cout << "  Исправлен конфликт: событие " << gene.eventId 
                                              << " перемещено на " << d << ":" << p << "\n";
                                }
                            }
                            
                            if (!moved) {
                                std::cout << "  ⚠ Не удалось исправить конфликт для события " 
                                          << gene.eventId << "\n";
                            }
                        }
                    }
                }
            }
        }
        
        std::cout << "  Попытка ремонта " << repairAttempts 
                  << ": конфликтов = " << (hasConflicts ? "есть" : "нет") << "\n";
    }
    
    std::cout << "[GA] Ремонт завершён.\n";
}