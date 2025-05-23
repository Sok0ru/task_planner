#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QInputDialog>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFont>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupUI();
    loadTasks();
    applyDarkTheme();
}

void MainWindow::setupUI()
{
    // Настройка дерева дат
    ui->datesTree->setColumnCount(1);
    ui->datesTree->setHeaderLabel("Даты");

    // Настройка списка задач
    ui->tasksList->setSelectionMode(QAbstractItemView::SingleSelection);

    // Соединение сигналов
    connect(ui->datesTree, &QTreeWidget::itemClicked,
            this, &MainWindow::onDateSelected);
    connect(ui->addButton, &QPushButton::clicked,
            this, &MainWindow::onAddTaskClicked);
    connect(ui->removeButton, &QPushButton::clicked,
            this, &MainWindow::onRemoveTaskClicked);
    connect(ui->completeButton, &QPushButton::clicked,
            this, &MainWindow::onCompleteClicked);
    connect(ui->promoteButton, &QPushButton::clicked,
            this, &MainWindow::onPromoteClicked);
    connect(ui->tasksList, &QListWidget::itemChanged,
            this, &MainWindow::onTaskStatusChanged);

    // Добавляем текущую дату
    currentDate = QDate::currentDate();
    addDateToTree(currentDate);
    updateTasksView();
}

void MainWindow::applyDarkTheme()
{
    // Основные цвета
    QString baseColor = "#424242";
    QString hoverColor = "#616161";
    QString pressColor = "#212121";
    QString accentColor = "#757575";

    // Общий стиль
    this->setStyleSheet(QString(R"(
        QMainWindow {
            background-color: #2d2d2d;
        }
        QTreeWidget, QListWidget {
            background-color: #353535;
            color: white;
            border: 1px solid #444;
            border-radius: 5px;
            font-size: 14px;
        }
        QTreeWidget::item, QListWidget::item {
            padding: 8px;
            border-bottom: 1px solid #444;
        }
        QTreeWidget::item:hover, QListWidget::item:hover {
            background-color: %1;
        }
        QTreeWidget::item:selected, QListWidget::item:selected {
            background-color: %2;
        }
        QPushButton {
            background-color: %3;
            color: white;
            border: 1px solid #555;
            padding: 8px 16px;
            border-radius: 4px;
            min-width: 100px;
        }
        QPushButton:hover {
            background-color: %4;
        }
        QPushButton:pressed {
            background-color: %5;
        }
    )").arg(hoverColor).arg(accentColor).arg(baseColor).arg(hoverColor).arg(pressColor));

    // Текстовые метки для кнопок
    ui->addButton->setText("＋ Добавить");
    ui->removeButton->setText("✕ Удалить");
    ui->completeButton->setText("✓ Выполнено");
    ui->promoteButton->setText("↑ Повысить");
}

void MainWindow::addDateToTree(const QDate &date)
{
    QString dateStr = date.toString("dd.MM.yyyy");

    for(int i = 0; i < ui->datesTree->topLevelItemCount(); ++i) {
        if(ui->datesTree->topLevelItem(i)->text(0) == dateStr) {
            return;
        }
    }

    QTreeWidgetItem *item = new QTreeWidgetItem(ui->datesTree);
    item->setText(0, dateStr);
    item->setData(0, Qt::UserRole, date);
    ui->datesTree->sortItems(0, Qt::AscendingOrder);
}

void MainWindow::onDateSelected(QTreeWidgetItem *item)
{
    currentDate = item->data(0, Qt::UserRole).toDate();
    updateTasksView();
}

void MainWindow::onAddTaskClicked()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("Добавить задачу"),
                                     tr("Текст задачи:"), QLineEdit::Normal,
                                     "", &ok);
    if (ok && !text.isEmpty()) {
        tasksData[currentDate].append({text, false, 0});
        updateTasksView();
        saveTasks();
        saveTasks(); // Явное сохранение после добавления
        qDebug() << "Задача добавлена, файл должен быть сохранён";
    }
}

