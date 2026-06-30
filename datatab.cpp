#include "datatab.h"
#include "teacheravailabilitydialog.h"
#include "subjectdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QPushButton>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QLabel>

DataTab::DataTab(DataManager* dataManager, QWidget* parent)
    : QWidget(parent), m_dataManager(dataManager)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QHBoxLayout* topBar = new QHBoxLayout();
    QPushButton* importButton = new QPushButton("Импорт из JSON...");
    connect(importButton, &QPushButton::clicked, this, &DataTab::onImportJson);
    topBar->addWidget(importButton);
    topBar->addStretch();
    mainLayout->addLayout(topBar);

    QTabWidget* innerTabs = new QTabWidget(this);
    innerTabs->addTab(buildGroupsPage(), "Группы");
    innerTabs->addTab(buildTeachersPage(), "Преподаватели");
    innerTabs->addTab(buildRoomsPage(), "Аудитории");
    innerTabs->addTab(buildSubjectsPage(), "Дисциплины");
    mainLayout->addWidget(innerTabs);

    connect(m_dataManager, &DataManager::dataUpdated, this, &DataTab::refreshAll);
    connect(m_dataManager, &DataManager::errorOccurred, this, [this](const QString& msg) {
        QMessageBox::warning(this, "Ошибка", msg);
    });

    refreshAll();
}

void DataTab::refreshAll()
{
    refreshGroupsTable();
    refreshTeachersTable();
    refreshRoomsTable();
    refreshSubjectsTable();
}

void DataTab::onImportJson()
{
    const QString filePath = QFileDialog::getOpenFileName(this, "Импорт данных из JSON",
                                                            QString(), "JSON файлы (*.json)");
    if (filePath.isEmpty())
        return;

    if (m_dataManager->loadFromJson(filePath))
        QMessageBox::information(this, "Импорт", "Данные успешно загружены.");
}

// =====================  Группы  =====================

QWidget* DataTab::buildGroupsPage()
{
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);

    m_groupsTable = new QTableWidget(0, 2, page);
    m_groupsTable->setHorizontalHeaderLabels({ "Название группы", "Кол-во студентов" });
    m_groupsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    connect(m_groupsTable, &QTableWidget::cellChanged, this, &DataTab::onGroupCellChanged);
    layout->addWidget(m_groupsTable);

    QHBoxLayout* buttonsLayout = new QHBoxLayout();
    QPushButton* addBtn = new QPushButton("Добавить");
    QPushButton* removeBtn = new QPushButton("Удалить");
    connect(addBtn, &QPushButton::clicked, this, &DataTab::onAddGroup);
    connect(removeBtn, &QPushButton::clicked, this, &DataTab::onRemoveGroup);
    buttonsLayout->addWidget(addBtn);
    buttonsLayout->addWidget(removeBtn);
    buttonsLayout->addStretch();
    layout->addLayout(buttonsLayout);
    layout->addWidget(new QLabel("Дважды кликните по ячейке, чтобы отредактировать значение."));

    return page;
}

void DataTab::refreshGroupsTable()
{
    m_blockSignals = true;
    const QVector<Group> groups = m_dataManager->getGroups();
    m_groupsTable->setRowCount(groups.size());
    for (int i = 0; i < groups.size(); ++i) {
        m_groupsTable->setItem(i, 0, new QTableWidgetItem(groups[i].name));
        m_groupsTable->setItem(i, 1, new QTableWidgetItem(QString::number(groups[i].studentsCount)));
    }
    m_blockSignals = false;
}

void DataTab::onAddGroup()
{
    Group g;
    g.name = "Новая группа";
    g.studentsCount = 0;
    m_dataManager->addGroup(g);
}

void DataTab::onRemoveGroup()
{
    const int row = m_groupsTable->currentRow();
    if (row < 0) return;
    m_dataManager->removeGroup(row);
}

void DataTab::onGroupCellChanged(int row, int column)
{
    if (m_blockSignals) return;
    QVector<Group> groups = m_dataManager->getGroups();
    if (row < 0 || row >= groups.size()) return;

    if (column == 0) {
        groups[row].name = m_groupsTable->item(row, column)->text();
    } else if (column == 1) {
        groups[row].studentsCount = m_groupsTable->item(row, column)->text().toInt();
    }
    m_dataManager->updateGroup(row, groups[row]);
}

// =====================  Преподаватели  =====================

