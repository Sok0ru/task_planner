#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDate>
#include <QMap>
#include <QJsonObject>
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
    void onDateSelected(const QDate &date);
    void onAddTaskClicked();
    void onRemoveTaskClicked();
    void onCompleteClicked();
    void onPromoteClicked();
    void onPlanForFutureClicked();  // Новый слот
    void addDateToTree(const QDate &date);
    void onTaskStatusChanged(QListWidgetItem *item);

private:
    Ui::MainWindow *ui;

    struct TaskData {
        QString text;
        bool completed;
        int priority;
        QDate dueDate;  // Дата выполнения
    };

    QMap<QDate, QList<TaskData>> tasksData;
    QDate currentDate;

    void setupUI();
    void loadTasks();
    void saveTasks();
    void updateTasksView();
    void updateDatesTree();
    void addTask(const QString &text, const QDate &date, bool completed = false, int priority = 0);
    void applyDarkTheme();
    void showDateInputDialog();  // Диалог выбора даты
};
#endif // MAINWINDOW_H
