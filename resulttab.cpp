#include "resulttab.h"
#include "scheduleutils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QPushButton>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QMap>

ResultTab::ResultTab(DataManager* dataManager, QWidget* parent)
    : QWidget(parent), m_dataManager(dataManager)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // --- Верхняя панель: фильтр по группе и экспорт ---
    QHBoxLayout* topBar = new QHBoxLayout();
    topBar->addWidget(new QLabel("Фильтр по группе:"));
    m_groupFilterCombo = new QComboBox(this);
    connect(m_groupFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ResultTab::onGroupFilterChanged);
    topBar->addWidget(m_groupFilterCombo);
    topBar->addStretch();

    QPushButton* exportCsvBtn = new QPushButton("Экспорт CSV");
    QPushButton* exportJsonBtn = new QPushButton("Экспорт JSON");
    QPushButton* exportSchedulePngBtn = new QPushButton("PNG таблицы");
    QPushButton* exportHeatmapPngBtn = new QPushButton("PNG тепловой карты");
    connect(exportCsvBtn, &QPushButton::clicked, this, &ResultTab::onExportCsv);
    connect(exportJsonBtn, &QPushButton::clicked, this, &ResultTab::onExportJson);
    connect(exportSchedulePngBtn, &QPushButton::clicked, this, &ResultTab::onExportSchedulePng);
    connect(exportHeatmapPngBtn, &QPushButton::clicked, this, &ResultTab::onExportHeatmapPng);
    topBar->addWidget(exportCsvBtn);
    topBar->addWidget(exportJsonBtn);
    topBar->addWidget(exportSchedulePngBtn);
    topBar->addWidget(exportHeatmapPngBtn);
    mainLayout->addLayout(topBar);

    // --- Таблица расписания ---
    m_scheduleTable = new QTableWidget(kPairsCount, 6, this);
    QStringList headers;
    for (int d = 1; d <= 6; ++d) headers << ScheduleUtils::dayName(d);
    m_scheduleTable->setHorizontalHeaderLabels(headers);

    QStringList vHeaders;
    for (int p = 1; p <= kPairsCount; ++p) vHeaders << QString("Пара %1").arg(p);
    m_scheduleTable->setVerticalHeaderLabels(vHeaders);

    m_scheduleTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_scheduleTable->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_scheduleTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_scheduleTable->setMinimumHeight(260);
    mainLayout->addWidget(m_scheduleTable, 3);

    // --- Нижняя зона: тепловая карта, статистика, диаграмма ---
    QSplitter* bottomSplitter = new QSplitter(Qt::Horizontal, this);

    QGroupBox* heatmapBox = new QGroupBox("Тепловая карта загрузки", this);
    QVBoxLayout* heatmapLayout = new QVBoxLayout(heatmapBox);
    m_heatmapWidget = new HeatmapWidget(heatmapBox);
    heatmapLayout->addWidget(m_heatmapWidget);
    bottomSplitter->addWidget(heatmapBox);

    QGroupBox* statsBox = new QGroupBox("Статистика", this);
    QVBoxLayout* statsLayout = new QVBoxLayout(statsBox);
    m_statsView = new QTextEdit(statsBox);
    m_statsView->setReadOnly(true);
    statsLayout->addWidget(m_statsView);
    bottomSplitter->addWidget(statsBox);

    QGroupBox* chartBox = new QGroupBox("Загрузка преподавателей", this);
    QVBoxLayout* chartLayout = new QVBoxLayout(chartBox);
    m_barChartWidget = new BarChartWidget(chartBox);
    chartLayout->addWidget(m_barChartWidget);
    bottomSplitter->addWidget(chartBox);

    mainLayout->addWidget(bottomSplitter, 2);

    connect(m_dataManager, &DataManager::dataUpdated, this, &ResultTab::rebuildGroupFilter);
    rebuildGroupFilter();
}

void ResultTab::setSchedule(const QVector<Lesson>& schedule)
{
    m_schedule = schedule;
    rebuildGroupFilter();
    refreshScheduleTable();
    refreshStatistics();
    m_heatmapWidget->setSchedule(m_schedule, kPairsCount);
}

void ResultTab::rebuildGroupFilter()
{
    const QString previous = m_groupFilterCombo->currentText();
    m_groupFilterCombo->blockSignals(true);
    m_groupFilterCombo->clear();
    m_groupFilterCombo->addItem("Все группы");

    QVector<Group> groups = m_dataManager->getGroups();
    for (const Group& g : groups)
        m_groupFilterCombo->addItem(g.name);

    const int idx = m_groupFilterCombo->findText(previous);
    m_groupFilterCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    m_groupFilterCombo->blockSignals(false);
}

void ResultTab::onGroupFilterChanged(int /*index*/)
{
    refreshScheduleTable();
}

