#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidgetItem>
#include <QDate>
#include <QMap>
#include <QListWidgetItem>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onDateSelected(QTreeWidgetItem *item);
    void onAddTaskClicked();
    void onRemoveTaskClicked();
    void onCompleteClicked();
    void onPromoteClicked();
    void onTaskStatusChanged(QListWidgetItem *item);

private:
    Ui::MainWindow *ui;

    struct TaskData {
        QString text;
        bool completed;
        int priority;
    };

    QMap<QDate, QList<TaskData>> tasksData;
    QDate currentDate;

    void setupUI();
    void loadTasks();
    void saveTasks();
    void updateTasksView();
    void addDateToTree(const QDate &date);
    void applyDarkTheme();
};
#endif // MAINWINDOW_H
