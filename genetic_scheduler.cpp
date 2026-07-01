#include "genetic_scheduler.h"

#include <QCoreApplication>
#include <QMap>
#include <QPair>
#include <QStringList>

#include <algorithm>
#include <limits>
#include <numeric>

namespace {

// Пересекаются ли два набора индексов групп (используется и для проверки
// конфликта "группа уже занята в этом слоте", и для расчёта штрафов ГА).
bool groupsOverlap(const QVector<int>& a, const QVector<int>& b)
{
    for (int x : a) {
        if (b.contains(x))
            return true;
    }
    return false;
}

} // namespace

GeneticScheduler::GeneticScheduler(const GAConfig& config)
    : config_(config), rng_(std::random_device{}())
{
}

bool GeneticScheduler::teacherAvailable(int teacherIndex, int day, int pair) const
{
    if (teacherIndex < 0 || teacherIndex >= teachers_.size())
        return false;

    const Teacher& teacher = teachers_.at(teacherIndex);

    // Если для преподавателя вообще не задана доступность, считаем его
    // доступным в любое время (иначе такие данные, как в test_data.json,
    // где доступность указана лишь у части преподавателей, сделали бы
    // планирование части дисциплин в принципе невозможным).
    if (teacher.availability.isEmpty())
        return true;

    if (!teacher.availability.contains(day))
        return false;

    return teacher.availability.value(day).contains(pair);
}

bool GeneticScheduler::buildTasks(const QVector<Subject>& subjects, GenerationResult& result)
{
    tasks_.clear();

    QMap<QString, int> teacherIndexByName;
    for (int i = 0; i < teachers_.size(); ++i)
        teacherIndexByName.insert(teachers_[i].fullName, i);

    QMap<QString, int> groupIndexByName;
    for (int i = 0; i < groups_.size(); ++i)
        groupIndexByName.insert(groups_[i].name, i);

    for (int si = 0; si < subjects.size(); ++si) {
        const Subject& subj = subjects.at(si);

        if (!teacherIndexByName.contains(subj.teacherId)) {
            UnscheduledLesson u;
            u.subject = subj;
            u.reason = "Не найден преподаватель \"" + subj.teacherId + "\" для дисциплины";
            result.unscheduled.append(u);
            continue;
        }

        QVector<int> groupIndices;
        int totalStudents = 0;
        for (const QString& gname : subj.targetGroups) {
            if (!groupIndexByName.contains(gname))
                continue;
            const int gi = groupIndexByName.value(gname);
            groupIndices.append(gi);
            totalStudents += groups_.at(gi).studentsCount;
        }

        if (groupIndices.isEmpty()) {
            UnscheduledLesson u;
            u.subject = subj;
            u.reason = "Не найдено ни одной целевой группы для дисциплины";
            result.unscheduled.append(u);
            continue;
        }

        const int teacherIndex = teacherIndexByName.value(subj.teacherId);
        // 1 пара = 2 академических часа (как и в StubScheduler).
        const int lessonsCount = qMax(1, (subj.hours + 1) / 2);

        for (int k = 0; k < lessonsCount; ++k) {
            Task t;
            t.subjectIndex = si;
            t.teacherIndex = teacherIndex;
            t.groupIndices = groupIndices;
            t.totalStudents = totalStudents;
            tasks_.append(t);
        }
    }

    return !tasks_.isEmpty();
}

GAGene GeneticScheduler::randomGene() const
{
    GAGene g;
    g.day = std::uniform_int_distribution<int>(1, kDays)(rng_);
    g.pairNumber = std::uniform_int_distribution<int>(1, kPairs)(rng_);
    g.roomIndex = std::uniform_int_distribution<int>(0, rooms_.size() - 1)(rng_);
    return g;
}

