#include "datamanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug> // Для вывода ошибок в консоль
#include <QTextStream>
#include <QMap>

DataManager::DataManager(QObject *parent) : QObject(parent)
{
}

// Геттеры для интерфейса
QVector<Group> DataManager::getGroups() const { return m_groups; }
QVector<Teacher> DataManager::getTeachers() const { return m_teachers; }
QVector<Room> DataManager::getRooms() const { return m_rooms; }
QVector<Subject> DataManager::getSubjects() const { return m_subjects; }

bool DataManager::loadFromJson(const QString& filePath)
{
    QFile file(filePath);

    // 1. Проверяем, существует ли файл и можем ли мы его открыть
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString errorMsg = "Не удалось открыть файл: " + filePath;
        qWarning() << errorMsg;
        emit errorOccurred(errorMsg); // Сообщаем интерфейсу об ошибке
        return false;
    }

    // 2. Читаем текст и пытаемся превратить его в JSON
    QString jsonString = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonString.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        QString errorMsg = "Ошибка формата JSON: " + parseError.errorString();
        qWarning() << errorMsg;
        emit errorOccurred(errorMsg);
        return false;
    }

    // 3. Получаем главный объект JSON
    QJsonObject rootObject = jsonDoc.object();

    // 4. Очищаем старые данные перед загрузкой новых
    m_groups.clear();
    m_teachers.clear();
    m_rooms.clear();
    m_subjects.clear();

    // --- ПАРСИНГ ГРУПП ---
    if (rootObject.contains("groups") && rootObject["groups"].isArray()) {
        QJsonArray groupsArray = rootObject["groups"].toArray();
        for (int i = 0; i < groupsArray.size(); ++i) {
            QJsonObject groupObj = groupsArray[i].toObject();
            Group newGroup;
            // Безопасное извлечение данных: если поля нет, берем значение по умолчанию
            newGroup.name = groupObj["name"].toString("Неизвестная группа");
            newGroup.studentsCount = groupObj["studentsCount"].toInt(0);
            m_groups.append(newGroup);
        }
    } else {
        qWarning() << "Поле 'groups' отсутствует или не является массивом!";
    }

    // --- ПАРСИНГ ПРЕПОДАВАТЕЛЕЙ ---
    if (rootObject.contains("teachers") && rootObject["teachers"].isArray()) {
        QJsonArray teachersArray = rootObject["teachers"].toArray();
        for (int i = 0; i < teachersArray.size(); ++i) {
            QJsonObject teacherObj = teachersArray[i].toObject();
            Teacher newTeacher;
            newTeacher.fullName = teacherObj["fullName"].toString("Неизвестный преподаватель");

            // Парсим доступность (вложенный объект)
            if (teacherObj.contains("availability") && teacherObj["availability"].isObject()) {
                QJsonObject availObj = teacherObj["availability"].toObject();
                // Проходим по всем ключам (дням недели: "1", "2" и т.д.)
                for (const QString& dayKey : availObj.keys()) {
                    int day = dayKey.toInt();
                    QVector<int> pairs;
                    // Получаем массив пар для этого дня
                    if (availObj[dayKey].isArray()) {
                        QJsonArray pairsArray = availObj[dayKey].toArray();
                        for (int j = 0; j < pairsArray.size(); ++j) {
                            pairs.append(pairsArray[j].toInt());
                        }
                    }
                    // Сохраняем в нашу карту QMap
                    newTeacher.availability.insert(day, pairs);
                }
            }

            m_teachers.append(newTeacher);
        }
    } else {
        qWarning() << "Поле 'teachers' отсутствует или не является массивом!";
    }

    // --- ПАРСИНГ АУДИТОРИЙ ---
    if (rootObject.contains("rooms") && rootObject["rooms"].isArray()) {
        QJsonArray roomsArray = rootObject["rooms"].toArray();
        for (int i = 0; i < roomsArray.size(); ++i) {
            QJsonObject roomObj = roomsArray[i].toObject();
            Room newRoom;
            newRoom.number = roomObj["number"].toString("Неизвестно");
            newRoom.type = roomObj["type"].toString("Общая");
            newRoom.capacity = roomObj["capacity"].toInt(0);
            m_rooms.append(newRoom);
        }
    } else {
        qWarning() << "Поле 'rooms' отсутствует или не является массивом!";
    }

    // --- ПАРСИНГ ДИСЦИПЛИН ---
    if (rootObject.contains("subjects") && rootObject["subjects"].isArray()) {
        QJsonArray subjectsArray = rootObject["subjects"].toArray();
        for (int i = 0; i < subjectsArray.size(); ++i) {
            QJsonObject subjObj = subjectsArray[i].toObject();
            Subject newSubject;
            newSubject.name = subjObj["name"].toString("Неизвестная дисциплина");
            newSubject.type = subjObj["type"].toString("Лекция");
            newSubject.hours = subjObj["hours"].toInt(0);
            newSubject.teacherId = subjObj["teacherId"].toString("");

            // Парсим список целевых групп для этой дисциплины (вложенный массив строк)
            if (subjObj.contains("targetGroups") && subjObj["targetGroups"].isArray()) {
                QJsonArray groupsArray = subjObj["targetGroups"].toArray();
                for (int j = 0; j < groupsArray.size(); ++j) {
                    newSubject.targetGroups.append(groupsArray[j].toString());
                }
            }

            m_subjects.append(newSubject);
        }
    } else {
        qWarning() << "Поле 'subjects' отсутствует или не является массивом!";
    }

    // Оповещаем интерфейс, что данные успешно загружены
    emit dataUpdated();
    return true;
}

