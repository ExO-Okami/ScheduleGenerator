#include "generationtab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QCoreApplication>

GenerationTab::GenerationTab(DataManager* dataManager, IScheduler* scheduler, QWidget* parent)
    : QWidget(parent), m_dataManager(dataManager), m_scheduler(scheduler)
{
    QVBoxLayout* layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel(
        "Нажмите \"Сгенерировать расписание\", чтобы построить расписание на основе "
        "данных, введённых на вкладке \"Данные\"."));

    QHBoxLayout* buttonRow = new QHBoxLayout();
    m_generateButton = new QPushButton("Сгенерировать расписание");
    connect(m_generateButton, &QPushButton::clicked, this, &GenerationTab::onGenerateClicked);
    buttonRow->addWidget(m_generateButton);
    buttonRow->addStretch();
    layout->addLayout(buttonRow);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    layout->addWidget(m_progressBar);

    layout->addWidget(new QLabel("Журнал генерации / ошибки назначения:"));
    m_logView = new QTextEdit(this);
    m_logView->setReadOnly(true);
    layout->addWidget(m_logView);
}

void GenerationTab::onGenerateClicked()
{
    const QVector<Group> groups = m_dataManager->getGroups();
    const QVector<Teacher> teachers = m_dataManager->getTeachers();
    const QVector<Room> rooms = m_dataManager->getRooms();
    const QVector<Subject> subjects = m_dataManager->getSubjects();

    if (groups.isEmpty() || teachers.isEmpty() || rooms.isEmpty() || subjects.isEmpty()) {
        QMessageBox::warning(this, "Генерация",
                              "Перед генерацией заполните данные на вкладке \"Данные\": "
                              "нужны хотя бы одна группа, преподаватель, аудитория и дисциплина.");
        return;
    }

    m_logView->clear();
    m_progressBar->setValue(10);
    m_generateButton->setEnabled(false);
    QCoreApplication::processEvents();

    const GenerationResult result = m_scheduler->generate(groups, teachers, rooms, subjects);

    m_progressBar->setValue(100);
    m_generateButton->setEnabled(true);

    m_logView->append(QString("Назначено занятий: %1").arg(result.schedule.size()));
    m_logView->append(QString("Не удалось назначить: %1").arg(result.unscheduled.size()));

    if (!result.unscheduled.isEmpty()) {
        m_logView->append("\nОшибки назначения:");
        for (const UnscheduledLesson& u : result.unscheduled) {
            m_logView->append(QString("- \"%1\" (группа: %2): %3")
                                   .arg(u.subject.name)
                                   .arg(u.group.name.isEmpty() ? "—" : u.group.name)
                                   .arg(u.reason));
        }
    }

    emit scheduleGenerated(result.schedule);
}
