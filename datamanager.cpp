#include "datamanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug> // Для вывода ошибок в консоль
#include <QTextStream>
DataManager::DataManager(QObject *parent) : QObject(parent)
{
}

// Геттеры для интерфейса
QVector<Group> DataManager::getGroups() const { return m_groups; }
QVector<Teacher> DataManager::getTeachers() const { return m_teachers; }

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

    // TODO: Добавить парсинг преподавателей (m_teachers)
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
    // Оповещаем интерфейс (Александра), что данные успешно загружены
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

            // Заполняем поля объекта
            lessonObj["day"] = lesson.slot.day;
            lessonObj["pair"] = lesson.slot.pairNumber;
            lessonObj["group"] = lesson.group.name;
            lessonObj["subject"] = lesson.subject.name;
            lessonObj["teacher"] = lesson.teacher.fullName;
            lessonObj["room"] = lesson.room.number;

            // Добавляем готовый объект занятия в массив
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