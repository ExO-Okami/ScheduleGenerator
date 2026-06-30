#ifndef SUBJECTDIALOG_H
#define SUBJECTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QListWidget>
#include "models.h"

// Диалог добавления/редактирования дисциплины: название, тип, часы,
// преподаватель (выбор из списка) и целевые группы (множественный выбор).
class SubjectDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SubjectDialog(QWidget* parent = nullptr);

    void setAvailableTeachers(const QVector<Teacher>& teachers);
    void setAvailableGroups(const QVector<Group>& groups);

    void setSubject(const Subject& subject);
    Subject subject() const;

private:
    QLineEdit* m_nameEdit;
    QComboBox* m_typeCombo;
    QSpinBox* m_hoursSpin;
    QComboBox* m_teacherCombo;
    QListWidget* m_groupsList;
};

#endif // SUBJECTDIALOG_H