bool DataManager::exportScheduleToCsv(const QString& filePath, const QVector<Lesson>& schedule)
{
    QFile file(filePath);
    // Открываем файл для записи. Флаг Truncate очистит файл, если он уже существовал
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QString errorMsg = "Не удалось создать CSV файл: " + filePath;
        qWarning() << errorMsg;
        emit errorOccurred(errorMsg);
        return false;
    }

    // Создаем текстовый поток для удобной записи
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // Excel в русскоязычной локали часто использует точку с запятой (;) как разделитель колонок
    const QString separator = ";";

    // Записываем заголовки таблицы (первая строка)
    out << "День" << separator
        << "Пара" << separator
        << "Группа" << separator
        << "Дисциплина" << separator
        << "Преподаватель" << separator
        << "Аудитория" << "\n";

    // Проходим по всему сгенерированному расписанию и записываем строки
    for (const Lesson& lesson : schedule) {
        out << lesson.slot.day << separator
            << lesson.slot.pairNumber << separator
            << lesson.group.name << separator
            << lesson.subject.name << separator
            << lesson.teacher.fullName << separator
            << lesson.room.number << "\n";
    }

    file.close();
    return true;
}

bool DataManager::exportScheduleToJson(const QString& filePath, const QVector<Lesson>& schedule)
{
    QFile file(filePath);
    // Открываем файл для записи
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QString errorMsg = "Не удалось создать JSON файл расписания: " + filePath;
        qWarning() << errorMsg;
        emit errorOccurred(errorMsg);
        return false;
    }

    QJsonArray scheduleArray;
    // Проходим по всему расписанию и упаковываем каждое занятие в JSON-объект
    for (const Lesson& lesson : schedule) {
        QJsonObject lessonObj;
        lessonObj["day"] = lesson.slot.day;
        lessonObj["pair"] = lesson.slot.pairNumber;
        lessonObj["group"] = lesson.group.name;
        lessonObj["subject"] = lesson.subject.name;
        lessonObj["teacher"] = lesson.teacher.fullName;
        lessonObj["room"] = lesson.room.number;
        scheduleArray.append(lessonObj);
    }

    // Создаем корневой объект документа
    QJsonObject rootObject;
    rootObject["schedule"] = scheduleArray;

    // Превращаем наши объекты в текстовый JSON-формат и записываем в файл
    QJsonDocument jsonDoc(rootObject);
    file.write(jsonDoc.toJson());
    file.close();
    return true;
}