GAChromosome GeneticScheduler::randomChromosome() const
{
    GAChromosome chromosome(tasks_.size());

    // occupied[день][пара] -> индексы уже размещённых в этом слоте задач.
    QVector<QVector<QVector<int>>> occupied(
        kDays + 1, QVector<QVector<int>>(kPairs + 1));

    QVector<int> order(tasks_.size());
    std::iota(order.begin(), order.end(), 0);
    std::shuffle(order.begin(), order.end(), rng_);

    const int MAX_ATTEMPTS = 60;

    for (int taskIdx : order) {
        const Task& task = tasks_.at(taskIdx);
        bool placed = false;

        for (int attempt = 0; attempt < MAX_ATTEMPTS && !placed; ++attempt) {
            const GAGene candidate = randomGene();

            if (!teacherAvailable(task.teacherIndex, candidate.day, candidate.pairNumber))
                continue;

            bool conflict = false;
            for (int otherIdx : occupied[candidate.day][candidate.pairNumber]) {
                const Task& other = tasks_.at(otherIdx);
                const GAGene& otherGene = chromosome.at(otherIdx);
                if (otherGene.roomIndex == candidate.roomIndex ||
                    other.teacherIndex == task.teacherIndex ||
                    groupsOverlap(other.groupIndices, task.groupIndices)) {
                    conflict = true;
                    break;
                }
            }

            if (!conflict) {
                chromosome[taskIdx] = candidate;
                occupied[candidate.day][candidate.pairNumber].append(taskIdx);
                placed = true;
            }
        }

        if (!placed)
            chromosome[taskIdx] = randomGene(); // допускаем конфликт — ГА будет его устранять
    }

    return chromosome;
}

void GeneticScheduler::initializePopulation()
{
    population_.clear();
    population_.reserve(config_.populationSize);
    for (int i = 0; i < config_.populationSize; ++i)
        population_.append(randomChromosome());

    bestFitness_ = -std::numeric_limits<double>::infinity();
    bestChromosome_.clear();
}

double GeneticScheduler::evaluateChromosome(const GAChromosome& chromosome) const
{
    double penalty = 0.0;

    QVector<QVector<QVector<int>>> occupied(
        kDays + 1, QVector<QVector<int>>(kPairs + 1));

    for (int i = 0; i < chromosome.size(); ++i) {
        const GAGene& g = chromosome.at(i);
        if (g.day >= 1 && g.day <= kDays && g.pairNumber >= 1 && g.pairNumber <= kPairs)
            occupied[g.day][g.pairNumber].append(i);
        else
            penalty += 500.0; // некорректный слот (в норме такого не бывает)
    }

    for (int day = 1; day <= kDays; ++day) {
        for (int pair = 1; pair <= kPairs; ++pair) {
            const QVector<int>& slotTasks = occupied[day][pair];

            for (int a = 0; a < slotTasks.size(); ++a) {
                const int taskA = slotTasks.at(a);
                const Task& ta = tasks_.at(taskA);
                const GAGene& ga = chromosome.at(taskA);

                if (!teacherAvailable(ta.teacherIndex, day, pair))
                    penalty += 800.0;

                if (ga.roomIndex >= 0 && ga.roomIndex < rooms_.size()) {
                    if (rooms_.at(ga.roomIndex).capacity < ta.totalStudents)
                        penalty += 300.0; // не хватает вместимости аудитории
                } else {
                    penalty += 300.0;
                }

                for (int b = a + 1; b < slotTasks.size(); ++b) {
                    const int taskB = slotTasks.at(b);
                    const Task& tb = tasks_.at(taskB);
                    const GAGene& gb = chromosome.at(taskB);

                    if (ga.roomIndex == gb.roomIndex)
                        penalty += 1000.0; // конфликт аудитории
                    if (ta.teacherIndex == tb.teacherIndex)
                        penalty += 1000.0; // конфликт преподавателя
                    if (groupsOverlap(ta.groupIndices, tb.groupIndices))
                        penalty += 1000.0; // конфликт группы
                }
            }
        }
    }

    return -penalty; // 0 = идеальное решение без единого нарушения
}

void GeneticScheduler::evaluatePopulation()
{
    fitness_.resize(population_.size());

    int bestIdx = -1;
    double bestVal = -std::numeric_limits<double>::infinity();

    for (int i = 0; i < population_.size(); ++i) {
        fitness_[i] = evaluateChromosome(population_.at(i));
        if (fitness_[i] > bestVal) {
            bestVal = fitness_[i];
            bestIdx = i;
        }
    }

    if (bestIdx >= 0 && bestVal > bestFitness_) {
        bestFitness_ = bestVal;
        bestChromosome_ = population_.at(bestIdx);
    }
}

GAChromosome GeneticScheduler::tournamentSelect() const
{
    int bestIdx = -1;
    double bestVal = -std::numeric_limits<double>::infinity();

    for (int i = 0; i < config_.tournamentSize; ++i) {
        const int idx = std::uniform_int_distribution<int>(0, population_.size() - 1)(rng_);
        if (fitness_.at(idx) > bestVal) {
            bestVal = fitness_.at(idx);
            bestIdx = idx;
        }
    }
    return population_.at(bestIdx);
}

