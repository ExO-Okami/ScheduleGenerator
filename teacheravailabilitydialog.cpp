#include "teacheravailabilitydialog.h"
#include "scheduleutils.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QLabel>

TeacherAvailabilityDialog::TeacherAvailabilityDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle("Доступность преподавателя");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(new QLabel("Отметьте слоты, в которые преподаватель может вести занятия:"));

    QGridLayout* grid = new QGridLayout();
    mainLayout->addLayout(grid);

    grid->addWidget(new QLabel(""), 0, 0);
    for (int d = 1; d <= kDays; ++d) {
        QLabel* header = new QLabel(ScheduleUtils::dayShortName(d));
        header->setAlignment(Qt::AlignCenter);
        QFont f = header->font();
        f.setBold(true);
        header->setFont(f);
        grid->addWidget(header, 0, d);
    }

    for (int p = 1; p <= kPairs; ++p) {
        grid->addWidget(new QLabel(QString("Пара %1").arg(p)), p, 0);
        for (int d = 1; d <= kDays; ++d) {
            QCheckBox* box = new QCheckBox();
            box->setStyleSheet("margin-left: 12px;");
            m_checkBoxes[d][p] = box;
            grid->addWidget(box, p, d);
        }
    }

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);
}

void TeacherAvailabilityDialog::setAvailability(const QMap<int, QVector<int>>& availability)
{
    for (int d = 1; d <= kDays; ++d)
        for (int p = 1; p <= kPairs; ++p)
            m_checkBoxes[d][p]->setChecked(false);

    for (auto it = availability.constBegin(); it != availability.constEnd(); ++it) {
        const int day = it.key();
        if (day < 1 || day > kDays) continue;
        for (int pair : it.value()) {
            if (pair >= 1 && pair <= kPairs)
                m_checkBoxes[day][pair]->setChecked(true);
        }
    }
}

QMap<int, QVector<int>> TeacherAvailabilityDialog::availability() const
{
    QMap<int, QVector<int>> result;
    for (int d = 1; d <= kDays; ++d) {
        QVector<int> pairs;
        for (int p = 1; p <= kPairs; ++p) {
            if (m_checkBoxes[d][p]->isChecked())
                pairs.append(p);
        }
        if (!pairs.isEmpty())
            result.insert(d, pairs);
    }
    return result;
}
