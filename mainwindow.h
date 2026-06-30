#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "datamanager.h"
#include "scheduler.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class DataTab;
class GenerationTab;
class ResultTab;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    DataManager* m_dataManager;
    IScheduler* m_scheduler; // временно указывает на StubScheduler, см. scheduler.h

    DataTab* m_dataTab;
    GenerationTab* m_generationTab;
    ResultTab* m_resultTab;
};
#endif // MAINWINDOW_H