QWidget* DataTab::buildTeachersPage()
{
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);

    m_teachersTable = new QTableWidget(0, 2, page);
    m_teachersTable->setHorizontalHeaderLabels({ "ФИО", "Доступность (слотов занято)" });
    m_teachersTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_teachersTable->setEditTriggers(QAbstractItemView::DoubleClicked);
    connect(m_teachersTable, &QTableWidget::cellChanged, this, &DataTab::onTeacherNameChanged);
    layout->addWidget(m_teachersTable);

    QHBoxLayout* buttonsLayout = new QHBoxLayout();
    QPushButton* addBtn = new QPushButton("Добавить");
    QPushButton* removeBtn = new QPushButton("Удалить");
    QPushButton* editAvailBtn = new QPushButton("Редактировать доступность...");
    connect(addBtn, &QPushButton::clicked, this, &DataTab::onAddTeacher);
    connect(removeBtn, &QPushButton::clicked, this, &DataTab::onRemoveTeacher);
    connect(editAvailBtn, &QPushButton::clicked, this, &DataTab::onEditTeacherAvailability);
    buttonsLayout->addWidget(addBtn);
    buttonsLayout->addWidget(removeBtn);
    buttonsLayout->addWidget(editAvailBtn);
    buttonsLayout->addStretch();
    layout->addLayout(buttonsLayout);

    return page;
}

void DataTab::refreshTeachersTable()
{
    m_blockSignals = true;
    const QVector<Teacher> teachers = m_dataManager->getTeachers();
    m_teachersTable->setRowCount(teachers.size());
    for (int i = 0; i < teachers.size(); ++i) {
        m_teachersTable->setItem(i, 0, new QTableWidgetItem(teachers[i].fullName));

        int slotCount = 0;
        for (const auto& pairs : teachers[i].availability)
            slotCount += pairs.size();

        QTableWidgetItem* availItem = new QTableWidgetItem(QString("%1 слотов").arg(slotCount));
        availItem->setFlags(availItem->flags() & ~Qt::ItemIsEditable);
        m_teachersTable->setItem(i, 1, availItem);
    }
    m_blockSignals = false;
}

void DataTab::onAddTeacher()
{
    Teacher t;
    t.fullName = "Новый преподаватель";
    m_dataManager->addTeacher(t);
}

void DataTab::onRemoveTeacher()
{
    const int row = m_teachersTable->currentRow();
    if (row < 0) return;
    m_dataManager->removeTeacher(row);
}

void DataTab::onTeacherNameChanged(int row, int column)
{
    if (m_blockSignals || column != 0) return;
    QVector<Teacher> teachers = m_dataManager->getTeachers();
    if (row < 0 || row >= teachers.size()) return;
    teachers[row].fullName = m_teachersTable->item(row, column)->text();
    m_dataManager->updateTeacher(row, teachers[row]);
}

void DataTab::onEditTeacherAvailability()
{
    const int row = m_teachersTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "Доступность", "Сначала выберите преподавателя в таблице.");
        return;
    }

    QVector<Teacher> teachers = m_dataManager->getTeachers();
    if (row >= teachers.size()) return;

    TeacherAvailabilityDialog dialog(this);
    dialog.setAvailability(teachers[row].availability);
    if (dialog.exec() == QDialog::Accepted) {
        teachers[row].availability = dialog.availability();
        m_dataManager->updateTeacher(row, teachers[row]);
    }
}

// =====================  Аудитории  =====================

QWidget* DataTab::buildRoomsPage()
{
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);

    m_roomsTable = new QTableWidget(0, 3, page);
    m_roomsTable->setHorizontalHeaderLabels({ "Номер", "Тип", "Вместимость" });
    m_roomsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    connect(m_roomsTable, &QTableWidget::cellChanged, this, &DataTab::onRoomCellChanged);
    layout->addWidget(m_roomsTable);

    QHBoxLayout* buttonsLayout = new QHBoxLayout();
    QPushButton* addBtn = new QPushButton("Добавить");
    QPushButton* removeBtn = new QPushButton("Удалить");
    connect(addBtn, &QPushButton::clicked, this, &DataTab::onAddRoom);
    connect(removeBtn, &QPushButton::clicked, this, &DataTab::onRemoveRoom);
    buttonsLayout->addWidget(addBtn);
    buttonsLayout->addWidget(removeBtn);
    buttonsLayout->addStretch();
    layout->addLayout(buttonsLayout);

    return page;
}

void DataTab::refreshRoomsTable()
{
    m_blockSignals = true;
    const QVector<Room> rooms = m_dataManager->getRooms();
    m_roomsTable->setRowCount(rooms.size());
    for (int i = 0; i < rooms.size(); ++i) {
        m_roomsTable->setItem(i, 0, new QTableWidgetItem(rooms[i].number));
        m_roomsTable->setItem(i, 1, new QTableWidgetItem(rooms[i].type));
        m_roomsTable->setItem(i, 2, new QTableWidgetItem(QString::number(rooms[i].capacity)));
    }
    m_blockSignals = false;
}

void DataTab::onAddRoom()
{
    Room r;
    r.number = "Новая аудитория";
    r.type = "Общая";
    r.capacity = 30;
    m_dataManager->addRoom(r);
}

