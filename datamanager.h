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
    QVector<Room> getRooms() const;
    QVector<Subject> getSubjects() const;

    // API для загрузки данных
    bool loadFromJson(const QString& filePath);

    // API для экспорта данных
    bool exportScheduleToCsv(const QString& filePath, const QVector<Lesson>& schedule);
    bool exportScheduleToJson(const QString& filePath, const QVector<Lesson>& schedule);
    bool exportStatisticsToJson(const QString& filePath, const QVector<Lesson>& schedule);

    // --- CRUD API для редактирования данных из интерфейса (вкладка "Данные") ---

    void setGroups(const QVector<Group>& groups);
    void addGroup(const Group& group);
    void updateGroup(int index, const Group& group);
    void removeGroup(int index);

    void setTeachers(const QVector<Teacher>& teachers);
    void addTeacher(const Teacher& teacher);
    void updateTeacher(int index, const Teacher& teacher);
    void removeTeacher(int index);

    void setRooms(const QVector<Room>& rooms);
    void addRoom(const Room& room);
    void updateRoom(int index, const Room& room);
    void removeRoom(int index);

    void setSubjects(const QVector<Subject>& subjects);
    void addSubject(const Subject& subject);
    void updateSubject(int index, const Subject& subject);
    void removeSubject(int index);

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
