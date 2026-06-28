#ifndef MODELS_H
#define MODELS_H

#include <QString>
#include <QVector>
#include <QMap>

// Временной слот [cite: 45]
struct Slot {
    int day;        // День недели (например, 1 - ПН, 6 - СБ)
    int pairNumber; // Номер пары (1-6)
};

// Учебная группа [cite: 41]
struct Group {
    QString name;
    int studentsCount;
};

// Преподаватель [cite: 42]
struct Teacher {
    QString fullName;
    // Доступность: ключ - день недели (1-6), значение - список доступных пар
    QMap<int, QVector<int>> availability;
};

// Аудитория [cite: 43]
struct Room {
    QString number;
    QString type;       // "Лекционная", "Практическая", "Компьютерный класс"
    int capacity;
};

// Дисциплина [cite: 44]
struct Subject {
    QString name;
    QString type;       // "Лекция", "Практика"
    int hours;          // Количество часов
    QString teacherId;  // Связь с преподавателем (лучше по имени или ID)
    QVector<QString> targetGroups; // Список названий групп
};

// Назначенное занятие (результат работы алгоритма) [cite: 46]
struct Lesson {
    Subject subject;
    Group group;
    Teacher teacher;
    Room room;
    Slot slot;
};

#endif // MODELS_H