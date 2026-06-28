#ifndef GENETIC_ALGORITHM_H
#define GENETIC_ALGORITHM_H

#include "models.h"
#include "fitness.h"
#include <vector>
#include <random>
#include <functional>

// 1. СНАЧАЛА определяем Config (вне класса!)
struct GAConfig {
    int populationSize = 100;
    int maxGenerations = 500;
    double crossoverRate = 0.8;
    double mutationRate = 0.1;
    int tournamentSize = 3;
    double elitismRate = 0.05;
    int reportEvery = 10;
};

// 2. Потом GenerationStats
struct GenerationStats {
    int generation = 0;
    int bestFitness = 0;
    int avgFitness = 0;
    int worstFitness = 0;
    FitnessDetails bestDetails;
};

// 3. И только потом класс
class GeneticAlgorithm {
public:
    using Config = GAConfig;
    using MonitorCallback = std::function<void(const GenerationStats&)>;

    // Конструктор БЕЗ значения по умолчанию в объявлении
    GeneticAlgorithm(const std::vector<Event>& events,
                     const std::vector<Group>& groups,
                     const std::vector<Teacher>& teachers,
                     const std::vector<Room>& rooms,
                     const Config& config);

    void setMonitorCallback(MonitorCallback cb) { monitor_ = std::move(cb); }
    Chromosome run();
    const Chromosome& getBestChromosome() const { return bestChromosome_; }
    int getBestFitness() const { return bestFitness_; }
    const FitnessDetails& getBestDetails() const { return bestDetails_; }
    void localSearch(Chromosome& chromosome);
    void repairConflicts(Chromosome& chromosome);

private:
    std::vector<Event> events_;
    std::vector<Group> groups_;
    std::vector<Teacher> teachers_;
    std::vector<Room> rooms_;
    Config config_;

    std::vector<Chromosome> population_;
    std::vector<int> fitnessValues_;
    std::vector<FitnessDetails> detailsCache_;
    Chromosome bestChromosome_;
    int bestFitness_ = -1;
    FitnessDetails bestDetails_{};

    std::mt19937 rng_;
    std::uniform_real_distribution<> probDist_;
    ConflictIndex conflictIndex_;
    MonitorCallback monitor_;

    void initializePopulation();
    Chromosome createRandomChromosome();
    void evaluatePopulation(int generation);
    int calculateFitness(const Chromosome& chromosome, FitnessDetails* details = nullptr);
    Chromosome tournamentSelection();
    std::pair<Chromosome, Chromosome> crossover(const Chromosome& p1, const Chromosome& p2);
    void mutate(Chromosome& chromosome);
    void applyElitism(std::vector<Chromosome>& newPopulation);
    Slot getRandomSlot();
    int getRandomRoom();
};

#endif