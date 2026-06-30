#ifndef SCHEDULEUTILS_H
#define SCHEDULEUTILS_H

#include <QString>
#include <QStringList>
#include <QColor>
#include <QVector>

// Общие вспомогательные функции для отображения расписания (дни недели,
// цветовая палитра дисциплин, цвета тепловой карты загрузки).
namespace ScheduleUtils {

inline QStringList weekDayNames()
{
    return { "Понедельник", "Вторник", "Среда", "Четверг", "Пятница", "Суббота" };
}

inline QString dayName(int day)
{
    static const QStringList names = weekDayNames();
    if (day >= 1 && day <= names.size())
        return names.at(day - 1);
    return QString("День %1").arg(day);
}

inline QString dayShortName(int day)
{
    static const QStringList names = { "ПН", "ВТ", "СР", "ЧТ", "ПТ", "СБ" };
    if (day >= 1 && day <= names.size())
        return names.at(day - 1);
    return QString::number(day);
}

// Палитра для цветового выделения ячеек по названию дисциплины.
inline QColor colorForSubject(const QString& subjectName)
{
    static const QVector<QColor> palette = {
        QColor("#AED9E0"), QColor("#FFD3B6"), QColor("#FFAAA5"),
        QColor("#D5E8D4"), QColor("#E1D5E7"), QColor("#FFF2CC"),
        QColor("#DAE8FC"), QColor("#F8CECC"), QColor("#D4E6F1"),
        QColor("#FCE4D6")
    };
    uint h = qHash(subjectName);
    return palette.at(int(h % uint(palette.size())));
}

// Цвет тепловой карты по числу занятий в слоте относительно максимума:
// зелёный -> жёлтый -> оранжевый -> красный.
inline QColor heatColor(int count, int maxCount)
{
    if (count <= 0)
        return QColor("#F0F0F0");
    if (maxCount <= 0)
        maxCount = 1;

    const double ratio = double(count) / double(maxCount);
    if (ratio <= 0.33) return QColor("#8BC34A"); // зелёный
    if (ratio <= 0.66) return QColor("#FFEB3B"); // жёлтый
    if (ratio <= 0.90) return QColor("#FF9800"); // оранжевый
    return QColor("#F44336");                     // красный
}

} // namespace ScheduleUtils

#endif // SCHEDULEUTILS_H