std::pair<GAChromosome, GAChromosome> GeneticScheduler::crossover(const GAChromosome& p1,
                                                                    const GAChromosome& p2) const
{
    if (prob_(rng_) > config_.crossoverRate)
        return { p1, p2 };

    GAChromosome c1, c2;
    c1.reserve(p1.size());
    c2.reserve(p2.size());

    for (int i = 0; i < p1.size(); ++i) {
        if (prob_(rng_) < 0.5) {
            c1.append(p1.at(i));
            c2.append(p2.at(i));
        } else {
            c1.append(p2.at(i));
            c2.append(p1.at(i));
        }
    }
    return { c1, c2 };
}

void GeneticScheduler::mutate(GAChromosome& chromosome) const
{
    const int MAX_ATTEMPTS = 40;

    for (int i = 0; i < chromosome.size(); ++i) {
        if (prob_(rng_) >= config_.mutationRate)
            continue;

        const Task& task = tasks_.at(i);
        bool moved = false;

        for (int attempt = 0; attempt < MAX_ATTEMPTS && !moved; ++attempt) {
            const GAGene candidate = randomGene();

            if (!teacherAvailable(task.teacherIndex, candidate.day, candidate.pairNumber))
                continue;

            bool conflict = false;
            for (int j = 0; j < chromosome.size(); ++j) {
                if (j == i) continue;
                const GAGene& other = chromosome.at(j);
                if (other.day != candidate.day || other.pairNumber != candidate.pairNumber)
                    continue;

                const Task& otherTask = tasks_.at(j);
                if (other.roomIndex == candidate.roomIndex ||
                    otherTask.teacherIndex == task.teacherIndex ||
                    groupsOverlap(otherTask.groupIndices, task.groupIndices)) {
                    conflict = true;
                    break;
                }
            }

            if (!conflict) {
                chromosome[i] = candidate;
                moved = true;
            }
        }

        if (!moved)
            chromosome[i] = randomGene();
    }
}

void GeneticScheduler::applyElitism(QVector<GAChromosome>& newPopulation) const
{
    const int eliteCount = std::max(1, int(config_.populationSize * config_.elitismRate));

    QVector<QPair<double, int>> ranked;
    ranked.reserve(population_.size());
    for (int i = 0; i < population_.size(); ++i)
        ranked.append({ fitness_.at(i), i });

    std::sort(ranked.begin(), ranked.end(), [](const QPair<double, int>& a, const QPair<double, int>& b) {
        return a.first > b.first;
    });

    for (int i = 0; i < eliteCount && i < ranked.size(); ++i)
        newPopulation.append(population_.at(ranked.at(i).second));
}

