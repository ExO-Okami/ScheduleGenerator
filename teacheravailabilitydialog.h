#ifndef TEACHERAVAILABILITYDIALOG_H
#define TEACHERAVAILABILITYDIALOG_H

#include <QDialog>
#include <QMap>
#include <QVector>
#include <QCheckBox>

// Диалог для редактирования доступности преподавателя: сетка "день x пара",
// в которой пользователь отмечает галочками доступные слоты.
class TeacherAvailabilityDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TeacherAvailabilityDialog(QWidget* parent = nullptr);

    void setAvailability(const QMap<int, QVector<int>>& availability);
    QMap<int, QVector<int>> availability() const;

private:
    static const int kDays = 6;
    static const int kPairs = 6;
    QCheckBox* m_checkBoxes[kDays + 1][kPairs + 1]; // [день 1..6][пара 1..6]
};

#endif // TEACHERAVAILABILITYDIALOG_H