bool DataManager::exportStatisticsToJson(const QString& filePath, const QVector<Lesson>& schedule)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QString errorMsg = "Не удалось создать JSON файл статистики: " + filePath;
        qWarning() << errorMsg;
        emit errorOccurred(errorMsg);
        return false;
    }

    // Словари для подсчета нагрузки (ключ - имя/номер, значение - количество пар)
    QMap<QString, int> teacherLoad;
    QMap<QString, int> groupLoad;
    QMap<QString, int> roomLoad;

    for (const Lesson& lesson : schedule) {
        teacherLoad[lesson.teacher.fullName]++;
        groupLoad[lesson.group.name]++;
        roomLoad[lesson.room.number]++;
    }

    QJsonObject rootObject;

    QJsonObject teachersObj;
    for (auto it = teacherLoad.constBegin(); it != teacherLoad.constEnd(); ++it)
        teachersObj[it.key()] = it.value();
    rootObject["teacherLoad"] = teachersObj;

    QJsonObject groupsObj;
    for (auto it = groupLoad.constBegin(); it != groupLoad.constEnd(); ++it)
        groupsObj[it.key()] = it.value();
    rootObject["groupLoad"] = groupsObj;

    QJsonObject roomsObj;
    for (auto it = roomLoad.constBegin(); it != roomLoad.constEnd(); ++it)
        roomsObj[it.key()] = it.value();
    rootObject["roomLoad"] = roomsObj;

    QJsonDocument jsonDoc(rootObject);
    file.write(jsonDoc.toJson());
    file.close();
    return true;
}

// =====================  CRUD: Группы  =====================

void DataManager::setGroups(const QVector<Group>& groups)
{
    m_groups = groups;
    emit dataUpdated();
}

void DataManager::addGroup(const Group& group)
{
    m_groups.append(group);
    emit dataUpdated();
}

void DataManager::updateGroup(int index, const Group& group)
{
    if (index < 0 || index >= m_groups.size()) return;
    m_groups[index] = group;
    emit dataUpdated();
}

void DataManager::removeGroup(int index)
{
    if (index < 0 || index >= m_groups.size()) return;
    m_groups.removeAt(index);
    emit dataUpdated();
}

// =====================  CRUD: Преподаватели  =====================

void DataManager::setTeachers(const QVector<Teacher>& teachers)
{
    m_teachers = teachers;
    emit dataUpdated();
}

void DataManager::addTeacher(const Teacher& teacher)
{
    m_teachers.append(teacher);
    emit dataUpdated();
}

void DataManager::updateTeacher(int index, const Teacher& teacher)
{
    if (index < 0 || index >= m_teachers.size()) return;
    m_teachers[index] = teacher;
    emit dataUpdated();
}

void DataManager::removeTeacher(int index)
{
    if (index < 0 || index >= m_teachers.size()) return;
    m_teachers.removeAt(index);
    emit dataUpdated();
}

// =====================  CRUD: Аудитории  =====================

void DataManager::setRooms(const QVector<Room>& rooms)
{
    m_rooms = rooms;
    emit dataUpdated();
}

void DataManager::addRoom(const Room& room)
{
    m_rooms.append(room);
    emit dataUpdated();
}

void DataManager::updateRoom(int index, const Room& room)
{
    if (index < 0 || index >= m_rooms.size()) return;
    m_rooms[index] = room;
    emit dataUpdated();
}

void DataManager::removeRoom(int index)
{
    if (index < 0 || index >= m_rooms.size()) return;
    m_rooms.removeAt(index);
    emit dataUpdated();
}

// =====================  CRUD: Дисциплины  =====================

void DataManager::setSubjects(const QVector<Subject>& subjects)
{
    m_subjects = subjects;
    emit dataUpdated();
}

void DataManager::addSubject(const Subject& subject)
{
    m_subjects.append(subject);
    emit dataUpdated();
}

void DataManager::updateSubject(int index, const Subject& subject)
{
    if (index < 0 || index >= m_subjects.size()) return;
    m_subjects[index] = subject;
    emit dataUpdated();
}

void DataManager::removeSubject(int index)
{
    if (index < 0 || index >= m_subjects.size()) return;
    m_subjects.removeAt(index);
    emit dataUpdated();
}
