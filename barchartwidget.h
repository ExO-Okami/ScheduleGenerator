#ifndef BARCHARTWIDGET_H
#define BARCHARTWIDGET_H

#include <QWidget>
#include <QMap>
#include <QString>

// Простая столбчатая диаграмма на QPainter — используется для отображения
// загрузки преподавателей без подключения дополнительного модуля QtCharts.
class BarChartWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BarChartWidget(QWidget* parent = nullptr);

    void setData(const QMap<QString, int>& data, const QString& title = QString());
    bool saveToPng(const QString& filePath);

protected:
    void paintEvent(QPaintEvent* event) override;
    QSize minimumSizeHint() const override;

private:
    QMap<QString, int> m_data;
    QString m_title;
};

#endif // BARCHARTWIDGET_H
