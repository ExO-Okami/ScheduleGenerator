#ifndef GENERATIONTAB_H
#define GENERATIONTAB_H

#include <QWidget>
#include <QProgressBar>
#include <QTextEdit>
#include <QPushButton>
#include "datamanager.h"
#include "scheduler.h"

// Вкладка "Генерация": запуск алгоритма построения расписания и
// отображение прогресса/ошибок (незаназначенных занятий).
class GenerationTab : public QWidget
{
    Q_OBJECT
public:
    explicit GenerationTab(DataManager* dataManager, IScheduler* scheduler, QWidget* parent = nullptr);

signals:
    // Испускается после успешной генерации, чтобы вкладка "Результат" обновила отображение.
    void scheduleGenerated(const QVector<Lesson>& schedule);

private slots:
    void onGenerateClicked();

private:
    DataManager* m_dataManager;
    IScheduler* m_scheduler;

    QPushButton* m_generateButton;
    QProgressBar* m_progressBar;
    QTextEdit* m_logView;
};

#endif // GENERATIONTAB_H
