#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QInputDialog>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFont>
#include <QLabel>
#include <QMessageBox>
#include <QDateEdit>
#include <QListWidgetItem>

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
    // Настройка элементов интерфейса
    ui->datesTree->setHeaderLabel("Даты");
    ui->datesTree->setSelectionMode(QAbstractItemView::SingleSelection);

    // Кнопки
    connect(ui->addButton, &QPushButton::clicked, this, &MainWindow::onAddTaskClicked);
    connect(ui->removeButton, &QPushButton::clicked, this, &MainWindow::onRemoveTaskClicked);
    connect(ui->completeButton, &QPushButton::clicked, this, &MainWindow::onCompleteClicked);
    connect(ui->promoteButton, &QPushButton::clicked, this, &MainWindow::onPromoteClicked);
    connect(ui->planFutureButton, &QPushButton::clicked, this, &MainWindow::onPlanForFutureClicked);

    // Выбор даты
    connect(ui->datesTree, &QTreeWidget::itemClicked, [this](QTreeWidgetItem *item) {
        onDateSelected(item->data(0, Qt::UserRole).toDate());
    });

    // Инициализация текущей даты
    currentDate = QDate::currentDate();
    updateDatesTree();
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

void MainWindow::onPlanForFutureClicked()
{
    showDateInputDialog();
}

void MainWindow::showDateInputDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Планирование на дату");

    QVBoxLayout layout(&dialog);
    QDateEdit dateEdit;
    dateEdit.setDate(QDate::currentDate().addDays(1));
    dateEdit.setMinimumDate(QDate::currentDate().addDays(1));
    dateEdit.setDisplayFormat("dd.MM.yyyy");

    QPushButton okButton("OK");

    layout.addWidget(new QLabel("Выберите дату:"));
    layout.addWidget(&dateEdit);
    layout.addWidget(&okButton);

    connect(&okButton, &QPushButton::clicked, [&]() {
        bool ok;
        QString text = QInputDialog::getText(this, "Новая задача",
                                             "Текст задачи:", QLineEdit::Normal, "", &ok);
        if (ok && !text.isEmpty()) {
            addTask(text, dateEdit.date());
            saveTasks();
            updateDatesTree();
        }
        dialog.close();
    });

    dialog.exec();
}


void MainWindow::addTask(const QString &text, const QDate &date, bool completed, int priority)
{
    tasksData[date].append({text, completed, priority, date});

    // Если дата новая, добавляем в дерево
    if (!ui->datesTree->findItems(date.toString("dd.MM.yyyy"), Qt::MatchExactly).isEmpty()) {
        QTreeWidgetItem *item = new QTreeWidgetItem(ui->datesTree);
        item->setText(0, date.toString("dd.MM.yyyy"));
        item->setData(0, Qt::UserRole, date);
        ui->datesTree->sortItems(0, Qt::AscendingOrder);
    }
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

void MainWindow::onDateSelected(const QDate &date)
{
    currentDate = date;
    updateTasksView();
}

void MainWindow::updateDatesTree()
{
    ui->datesTree->clear();
    QList<QDate> dates = tasksData.keys();
    std::sort(dates.begin(), dates.end());

    for (const QDate &date : dates) {
        QTreeWidgetItem *item = new QTreeWidgetItem(ui->datesTree);
        item->setText(0, date.toString("dd.MM.yyyy"));
        item->setData(0, Qt::UserRole, date);

        // Подсветка сегодняшней даты
        if (date == QDate::currentDate()) {
            item->setBackground(0, QBrush(QColor(100, 100, 150)));
        }
    }
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

    // Сортируем задачи по приоритету
    auto &tasks = tasksData[currentDate];
    std::sort(tasks.begin(), tasks.end(), [](const TaskData &a, const TaskData &b) {
        return a.priority > b.priority;
    });

    // Добавляем задачи в список
    for (const TaskData &task : tasks) {
        QListWidgetItem *item = new QListWidgetItem(task.text, ui->tasksList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(task.completed ? Qt::Checked : Qt::Unchecked);
        item->setData(Qt::UserRole, QVariant::fromValue(task));

        // Визуальное оформление
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
        for (const auto &task : it.value()) {
            QJsonObject taskObj;
            taskObj["text"] = task.text;
            taskObj["completed"] = task.completed;
            taskObj["priority"] = task.priority;
            taskObj["dueDate"] = task.dueDate.toString("yyyy-MM-dd");
            tasksArray.append(taskObj);
        }
        root[it.key().toString("yyyy-MM-dd")] = tasksArray;
    }

    QFile file("tasks.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
    }
    file.flush(); // Принудительная запись
    qDebug() << "Файл успешно сохранён";


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
            QDate date = QDate::fromString(it.key(), "yyyy-MM-dd");
            if (!date.isValid()) continue;

            QList<TaskData> tasks;
            for (const auto &taskVal : it.value().toArray()) {
                QJsonObject taskObj = taskVal.toObject();
                TaskData task;
                task.text = taskObj["text"].toString();
                task.completed = taskObj["completed"].toBool();
                task.priority = taskObj["priority"].toInt();
                task.dueDate = QDate::fromString(taskObj["dueDate"].toString(), "yyyy-MM-dd");

                if (!task.dueDate.isValid()) {
                    task.dueDate = date;  // Для обратной совместимости
                }

                tasks.append(task);
            }

            tasksData[date] = tasks;
        }
    }
    updateDatesTree();
}

MainWindow::~MainWindow()
{
    saveTasks();
    delete ui;
}
