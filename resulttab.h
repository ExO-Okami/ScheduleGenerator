#ifndef RESULTTAB_H
#define RESULTTAB_H

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QLabel>
#include <QTextEdit>
#include "datamanager.h"
#include "heatmapwidget.h"
#include "barchartwidget.h"

// Вкладка "Результат": отображение сгенерированного расписания и его
// визуализация (таблица, тепловая карта загрузки, статистика и
// диаграмма загрузки преподавателей), а также экспорт результатов.
class ResultTab : public QWidget
{
    Q_OBJECT
public:
    explicit ResultTab(DataManager* dataManager, QWidget* parent = nullptr);

public slots:
    void setSchedule(const QVector<Lesson>& schedule);

private slots:
    void onGroupFilterChanged(int index);
    void onExportCsv();
    void onExportJson();
    void onExportSchedulePng();
    void onExportHeatmapPng();

private:
    DataManager* m_dataManager;
    QVector<Lesson> m_schedule;

    QComboBox* m_groupFilterCombo;
    QTableWidget* m_scheduleTable;
    HeatmapWidget* m_heatmapWidget;
    BarChartWidget* m_barChartWidget;
    QTextEdit* m_statsView;

    static const int kPairsCount = 6;

    void rebuildGroupFilter();
    void refreshScheduleTable();
    void refreshStatistics();
};

#endif // RESULTTAB_H