void DataTab::onRemoveRoom()
{
    const int row = m_roomsTable->currentRow();
    if (row < 0) return;
    m_dataManager->removeRoom(row);
}

void DataTab::onRoomCellChanged(int row, int column)
{
    if (m_blockSignals) return;
    QVector<Room> rooms = m_dataManager->getRooms();
    if (row < 0 || row >= rooms.size()) return;

    switch (column) {
    case 0: rooms[row].number = m_roomsTable->item(row, column)->text(); break;
    case 1: rooms[row].type = m_roomsTable->item(row, column)->text(); break;
    case 2: rooms[row].capacity = m_roomsTable->item(row, column)->text().toInt(); break;
    }
    m_dataManager->updateRoom(row, rooms[row]);
}

// =====================  Дисциплины  =====================

QWidget* DataTab::buildSubjectsPage()
{
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);

    m_subjectsTable = new QTableWidget(0, 5, page);
    m_subjectsTable->setHorizontalHeaderLabels({ "Название", "Тип", "Часы", "Преподаватель", "Группы" });
    m_subjectsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_subjectsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(m_subjectsTable, &QTableWidget::cellDoubleClicked, this, &DataTab::onEditSubject);
    layout->addWidget(m_subjectsTable);

    QHBoxLayout* buttonsLayout = new QHBoxLayout();
    QPushButton* addBtn = new QPushButton("Добавить");
    QPushButton* removeBtn = new QPushButton("Удалить");
    QPushButton* editBtn = new QPushButton("Редактировать...");
    connect(addBtn, &QPushButton::clicked, this, &DataTab::onAddSubject);
    connect(removeBtn, &QPushButton::clicked, this, &DataTab::onRemoveSubject);
    connect(editBtn, &QPushButton::clicked, this, [this]() { onEditSubject(m_subjectsTable->currentRow(), 0); });
    buttonsLayout->addWidget(addBtn);
    buttonsLayout->addWidget(removeBtn);
    buttonsLayout->addWidget(editBtn);
    buttonsLayout->addStretch();
    layout->addLayout(buttonsLayout);
    layout->addWidget(new QLabel("Дважды кликните по строке, чтобы отредактировать дисциплину."));

    return page;
}

void DataTab::refreshSubjectsTable()
{
    const QVector<Subject> subjects = m_dataManager->getSubjects();
    m_subjectsTable->setRowCount(subjects.size());
    for (int i = 0; i < subjects.size(); ++i) {
        m_subjectsTable->setItem(i, 0, new QTableWidgetItem(subjects[i].name));
        m_subjectsTable->setItem(i, 1, new QTableWidgetItem(subjects[i].type));
        m_subjectsTable->setItem(i, 2, new QTableWidgetItem(QString::number(subjects[i].hours)));
        m_subjectsTable->setItem(i, 3, new QTableWidgetItem(subjects[i].teacherId));
        m_subjectsTable->setItem(i, 4, new QTableWidgetItem(subjects[i].targetGroups.join(", ")));
    }
}

void DataTab::onAddSubject()
{
    SubjectDialog dialog(this);
    dialog.setAvailableTeachers(m_dataManager->getTeachers());
    dialog.setAvailableGroups(m_dataManager->getGroups());

    if (m_dataManager->getTeachers().isEmpty() || m_dataManager->getGroups().isEmpty()) {
        QMessageBox::information(this, "Дисциплина",
                                  "Сначала добавьте хотя бы одну группу и одного преподавателя.");
        return;
    }

    if (dialog.exec() == QDialog::Accepted) {
        Subject s = dialog.subject();
        if (s.name.isEmpty()) {
            QMessageBox::warning(this, "Дисциплина", "Название дисциплины не может быть пустым.");
            return;
        }
        m_dataManager->addSubject(s);
    }
}

void DataTab::onRemoveSubject()
{
    const int row = m_subjectsTable->currentRow();
    if (row < 0) return;
    m_dataManager->removeSubject(row);
}

void DataTab::onEditSubject(int row, int /*column*/)
{
    if (row < 0) return;
    QVector<Subject> subjects = m_dataManager->getSubjects();
    if (row >= subjects.size()) return;

    SubjectDialog dialog(this);
    dialog.setAvailableTeachers(m_dataManager->getTeachers());
    dialog.setAvailableGroups(m_dataManager->getGroups());
    dialog.setSubject(subjects[row]);

    if (dialog.exec() == QDialog::Accepted) {
        Subject s = dialog.subject();
        if (s.name.isEmpty()) {
            QMessageBox::warning(this, "Дисциплина", "Название дисциплины не может быть пустым.");
            return;
        }
        m_dataManager->updateSubject(row, s);
    }
}
