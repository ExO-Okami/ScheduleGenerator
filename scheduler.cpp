#include "scheduler.h"
#include <QMap>
#include <QSet>
#include <QPair>
#include <algorithm>

GenerationResult StubScheduler::generate(const QVector<Group>& groups,
                                          const QVector<Teacher>& teachers,
                                          const QVector<Room>& rooms,
                                          const QVector<Subject>& subjects)
{
    GenerationResult result;

    // Быстрый поиск преподавателя по ФИО (teacherId хранит ФИО, см. test_data.json)
    QMap<QString, Teacher> teacherByName;
    for (const Teacher& t : teachers)
        teacherByName.insert(t.fullName, t);

    using SlotKey = QPair<int, int>; // (день, пара)

    // Занятость преподавателей / аудиторий / групп по слотам
    QMap<QString, QSet<SlotKey>> teacherBusy;
    QMap<QString, QSet<SlotKey>> roomBusy;
    QMap<QString, QSet<SlotKey>> groupBusy;

    // Декомпозиция дисциплин на отдельные занятия согласно недельной нагрузке
    struct Task
    {
        Subject subject;
        int availableSlots;
    };
    QVector<Task> tasks;

    for (const Subject& subj : subjects) {
        if (!teacherByName.contains(subj.teacherId)) {
            UnscheduledLesson u;
            u.subject = subj;
            u.reason = "Не найден преподаватель \"" + subj.teacherId + "\" для дисциплины";
            result.unscheduled.append(u);
            continue;
        }

        const Teacher& teacher = teacherByName.value(subj.teacherId);
        int availableSlots = 0;
        for (auto it = teacher.availability.constBegin(); it != teacher.availability.constEnd(); ++it)
            availableSlots += it.value().size();

        // 1 пара = 2 академических часа
        const int lessonsCount = qMax(1, (subj.hours + 1) / 2);
        for (int i = 0; i < lessonsCount; ++i) {
            Task t;
            t.subject = subj;
            t.availableSlots = availableSlots;
            tasks.append(t);
        }
    }

    // Сортировка занятий по возрастанию числа допустимых временных слотов
    // (принцип наиболее ограниченной переменной)
    std::sort(tasks.begin(), tasks.end(), [](const Task& a, const Task& b) {
        return a.availableSlots < b.availableSlots;
    });

    for (const Task& task : tasks) {
        const Subject& subj = task.subject;
        const Teacher& teacher = teacherByName.value(subj.teacherId);

        // Собираем целевые группы и суммарное количество студентов
        QVector<Group> taskGroups;
        int totalStudents = 0;
        for (const QString& gname : subj.targetGroups) {
            for (const Group& g : groups) {
                if (g.name == gname) {
                    taskGroups.append(g);
                    totalStudents += g.studentsCount;
                    break;
                }
            }
        }

        bool assigned = false;

        for (auto dayIt = teacher.availability.constBegin();
             dayIt != teacher.availability.constEnd() && !assigned; ++dayIt) {

            const int day = dayIt.key();
            for (int pair : dayIt.value()) {
                const SlotKey slotKey(day, pair);

                if (teacherBusy[subj.teacherId].contains(slotKey))
                    continue;

                bool groupConflict = false;
                for (const Group& g : taskGroups) {
                    if (groupBusy[g.name].contains(slotKey)) {
                        groupConflict = true;
                        break;
                    }
                }
                if (groupConflict)
                    continue;

                // Назначение каждого занятия на первый допустимый слот с
                // одновременной проверкой всех ограничений: ищем свободную
                // аудиторию подходящей вместимости.
                const Room* chosenRoom = nullptr;
                for (const Room& r : rooms) {
                    if (r.capacity < totalStudents)
                        continue;
                    if (roomBusy[r.number].contains(slotKey))
                        continue;
                    chosenRoom = &r;
                    break;
                }
                if (!chosenRoom)
                    continue;

                // Фиксация назначения в структуре расписания: для каждой
                // целевой группы создаём отдельную запись Lesson, чтобы
                // фильтр по группе в таблице результата работал корректно.
                for (const Group& g : taskGroups) {
                    Lesson lesson;
                    lesson.subject = subj;
                    lesson.teacher = teacher;
                    lesson.room = *chosenRoom;
                    lesson.slot.day = day;
                    lesson.slot.pairNumber = pair;
                    lesson.group = g;
                    result.schedule.append(lesson);
                    groupBusy[g.name].insert(slotKey);
                }

                teacherBusy[subj.teacherId].insert(slotKey);
                roomBusy[chosenRoom->number].insert(slotKey);
                assigned = true;
                break;
            }
        }

        if (!assigned) {
            UnscheduledLesson u;
            u.subject = subj;
            u.group = taskGroups.isEmpty() ? Group{} : taskGroups.first();
            u.reason = "Не найден свободный слот и/или аудитория без конфликтов";
            result.unscheduled.append(u);
        }
    }

    result.success = result.unscheduled.isEmpty();
    return result;
}
