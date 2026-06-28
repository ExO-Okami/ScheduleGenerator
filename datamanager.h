#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include <QObject>
#include <QVector>
#include "models.h"

class DataManager : public QObject
{
    Q_OBJECT

public:
    explicit DataManager(QObject *parent = nullptr);

    // API для получения данных (Геттеры)
    QVector<Group> getGroups() const;
    QVector<Teacher> getTeachers() const;

    // API для загрузки данных
    bool loadFromJson(const QString& filePath);
    // API для экспорта данных
    bool exportScheduleToCsv(const QString& filePath, const QVector<Lesson>& schedule);
    // Экспорт расписания в JSON
    bool exportScheduleToJson(const QString& filePath, const QVector<Lesson>& schedule);

    // Экспорт статистики (пока сделаем каркас)
    bool exportStatisticsToJson(const QString& filePath);

signals:
    // Сигналы для оповещения интерфейса об ошибках и обновлениях
    void dataUpdated();
    void errorOccurred(const QString& errorMessage);

private:
    // Сами коллекции данных (переменные)
    QVector<Group> m_groups;
    QVector<Teacher> m_teachers;
    QVector<Room> m_rooms;
    QVector<Subject> m_subjects;
};

#endif // DATAMANAGER_H