void MainWindow::onRemoveTaskClicked()
{
    int row = ui->tasksList->currentRow();
    if (row >= 0 && tasksData.contains(currentDate)) {
        tasksData[currentDate].removeAt(row);
        updateTasksView();
        saveTasks();
    }
}

void MainWindow::onCompleteClicked()
{
    QListWidgetItem *item = ui->tasksList->currentItem();
    if (item) {
        bool completed = item->checkState() != Qt::Checked;
        item->setCheckState(completed ? Qt::Checked : Qt::Unchecked);
    }
}

void MainWindow::onPromoteClicked()
{
    int currentRow = ui->tasksList->currentRow();
    if (currentRow > 0 && tasksData.contains(currentDate)) {
        auto& tasks = tasksData[currentDate];
        if (currentRow < tasks.size()) {
            tasks[currentRow].priority += 2;
            updateTasksView();
            saveTasks();
        }
    }
}

void MainWindow::onTaskStatusChanged(QListWidgetItem *item)
{
    if (!tasksData.contains(currentDate)) return;

    QString taskText = item->text();
    bool completed = item->checkState() == Qt::Checked;

    for (auto& task : tasksData[currentDate]) {
        if (task.text == taskText) {
            task.completed = completed;
            break;
        }
    }

    QFont font = item->font();
    font.setStrikeOut(completed);
    item->setFont(font);
    item->setForeground(completed ? QColor("#a0a0a0") : QColor("#ffffff"));

    saveTasks();
}

void MainWindow::updateTasksView()
{
    ui->tasksList->clear();
    if (!tasksData.contains(currentDate)) return;

    // Сортируем по приоритету (по убыванию)
    auto& tasks = tasksData[currentDate];
    std::sort(tasks.begin(), tasks.end(),
        [](const TaskData& a, const TaskData& b) {
            return a.priority > b.priority;
        });

    for (const auto& task : tasks) {
        QListWidgetItem *item = new QListWidgetItem(task.text, ui->tasksList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(task.completed ? Qt::Checked : Qt::Unchecked);

        QFont font = item->font();
        font.setStrikeOut(task.completed);
        item->setFont(font);
        item->setForeground(task.completed ? QColor("#a0a0a0") : QColor("#ffffff"));
    }
}

void MainWindow::saveTasks()
{
    QJsonObject root;
    for (auto it = tasksData.begin(); it != tasksData.end(); ++it) {
        QJsonArray tasksArray;
        for (const auto& task : it.value()) {
            QJsonObject taskObj;
            taskObj["text"] = task.text;
            taskObj["completed"] = task.completed;
            taskObj["priority"] = task.priority;
            tasksArray.append(taskObj);
        }
        root[it.key().toString("dd.MM.yyyy")] = tasksArray;
    }

    QFile file("tasks.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
        QString path = QCoreApplication::applicationDirPath() + "/tasks.json";
        qDebug() << "Пытаюсь сохранить в:" << path;

        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            qDebug() << "Ошибка открытия файла:" << file.errorString();
            return;
        }

        // ... (остальной код сохранения)
        file.flush(); // Принудительная запись
        qDebug() << "Файл успешно сохранён";
    }

    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Не удалось сохранить задачи!";
        return;

    }
}

void MainWindow::loadTasks()
{
    QFile file("tasks.json");
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject root = doc.object();

        for (auto it = root.begin(); it != root.end(); ++it) {
            QDate date = QDate::fromString(it.key(), "dd.MM.yyyy");
            QList<TaskData> tasks;

            for (const auto& taskVal : it.value().toArray()) {
                QJsonObject taskObj = taskVal.toObject();
                tasks.append({
                    taskObj["text"].toString(),
                    taskObj["completed"].toBool(),
                    taskObj["priority"].toInt()
                });
            }

            tasksData[date] = tasks;
            addDateToTree(date);
        }
    }
}

MainWindow::~MainWindow()
{
    saveTasks();
    delete ui;
}