GenerationResult GeneticScheduler::buildResult(const GAChromosome& chromosome) const
{
    GenerationResult result;
    if (chromosome.isEmpty() || tasks_.isEmpty()) {
        result.success = false;
        return result;
    }

    QVector<QVector<QVector<int>>> occupied(
        kDays + 1, QVector<QVector<int>>(kPairs + 1));

    for (int i = 0; i < chromosome.size(); ++i) {
        const GAGene& g = chromosome.at(i);
        if (g.day >= 1 && g.day <= kDays && g.pairNumber >= 1 && g.pairNumber <= kPairs)
            occupied[g.day][g.pairNumber].append(i);
    }

    for (int i = 0; i < chromosome.size(); ++i) {
        const Task& task = tasks_.at(i);
        const GAGene& gene = chromosome.at(i);
        const Subject& subject = subjects_.at(task.subjectIndex);

        QStringList reasons;

        const bool slotValid = gene.day >= 1 && gene.day <= kDays &&
                                gene.pairNumber >= 1 && gene.pairNumber <= kPairs;
        if (!slotValid)
            reasons << "некорректный временной слот";

        if (!teacherAvailable(task.teacherIndex, gene.day, gene.pairNumber))
            reasons << "преподаватель недоступен в это время";

        if (gene.roomIndex < 0 || gene.roomIndex >= rooms_.size())
            reasons << "не удалось подобрать аудиторию";
        else if (rooms_.at(gene.roomIndex).capacity < task.totalStudents)
            reasons << QString("недостаточная вместимость аудитории (%1 мест, нужно %2)")
                            .arg(rooms_.at(gene.roomIndex).capacity)
                            .arg(task.totalStudents);

        if (slotValid) {
            for (int other : occupied[gene.day][gene.pairNumber]) {
                if (other == i) continue;
                const Task& otherTask = tasks_.at(other);
                const GAGene& otherGene = chromosome.at(other);

                if (gene.roomIndex >= 0 && otherGene.roomIndex == gene.roomIndex)
                    reasons << "конфликт аудитории с другим занятием";
                if (otherTask.teacherIndex == task.teacherIndex)
                    reasons << "конфликт преподавателя с другим занятием";
                if (groupsOverlap(otherTask.groupIndices, task.groupIndices))
                    reasons << "конфликт группы с другим занятием";
            }
        }

        reasons.removeDuplicates();

        if (!reasons.isEmpty()) {
            UnscheduledLesson u;
            u.subject = subject;
            u.group = task.groupIndices.isEmpty() ? Group{} : groups_.at(task.groupIndices.first());
            u.reason = reasons.join("; ");
            result.unscheduled.append(u);
            continue;
        }

        const Room& room = rooms_.at(gene.roomIndex);
        const Teacher& teacher = teachers_.at(task.teacherIndex);

        // Одна запись Lesson на каждую целевую группу — так работает и
        // StubScheduler, и фильтр по группе на вкладке "Результат".
        for (int gi : task.groupIndices) {
            Lesson lesson;
            lesson.subject = subject;
            lesson.teacher = teacher;
            lesson.room = room;
            lesson.slot.day = gene.day;
            lesson.slot.pairNumber = gene.pairNumber;
            lesson.group = groups_.at(gi);
            result.schedule.append(lesson);
        }
    }

    result.success = result.unscheduled.isEmpty();
    return result;
}

GenerationResult GeneticScheduler::generate(const QVector<Group>& groups,
                                             const QVector<Teacher>& teachers,
                                             const QVector<Room>& rooms,
                                             const QVector<Subject>& subjects)
{
    GenerationResult failedBefore;

    groups_ = groups;
    teachers_ = teachers;
    rooms_ = rooms;
    subjects_ = subjects;
    tasks_.clear();
    population_.clear();
    fitness_.clear();
    bestChromosome_.clear();
    bestFitness_ = -std::numeric_limits<double>::infinity();

    if (rooms_.isEmpty() || teachers_.isEmpty() || groups_.isEmpty()) {
        for (const Subject& s : subjects) {
            UnscheduledLesson u;
            u.subject = s;
            u.reason = "Недостаточно исходных данных (группы/преподаватели/аудитории)";
            failedBefore.unscheduled.append(u);
        }
        failedBefore.success = false;
        return failedBefore;
    }

    if (!buildTasks(subjects, failedBefore)) {
        // Ни одной задачи не удалось построить — все дисциплины уже
        // попали в failedBefore.unscheduled с конкретной причиной.
        failedBefore.success = false;
        return failedBefore;
    }

    initializePopulation();
    evaluatePopulation();

    int stagnation = 0;
    for (int generation = 0; generation < config_.maxGenerations; ++generation) {
        if (bestFitness_ >= -0.5) // фактически 0 — решение без единого нарушения
            break;

        QVector<GAChromosome> newPopulation;
        newPopulation.reserve(config_.populationSize);
        applyElitism(newPopulation);

        while (newPopulation.size() < config_.populationSize) {
            const GAChromosome p1 = tournamentSelect();
            const GAChromosome p2 = tournamentSelect();
            auto pair = crossover(p1, p2);
            mutate(pair.first);
            mutate(pair.second);
            newPopulation.append(pair.first);
            if (newPopulation.size() < config_.populationSize)
                newPopulation.append(pair.second);
        }

        population_ = std::move(newPopulation);

        const double previousBest = bestFitness_;
        evaluatePopulation();

        if (bestFitness_ > previousBest)
            stagnation = 0;
        else
            ++stagnation;

        if (stagnation >= config_.stagnationLimit)
            break;

        if (generation % 15 == 0)
            QCoreApplication::processEvents(); // не блокируем интерфейс на долгих прогонах
    }

    GenerationResult result = buildResult(bestChromosome_);
    // Дисциплины, отсеянные ещё на этапе построения задач (нет
    // преподавателя/групп) — тоже часть итогового отчёта.
    result.unscheduled.append(failedBefore.unscheduled);
    result.success = result.unscheduled.isEmpty();
    return result;
}
