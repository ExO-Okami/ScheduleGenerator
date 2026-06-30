#include "subjectdialog.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QLabel>

SubjectDialog::SubjectDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle("Дисциплина");
    setMinimumWidth(360);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QFormLayout* form = new QFormLayout();
    mainLayout->addLayout(form);

    m_nameEdit = new QLineEdit(this);
    form->addRow("Название:", m_nameEdit);

    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItems({ "Лекция", "Практика", "Лабораторная" });
    m_typeCombo->setEditable(true);
    form->addRow("Тип занятия:", m_typeCombo);

    m_hoursSpin = new QSpinBox(this);
    m_hoursSpin->setRange(1, 200);
    m_hoursSpin->setValue(2);
    form->addRow("Часов в неделю:", m_hoursSpin);

    m_teacherCombo = new QComboBox(this);
    form->addRow("Преподаватель:", m_teacherCombo);

    mainLayout->addWidget(new QLabel("Группы (можно выбрать несколько):"));
    m_groupsList = new QListWidget(this);
    m_groupsList->setSelectionMode(QAbstractItemView::NoSelection);
    mainLayout->addWidget(m_groupsList);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);
}

void SubjectDialog::setAvailableTeachers(const QVector<Teacher>& teachers)
{
    m_teacherCombo->clear();
    for (const Teacher& t : teachers)
        m_teacherCombo->addItem(t.fullName);
}

void SubjectDialog::setAvailableGroups(const QVector<Group>& groups)
{
    m_groupsList->clear();
    for (const Group& g : groups) {
        QListWidgetItem* item = new QListWidgetItem(g.name, m_groupsList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }
}

void SubjectDialog::setSubject(const Subject& subject)
{
    m_nameEdit->setText(subject.name);
    m_typeCombo->setCurrentText(subject.type);
    m_hoursSpin->setValue(subject.hours > 0 ? subject.hours : 2);

    const int teacherIndex = m_teacherCombo->findText(subject.teacherId);
    if (teacherIndex >= 0)
        m_teacherCombo->setCurrentIndex(teacherIndex);

    for (int i = 0; i < m_groupsList->count(); ++i) {
        QListWidgetItem* item = m_groupsList->item(i);
        item->setCheckState(subject.targetGroups.contains(item->text()) ? Qt::Checked : Qt::Unchecked);
    }
}

Subject SubjectDialog::subject() const
{
    Subject s;
    s.name = m_nameEdit->text().trimmed();
    s.type = m_typeCombo->currentText().trimmed();
    s.hours = m_hoursSpin->value();
    s.teacherId = m_teacherCombo->currentText();

    for (int i = 0; i < m_groupsList->count(); ++i) {
        QListWidgetItem* item = m_groupsList->item(i);
        if (item->checkState() == Qt::Checked)
            s.targetGroups.append(item->text());
    }
    return s;
}
