#include "heatmapwidget.h"
#include "scheduleutils.h"
#include <QPainter>
#include <QPixmap>
#include <algorithm>
#include <cstring>

HeatmapWidget::HeatmapWidget(QWidget* parent) : QWidget(parent)
{
    std::memset(m_grid, 0, sizeof(m_grid));
    setMinimumSize(420, 320);
}

void HeatmapWidget::setSchedule(const QVector<Lesson>& schedule, int pairsCount)
{
    m_schedule = schedule;
    m_pairsCount = pairsCount;
    recalcGrid();
    update();
}

void HeatmapWidget::recalcGrid()
{
    std::memset(m_grid, 0, sizeof(m_grid));
    for (const Lesson& l : m_schedule) {
        const int day = l.slot.day;
        const int pair = l.slot.pairNumber;
        if (day >= 1 && day <= 6 && pair >= 1 && pair <= 6)
            m_grid[day][pair]++;
    }
}

QSize HeatmapWidget::minimumSizeHint() const { return QSize(420, 320); }

void HeatmapWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const int days = 6;
    const int pairs = m_pairsCount;
    const int legendHeight = 40;
    const int leftMargin = 70;
    const int topMargin = 30;

    const int gridW = width() - leftMargin - 10;
    const int gridH = height() - topMargin - legendHeight - 10;
    if (gridW <= 0 || gridH <= 0)
        return;

    const double cellW = double(gridW) / days;
    const double cellH = double(gridH) / pairs;

    int maxCount = 1;
    for (int d = 1; d <= days; ++d)
        for (int p = 1; p <= pairs; ++p)
            maxCount = std::max(maxCount, m_grid[d][p]);

    // Заголовки дней недели
    QFont headerFont = painter.font();
    headerFont.setBold(true);
    painter.setFont(headerFont);
    painter.setPen(palette().color(QPalette::WindowText));
    for (int d = 1; d <= days; ++d) {
        QRectF r(leftMargin + (d - 1) * cellW, 0, cellW, topMargin);
        painter.drawText(r, Qt::AlignCenter, ScheduleUtils::dayShortName(d));
    }

    QFont normalFont = painter.font();
    normalFont.setBold(false);
    painter.setFont(normalFont);

    // Сетка с подписями пар
    for (int p = 1; p <= pairs; ++p) {
        QRectF labelRect(0, topMargin + (p - 1) * cellH, leftMargin - 6, cellH);
        painter.drawText(labelRect, Qt::AlignRight | Qt::AlignVCenter, QString("Пара %1").arg(p));

        for (int d = 1; d <= days; ++d) {
            QRectF cell(leftMargin + (d - 1) * cellW, topMargin + (p - 1) * cellH, cellW, cellH);
            const int count = m_grid[d][p];
            const QColor color = ScheduleUtils::heatColor(count, maxCount);

            painter.fillRect(cell.adjusted(1, 1, -1, -1), color);
            painter.setPen(QColor(90, 90, 90));
            painter.drawRect(cell);

            if (count > 0) {
                painter.setPen(Qt::black);
                painter.drawText(cell, Qt::AlignCenter, QString::number(count));
            }
        }
    }

    // Легенда
    const int legendY = topMargin + gridH + 10;
    struct LegendItem { QString label; QColor color; };
    const QVector<LegendItem> items = {
        { "Свободно", QColor("#F0F0F0") },
        { "Низкая загрузка", QColor("#8BC34A") },
        { "Средняя загрузка", QColor("#FFEB3B") },
        { "Высокая загрузка", QColor("#FF9800") },
        { "Перегрузка", QColor("#F44336") }
    };

    int x = leftMargin;
    for (const LegendItem& item : items) {
        painter.fillRect(QRectF(x, legendY, 14, 14), item.color);
        painter.setPen(QColor(90, 90, 90));
        painter.drawRect(QRectF(x, legendY, 14, 14));
        painter.setPen(palette().color(QPalette::WindowText));
        painter.drawText(QRectF(x + 18, legendY - 2, 140, 18), Qt::AlignVCenter, item.label);
        x += 150;
    }
}

bool HeatmapWidget::saveToPng(const QString& filePath)
{
    QPixmap pixmap = grab();
    return pixmap.save(filePath, "PNG");
}
