#ifndef GENETIC_SCHEDULER_H
#define GENETIC_SCHEDULER_H

#include "scheduler.h"
#include "models.h"

#include <QVector>
#include <random>
#include <utility>

// Ген хромосомы: временной слот (день/пара) и аудитория, назначенные одной
// "сессии" дисциплины (одному конкретному занятию в расписании).
struct GAGene
{
    int day = 1;         // 1..kDays
    int pairNumber = 1;  // 1..kPairs
    int roomIndex = 0;   // индекс в векторе аудиторий
};

using GAChromosome = QVector<GAGene>;

// Параметры генетического алгоритма.
struct GAConfig
{
    int populationSize = 80;
    int maxGenerations = 300;
    double crossoverRate = 0.85;
    double mutationRate = 0.15;
    int tournamentSize = 4;
    double elitismRate = 0.08;
    int stagnationLimit = 60; // остановка, если решение не улучшается N поколений подряд
};

// Генетический алгоритм построения расписания.
//
// Реализует IScheduler (см. scheduler.h), поэтому подключается в
// MainWindow вместо StubScheduler без каких-либо изменений остального GUI —
// достаточно заменить `new StubScheduler()` на `new GeneticScheduler()`.
//
// Ключевая особенность модели: одна "сессия" дисциплины (одна запись в
// расписании) охватывает ВСЕ целевые группы этой дисциплины одновременно —
// это позволяет вести, например, лекцию сразу для нескольких групп в одной
// аудитории, при условии, что вместимость аудитории покрывает суммарное
// число студентов всех групп. Недостаточная вместимость учитывается как
// штраф в функции приспособленности, поэтому алгоритм стремится подобрать
// подходящую по размеру аудиторию; если такой аудитории нет вовсе — занятие
// попадёт в список "не удалось назначить" с указанием причины.
class GeneticScheduler : public IScheduler
{
public:
    explicit GeneticScheduler(const GAConfig& config = GAConfig());

    GenerationResult generate(const QVector<Group>& groups,
                               const QVector<Teacher>& teachers,
                               const QVector<Room>& rooms,
                               const QVector<Subject>& subjects) override;

private:
    static constexpr int kDays = 6;
    static constexpr int kPairs = 6;

    // Одна задача = одно занятие, которое нужно расставить в расписании.
    // Если у дисциплины несколько групп, все они входят в одну задачу
    // (совместное занятие, например лекция на поток).
    struct Task
    {
        int subjectIndex = -1;
        int teacherIndex = -1;
        QVector<int> groupIndices;
        int totalStudents = 0;
    };

    GAConfig config_;
    mutable std::mt19937 rng_;
    mutable std::uniform_real_distribution<double> prob_{0.0, 1.0};

    QVector<Group> groups_;
    QVector<Teacher> teachers_;
    QVector<Room> rooms_;
    QVector<Subject> subjects_;
    QVector<Task> tasks_;

    QVector<GAChromosome> population_;
    QVector<double> fitness_;
    GAChromosome bestChromosome_;
    double bestFitness_ = 0.0;

    bool buildTasks(const QVector<Subject>& subjects, GenerationResult& result);

    GAGene randomGene() const;
    GAChromosome randomChromosome() const;

    void initializePopulation();
    void evaluatePopulation();
    double evaluateChromosome(const GAChromosome& chromosome) const;

    GAChromosome tournamentSelect() const;
    std::pair<GAChromosome, GAChromosome> crossover(const GAChromosome& p1, const GAChromosome& p2) const;
    void mutate(GAChromosome& chromosome) const;
    void applyElitism(QVector<GAChromosome>& newPopulation) const;

    bool teacherAvailable(int teacherIndex, int day, int pair) const;

    GenerationResult buildResult(const GAChromosome& chromosome) const;
};

#endif // GENETIC_SCHEDULER_H
