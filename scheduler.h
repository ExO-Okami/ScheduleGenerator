#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <QVector>
#include <QString>
#include "models.h"

// Незаназначенное занятие с причиной отказа (для отображения ошибок генерации).
struct UnscheduledLesson
{
    Subject subject;
    Group group;
    QString reason;
};

// Результат работы генератора расписания.
struct GenerationResult
{
    QVector<Lesson> schedule;
    QVector<UnscheduledLesson> unscheduled;
    bool success = false;
};

// Интерфейс генератора расписания. Реальный алгоритм (см. "Метод решения" в
// техническом описании проекта) реализуется разработчиком алгоритма как
// отдельный класс, реализующий этот интерфейс. Вкладка "Генерация" работает
// с любым IScheduler, поэтому подключение настоящего алгоритма не потребует
// изменений в интерфейсе — достаточно передать в MainWindow другой экземпляр.
class IScheduler
{
public:
    virtual ~IScheduler() = default;
    virtual GenerationResult generate(const QVector<Group>& groups,
                                       const QVector<Teacher>& teachers,
                                       const QVector<Room>& rooms,
                                       const QVector<Subject>& subjects) = 0;
};

// ВРЕМЕННАЯ ЗАГЛУШКА. Реализует упрощённую версию жадного алгоритма с
// сортировкой по принципу "наиболее ограниченной переменной" (см. техническое
// описание), чтобы вкладка "Генерация" и "Результат" были полностью
// работоспособны уже сейчас. Подлежит замене/доработке разработчиком
// алгоритма на полноценную реализацию.
class StubScheduler : public IScheduler
{
public:
    GenerationResult generate(const QVector<Group>& groups,
                               const QVector<Teacher>& teachers,
                               const QVector<Room>& rooms,
                               const QVector<Subject>& subjects) override;
};

#endif // SCHEDULER_H
