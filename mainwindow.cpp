#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "datatab.h"
#include "generationtab.h"
#include "resulttab.h"

#include <QTabWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("Генератор учебного расписания");
    resize(1200, 800);

    m_dataManager = new DataManager(this);

    // ВРЕМЕННО: используется StubScheduler (заглушка), пока разработчик
    // алгоритма не предоставит полноценную реализацию IScheduler.
    // Когда алгоритм будет готов, достаточно заменить эту строку на
    // создание реального класса-наследника IScheduler — остальной код
    // интерфейса менять не требуется.
    m_scheduler = new StubScheduler();

    QTabWidget* tabs = new QTabWidget(this);
    setCentralWidget(tabs);

    m_dataTab = new DataTab(m_dataManager, this);
    m_generationTab = new GenerationTab(m_dataManager, m_scheduler, this);
    m_resultTab = new ResultTab(m_dataManager, this);

    tabs->addTab(m_dataTab, "Данные");
    tabs->addTab(m_generationTab, "Генерация");
    tabs->addTab(m_resultTab, "Результат");

    connect(m_generationTab, &GenerationTab::scheduleGenerated,
            m_resultTab, &ResultTab::setSchedule);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_scheduler;
}
