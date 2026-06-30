#include "barchartwidget.h"
#include <QPainter>
#include <QPixmap>
#include <algorithm>

BarChartWidget::BarChartWidget(QWidget* parent) : QWidget(parent)
{
    setMinimumSize(420, 280);
}

void BarChartWidget::setData(const QMap<QString, int>& data, const QString& title)
{
    m_data = data;
    m_title = title;
    update();
}

QSize BarChartWidget::minimumSizeHint() const { return QSize(420, 280); }

void BarChartWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), palette().color(QPalette::Base));

    if (m_data.isEmpty()) {
        painter.setPen(palette().color(QPalette::WindowText));
        painter.drawText(rect(), Qt::AlignCenter, "Нет данных для отображения.\nСначала сгенерируйте расписание.");
        return;
    }

    const int topMargin = m_title.isEmpty() ? 10 : 34;
    const int bottomMargin = 50;
    const int leftMargin = 40;
    const int rightMargin = 15;

    if (!m_title.isEmpty()) {
        QFont titleFont = painter.font();
        titleFont.setBold(true);
        painter.setFont(titleFont);
        painter.setPen(palette().color(QPalette::WindowText));
        painter.drawText(QRect(0, 4, width(), 24), Qt::AlignCenter, m_title);
        painter.setFont(QFont());
    }

    const int chartW = width() - leftMargin - rightMargin;
    const int chartH = height() - topMargin - bottomMargin;
    if (chartW <= 0 || chartH <= 0) return;

    int maxValue = 1;
    for (int v : m_data.values())
        maxValue = std::max(maxValue, v);

    const int n = m_data.size();
    const double slotW = double(chartW) / n;
    const double barW = std::min(slotW * 0.6, 60.0);

    // Оси
    painter.setPen(QColor(150, 150, 150));
    painter.drawLine(leftMargin, topMargin, leftMargin, topMargin + chartH);
    painter.drawLine(leftMargin, topMargin + chartH, leftMargin + chartW, topMargin + chartH);

    // Горизонтальные направляющие + подписи оси Y
    painter.setPen(palette().color(QPalette::WindowText));
    const int steps = 4;
    for (int s = 0; s <= steps; ++s) {
        const double value = maxValue * double(s) / steps;
        const double y = topMargin + chartH - (value / maxValue) * chartH;
        painter.setPen(QColor(225, 225, 225));
        painter.drawLine(leftMargin, int(y), leftMargin + chartW, int(y));
        painter.setPen(palette().color(QPalette::WindowText));
        painter.drawText(QRectF(0, y - 8, leftMargin - 4, 16), Qt::AlignRight | Qt::AlignVCenter,
                          QString::number(qRound(value)));
    }

    // Столбцы
    int i = 0;
    static const QVector<QColor> palette_colors = {
        QColor("#5B8FF9"), QColor("#5AD8A6"), QColor("#F6BD16"),
        QColor("#E8684A"), QColor("#6DC8EC"), QColor("#9270CA")
    };

    for (auto it = m_data.constBegin(); it != m_data.constEnd(); ++it, ++i) {
        const double centerX = leftMargin + slotW * i + slotW / 2.0;
        const double barHeight = (double(it.value()) / maxValue) * chartH;
        QRectF barRect(centerX - barW / 2.0, topMargin + chartH - barHeight, barW, barHeight);

        painter.setPen(Qt::NoPen);
        painter.setBrush(palette_colors.at(i % palette_colors.size()));
        painter.drawRect(barRect);

        // Значение над столбцом
        painter.setPen(palette().color(QPalette::WindowText));
        painter.drawText(QRectF(centerX - slotW / 2.0, barRect.top() - 18, slotW, 16),
                          Qt::AlignCenter, QString::number(it.value()));

        // Подпись (ФИО преподавателя) под осью X, с переносом по ширине слота
        QFont smallFont = painter.font();
        smallFont.setPointSize(8);
        painter.setFont(smallFont);
        QRectF labelRect(centerX - slotW / 2.0, topMargin + chartH + 4, slotW, bottomMargin - 6);
        painter.drawText(labelRect, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, it.key());
        painter.setFont(QFont());
    }
}

bool BarChartWidget::saveToPng(const QString& filePath)
{
    QPixmap pixmap = grab();
    return pixmap.save(filePath, "PNG");
}
