#ifndef DATATAB_H
#define DATATAB_H

#include <QWidget>
#include <QTableWidget>
#include "datamanager.h"

// Вкладка "Данные": ввод и редактирование групп, преподавателей,
// аудиторий и дисциплин. Содержит внутренний QTabWidget с четырьмя
// таблицами и кнопками "Добавить" / "Удалить" / "Редактировать",
// а также общую кнопку "Импорт из JSON".
class DataTab : public QWidget
{
    Q_OBJECT
public:
    explicit DataTab(DataManager* dataManager, QWidget* parent = nullptr);

private slots:
    void onImportJson();
    void refreshAll();

    // Группы
    void onAddGroup();
    void onRemoveGroup();
    void onGroupCellChanged(int row, int column);

    // Преподаватели
    void onAddTeacher();
    void onRemoveTeacher();
    void onEditTeacherAvailability();
    void onTeacherNameChanged(int row, int column);

    // Аудитории
    void onAddRoom();
    void onRemoveRoom();
    void onRoomCellChanged(int row, int column);

    // Дисциплины
    void onAddSubject();
    void onRemoveSubject();
    void onEditSubject(int row, int column);

private:
    DataManager* m_dataManager;

    QTableWidget* m_groupsTable;
    QTableWidget* m_teachersTable;
    QTableWidget* m_roomsTable;
    QTableWidget* m_subjectsTable;

    bool m_blockSignals = false; // защита от рекурсивного срабатывания itemChanged при перезаполнении таблиц

    void refreshGroupsTable();
    void refreshTeachersTable();
    void refreshRoomsTable();
    void refreshSubjectsTable();

    QWidget* buildGroupsPage();
    QWidget* buildTeachersPage();
    QWidget* buildRoomsPage();
    QWidget* buildSubjectsPage();
};

#endif // DATATAB_H