void ResultTab::refreshScheduleTable()
{
    m_scheduleTable->clearContents();

    const QString filterGroup = m_groupFilterCombo->currentText();
    const bool showAll = (filterGroup.isEmpty() || filterGroup == "Все группы");

    for (const Lesson& lesson : m_schedule) {
        if (!showAll && lesson.group.name != filterGroup)
            continue;

        const int row = lesson.slot.pairNumber - 1;
        const int col = lesson.slot.day - 1;
        if (row < 0 || row >= kPairsCount || col < 0 || col >= 6)
            continue;

        QTableWidgetItem* existing = m_scheduleTable->item(row, col);
        QString text = QString("%1\n%2\n%3 / %4")
                            .arg(lesson.subject.name)
                            .arg(lesson.group.name)
                            .arg(lesson.teacher.fullName)
                            .arg(lesson.room.number);

        if (existing) {
            // В ячейке уже есть занятие (например, общая лекция для нескольких групп при "Все группы") —
            // добавляем через разделитель, чтобы не потерять информацию.
            existing->setText(existing->text() + "\n---\n" + text);
        } else {
            QTableWidgetItem* item = new QTableWidgetItem(text);
            item->setBackground(ScheduleUtils::colorForSubject(lesson.subject.name));
            item->setTextAlignment(Qt::AlignCenter);
            m_scheduleTable->setItem(row, col, item);
        }
    }
}

void ResultTab::refreshStatistics()
{
    QMap<QString, int> teacherLoad;
    QMap<QString, int> groupLoad;
    QMap<int, int> dayLoad;

    for (const Lesson& lesson : m_schedule) {
        teacherLoad[lesson.teacher.fullName]++;
        groupLoad[lesson.group.name]++;
        dayLoad[lesson.slot.day]++;
    }

    int busiestDay = -1;
    int busiestDayCount = -1;
    for (auto it = dayLoad.constBegin(); it != dayLoad.constEnd(); ++it) {
        if (it.value() > busiestDayCount) {
            busiestDayCount = it.value();
            busiestDay = it.key();
        }
    }

    QString text;
    text += QString("Общее количество занятий: %1\n\n").arg(m_schedule.size());

    text += "Загрузка преподавателей:\n";
    for (auto it = teacherLoad.constBegin(); it != teacherLoad.constEnd(); ++it)
        text += QString("  %1 — %2 пар\n").arg(it.key()).arg(it.value());

    text += "\nЗагрузка групп:\n";
    for (auto it = groupLoad.constBegin(); it != groupLoad.constEnd(); ++it)
        text += QString("  %1 — %2 пар\n").arg(it.key()).arg(it.value());

    text += "\n";
    if (busiestDay > 0)
        text += QString("Самый загруженный день: %1 (%2 пар)\n")
                    .arg(ScheduleUtils::dayName(busiestDay))
                    .arg(busiestDayCount);
    else
        text += "Самый загруженный день: нет данных\n";

    m_statsView->setPlainText(text);
    m_barChartWidget->setData(teacherLoad, "Загрузка преподавателей (пар в неделю)");
}

void ResultTab::onExportCsv()
{
    if (m_schedule.isEmpty()) {
        QMessageBox::information(this, "Экспорт", "Сначала сгенерируйте расписание.");
        return;
    }
    const QString filePath = QFileDialog::getSaveFileName(this, "Сохранить расписание как CSV",
                                                            "schedule.csv", "CSV файлы (*.csv)");
    if (filePath.isEmpty()) return;

    if (m_dataManager->exportScheduleToCsv(filePath, m_schedule))
        QMessageBox::information(this, "Экспорт", "Расписание сохранено в CSV.");
}

void ResultTab::onExportJson()
{
    if (m_schedule.isEmpty()) {
        QMessageBox::information(this, "Экспорт", "Сначала сгенерируйте расписание.");
        return;
    }
    const QString filePath = QFileDialog::getSaveFileName(this, "Сохранить расписание как JSON",
                                                            "schedule.json", "JSON файлы (*.json)");
    if (filePath.isEmpty()) return;

    if (m_dataManager->exportScheduleToJson(filePath, m_schedule))
        QMessageBox::information(this, "Экспорт", "Расписание сохранено в JSON.");
}

void ResultTab::onExportSchedulePng()
{
    const QString filePath = QFileDialog::getSaveFileName(this, "Сохранить таблицу расписания как PNG",
                                                            "schedule.png", "PNG файлы (*.png)");
    if (filePath.isEmpty()) return;

    QPixmap pixmap = m_scheduleTable->grab();
    if (pixmap.save(filePath, "PNG"))
        QMessageBox::information(this, "Экспорт", "Таблица расписания сохранена как PNG.");
    else
        QMessageBox::warning(this, "Экспорт", "Не удалось сохранить PNG файл.");
}

void ResultTab::onExportHeatmapPng()
{
    const QString filePath = QFileDialog::getSaveFileName(this, "Сохранить тепловую карту как PNG",
                                                            "heatmap.png", "PNG файлы (*.png)");
    if (filePath.isEmpty()) return;

    if (m_heatmapWidget->saveToPng(filePath))
        QMessageBox::information(this, "Экспорт", "Тепловая карта сохранена как PNG.");
    else
        QMessageBox::warning(this, "Экспорт", "Не удалось сохранить PNG файл.");
}
