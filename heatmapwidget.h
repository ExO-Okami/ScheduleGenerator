#ifndef HEATMAPWIDGET_H
#define HEATMAPWIDGET_H

#include <QWidget>
#include <QVector>
#include "models.h"

// Кастомный виджет тепловой карты загрузки расписания.
// Оси: дни недели (столбцы) и пары (строки).
// Цвет ячейки зависит от количества занятий в данном слоте.
class HeatmapWidget : public QWidget
{
    Q_OBJECT
public:
    explicit HeatmapWidget(QWidget* parent = nullptr);

    void setSchedule(const QVector<Lesson>& schedule, int pairsCount = 6);
    bool saveToPng(const QString& filePath);

protected:
    void paintEvent(QPaintEvent* event) override;
    QSize minimumSizeHint() const override;

private:
    QVector<Lesson> m_schedule;
    int m_pairsCount = 6;
    int m_grid[7][7]; // индексы 1..6 используются, [день][пара]

    void recalcGrid();
};

#endif // HEATMAPWIDGET_H
