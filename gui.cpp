//
// Created by jeremy on 8/29/26.
//

#include "gui.h"
#include "autoSelectWidgets.h"
#include "db.h"
#include "printDialog.h"

#include <iostream>
#include <QMainWindow>
#include <QLayout>
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QGroupBox>
#include <QPushButton>
#include <QScrollArea>
#include <QJsonDocument>
#include <QTextEdit>
#include <QDateEdit>
#include <QMessageBox>
#include <QBuffer>
#include <QPdfWriter>
#include <QImageReader>
#include <QTextTable>
#include <QPainter>
#include <QPainterPath>
#include <QCheckBox>
#include <QFileDialog>
#include <QApplication>


MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    /**
     * Main GUI Class
     */
    version = "0.1.4";

    // Create and show a splash while loading
    auto* splashWidget = makeSplashWidget();
    splashWidget->findChild<QLabel*>("detail")->setText("Creating GUI");
    splashWidget->show();
    QApplication::processEvents();

    // Set the member font variables
    mStandardFont = QFont("sans", 12);
    mBoldFont = QFont("sans:", 12, QFont::Bold);
    mHeaderFont = QFont("sans:", 16, QFont::Bold);
    mHeaderFont.setUnderline(true);
    mTitleFont = QFont("sans:", 14, QFont::Bold);

    splashWidget->findChild<QLabel*>("detail")->setText("Connecting Printers");
    QApplication::processEvents();

    // Create a single public QPrinter instance to reduce print widget load time
    printer = new QPrinter(QPrinter::PrinterResolution);
    printer->setPageSize(QPageSize(QPageSize::PageSizeId::Letter));

    loadSettings();

    splashWidget->findChild<QLabel*>("detail")->setText("Connecting to Database");
    QApplication::processEvents();

    // Load the database. Create a new database or prompt for the database location if not found.
    QString dbFileLoc = settingsJson.object()["fileLoc"].toString();
    auto [dbOk, tempDbPath] = createConnection(dbFileLoc);
    mTempDbPath = tempDbPath;

    if (!dbOk) {
        int response = QMessageBox::critical(
            this,
            "Database Error",
            "Unable to connect to the database. Would you like to locate the database?",
            QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::Cancel
        );

        if (response != QMessageBox::StandardButton::Yes) {
            QApplication::processEvents();
            QApplication::exit(EXIT_FAILURE);
            return;
        }

        QString result = QFileDialog::getOpenFileName(
            this,
            "Find Database File",
            QDir::homePath(),
            "SQLite Database (*.db)"
        );

        if (!result.isEmpty()) {
            auto [dbOk, tempDbPath] = createConnection(result);
            if (!dbOk) {
                QMessageBox::critical(
                    this,
                    "Database Error",
                    "Unable to connect to the database. The program cannot continue.",
                    QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::Cancel
                );
                QCoreApplication::exit(EXIT_FAILURE);
            } else {
                QJsonObject object = settingsJson.object();
                object["fileLoc"] = result;
                settingsJson.setObject(object);
                saveSettings();
            }
        } else {
            QCoreApplication::exit(EXIT_FAILURE);
        }
    }

    // Create the dates and ids indices
    auto [dates, ids] = getIndices();
    mDates = dates;
    mIds = ids;

    // Check if the database is an older version and convert it if so
    convertDatabase(this);

    if (mIds.size() == 0) {
        QMessageBox::critical(
            nullptr,
            "Database Error",
            "Unable to open database. The program cannot continue."
        );
        std::cout << "Unable to open database. Exiting." << std::endl;
        QCoreApplication::exit(EXIT_FAILURE);
    }

    initComponents();
    populateComboBoxes();
    addListeners();
    QList<QPair<QString, QVariant>> resultSet = getById(mIds.last());
    populateData(resultSet);

    // Set the member variable tracking changes
    mChanges = false;
    splashWidget->deleteLater();
}

QWidget* MainWindow::makeSplashWidget() {
    /**
     * Independent widget for showing messages as the program is loading
     * @return the splash widget
     */
    auto* splashWidget = new QWidget();
    splashWidget->setStyleSheet("background-color: #709070 ;");
    splashWidget->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);

    auto* splashLayout = new QVBoxLayout(splashWidget);

    auto* innerContainer = new QWidget();
    innerContainer->setObjectName("innerContainer");
    innerContainer->setStyleSheet("#innerContainer { background-color: #709070; border: 3px solid white; }");
    splashLayout->addWidget(innerContainer);

    auto* innerLayout = new QVBoxLayout(innerContainer);

    auto* splashTitleLabel = new QLabel("Starting Weekly Giving v." + version);
    splashTitleLabel->setStyleSheet("color: white;");
    splashTitleLabel->setFont(QFont("sans", 14, QFont::Bold));
    innerLayout->addWidget(splashTitleLabel);

    auto* detailContainer = new QWidget();
    innerLayout->addWidget(detailContainer);
    auto* detailLayout = new QHBoxLayout(detailContainer);

    auto* splashDetailLabel = new QLabel();
    splashDetailLabel->setObjectName("detail");
    splashDetailLabel->setStyleSheet("color: white;");
    splashDetailLabel->setFont(QFont("sans", 11, QFont::Bold));
    detailLayout->addStretch();
    detailLayout->addWidget(splashDetailLabel);
    detailLayout->addStretch();

    return splashWidget;
}

void MainWindow::loadSettings() {
    /**
     * Loads the settings from file or creates a settings file with default values if it doesn't exist
     * @return void
     */
    QFile file;
    if (QSysInfo::kernelType() == "linux") {
        file.setFileName(QDir::homePath() + "/.WeeklyGiving/config.json");
    } else {
        file.setFileName(QDir::homePath() + "/AppData/Roaming/WeeklyGiving/config.json");
    }

    // Should the file not exist or cannot be opened, create default settings and save to file
    if (!file.exists() && file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QString settingsString =
                "{"
                    "\"fileLoc\": \"/home/jeremy/LBCN/public/databases/weekly_giving.db\","
                    "\"specialDesignations\": {"
                        "\"spec1\": \"Benevolence Fund\","
                        "\"spec2\": \"Women's Ministry\","
                        "\"spec3\": \"North American Mission\","
                        "\"spec4\": \"International Mission\","
                        "\"spec5\": \"Synod\","
                        "\"spec6\": \"Seminary\","
                        "\"spec7\": \"Other (See notes)\""
                    "},"
                    "\"maxChecks\": 30,"
                    "\"name\": \"Church\","
                    "\"includeSpecial\": true"
                "}";
        file.write(settingsString.toUtf8());
        file.close();
    }

    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QString settingsString = file.readAll();
        bool saveNew = false;
        // Should the settings string be empty, create default settings
        if (settingsString.trimmed().isEmpty()) {
            settingsString =
                "{"
                    "\"fileLoc\": \"/home/jeremy/LBCN/public/databases/weekly_giving.db\","
                    "\"specialDesignations\": {"
                        "\"spec1\": \"Benevolence Fund\","
                        "\"spec2\": \"Women's Ministry\","
                        "\"spec3\": \"North American Mission\","
                        "\"spec4\": \"International Mission\","
                        "\"spec5\": \"Synod\","
                        "\"spec6\": \"Seminary\","
                        "\"spec7\": \"Other (See notes)\""
                    "},"
                    "\"maxChecks\": 30,"
                    "\"name\": \"Church\","
                    "\"includeSpecial\": true"
                "}";
            saveNew = true;
        }

        settingsJson = QJsonDocument::fromJson(settingsString.toUtf8());
        if (settingsJson.object()["includeSpecial"].isString()) {
            QJsonObject object = settingsJson.object();
            if (object["includeSpecial"].toString() == "False") {
                object["includeSpecial"] = false;
            } else {
                object["includeSpecial"] = true;
            }
            settingsJson.setObject(object);
        }

        if (saveNew) saveSettings();
    }
    file.close();
}

void MainWindow::saveSettings() {
    /**
     * Saves the current settings to file
     * @return void
     */
    QFile file;
    if (QSysInfo::kernelType() == "linux") {
        file.setFileName(QDir::homePath() + "/.WeeklyGiving/config.json");
    } else {
        file.setFileName(QDir::homePath() + "/AppData/Roaming/WeeklyGiving/config.json");
    }

    if (file.open(QFile::WriteOnly | QFile::Text)) {
        QString settingsString = settingsJson.toJson();
        file.write(settingsString.toUtf8());
    } else {
        QMessageBox::critical(
            this,
            "Error Opening Settings",
            "There was an error opening the settings file. Your setting may not have been saved."
        );
    }
}

void MainWindow::editSettings() {
    /**
     * Creates and shows a dialog where the program settings can be changed. Updates and saves the settings on User OK.
     * @return void
     */
    QDialog editDialog;
    editDialog.setObjectName("container");
    editDialog.setMinimumWidth(500);
    QGridLayout editLayout(&editDialog);

    QFont settingsTitleFont(mTitleFont.family(), mTitleFont.pointSize(), QFont::Bold);
    settingsTitleFont.setUnderline(true);

    QLabel specialLabel("Special Giving Categories:");
    specialLabel.setFont(settingsTitleFont);
    specialLabel.setStyleSheet("margin-top: 20px; margin-bottom: 10px;");
    editLayout.addWidget(&specialLabel, 0, 0, 1, 2);

    QLabel currentLabel("Current Label:");
    currentLabel.setFont(mBoldFont);
    editLayout.addWidget(&currentLabel, 1, 0);

    QLabel newLabel("New Label");
    newLabel.setFont(mBoldFont);
    editLayout.addWidget(&newLabel, 1, 1);

    QJsonObject specialDesignations = settingsJson.object()["specialDesignations"].toObject();

    QLabel spec1Label(specialDesignations["spec1"].toString());
    spec1Label.setFont(mBoldFont);
    editLayout.addWidget(&spec1Label, 2, 0);

    QLineEdit spec1LineEdit(specialDesignations["spec1"].toString());
    spec1LineEdit.setFont(mStandardFont);
    editLayout.addWidget(&spec1LineEdit, 2, 1);

    QLabel spec2Label(specialDesignations["spec2"].toString());
    spec2Label.setFont(mBoldFont);
    editLayout.addWidget(&spec2Label, 3, 0);

    QLineEdit spec2LineEdit(specialDesignations["spec2"].toString());
    spec2LineEdit.setFont(mStandardFont);
    editLayout.addWidget(&spec2LineEdit, 3, 1);

    QLabel spec3Label(specialDesignations["spec3"].toString());
    spec3Label.setFont(mBoldFont);
    editLayout.addWidget(&spec3Label, 4, 0);

    QLineEdit spec3LineEdit(specialDesignations["spec3"].toString());
    spec3LineEdit.setFont(mStandardFont);
    editLayout.addWidget(&spec3LineEdit, 4, 1);

    QLabel spec4Label(specialDesignations["spec4"].toString());
    spec4Label.setFont(mBoldFont);
    editLayout.addWidget(&spec4Label, 5, 0);

    QLineEdit spec4LineEdit(specialDesignations["spec4"].toString());
    spec4LineEdit.setFont(mStandardFont);
    editLayout.addWidget(&spec4LineEdit, 5, 1);

    QLabel spec5Label(specialDesignations["spec5"].toString());
    spec5Label.setFont(mBoldFont);
    editLayout.addWidget(&spec5Label, 6, 0);

    QLineEdit spec5LineEdit(specialDesignations["spec5"].toString());
    spec5LineEdit.setFont(mStandardFont);
    editLayout.addWidget(&spec5LineEdit, 6, 1);

    QLabel spec6Label(specialDesignations["spec6"].toString());
    spec6Label.setFont(mBoldFont);
    editLayout.addWidget(&spec6Label, 7, 0);

    QLineEdit spec6LineEdit(specialDesignations["spec6"].toString());
    spec6LineEdit.setFont(mStandardFont);
    editLayout.addWidget(&spec6LineEdit, 7, 1);

    QLabel spec7Label(specialDesignations["spec7"].toString());
    spec7Label.setFont(mBoldFont);
    editLayout.addWidget(&spec7Label, 8, 0);

    QLineEdit spec7LineEdit(specialDesignations["spec7"].toString());
    spec7LineEdit.setFont(mStandardFont);
    editLayout.addWidget(&spec7LineEdit, 8, 1);

    QLabel otherSettingsLabel("Other Settings");
    editLayout.addWidget(&otherSettingsLabel, 9, 0, 1, 2);
    otherSettingsLabel.setFont(settingsTitleFont);
    otherSettingsLabel.setStyleSheet("margin-top: 20px; margin-bottom: 10px;");

    QLabel nameLabel("Church Name");
    editLayout.addWidget(&nameLabel, 10, 0);
    nameLabel.setFont(mBoldFont);
    nameLabel.setStyleSheet("margin-top: 20px;");

    QLineEdit nameLineEdit(settingsJson.object()["name"].toString());
    nameLineEdit.setFont(mStandardFont);
    editLayout.addWidget(&nameLineEdit, 10, 1);

    QLabel includeSpecialLabel("Include Special Offerings in Total");
    editLayout.addWidget(&includeSpecialLabel, 11, 0);
    includeSpecialLabel.setFont(mBoldFont);
    includeSpecialLabel.setStyleSheet("margin-top: 20px;");

    QCheckBox includeSpecialCheckBox;
    includeSpecialCheckBox.setStyleSheet("margin-top: 20px;");
    includeSpecialCheckBox.setChecked(settingsJson.object()["includeSpecial"].toBool());
    editLayout.addWidget(&includeSpecialCheckBox, 11, 1);

    QWidget buttonWidget;
    editLayout.addWidget(&buttonWidget, 12, 0, 1, 2);
    buttonWidget.setStyleSheet("margin-top: 20px;");

    QHBoxLayout buttonLayout(&buttonWidget);

    QPushButton okButton("Ok");
    buttonLayout.addStretch();
    buttonLayout.addWidget(&okButton);
    buttonLayout.addSpacing(20);
    okButton.setFont(mStandardFont);
    connect(&okButton, &QPushButton::clicked, this, [&] { editDialog.done(0); });

    QPushButton cancelButton("Cancel");
    buttonLayout.addWidget(&cancelButton);
    buttonLayout.addStretch();
    cancelButton.setFont(mStandardFont);
    connect(&cancelButton, &QPushButton::clicked, this, [&] { editDialog.done(1); });

    int result = editDialog.exec();
    if (result == 0) {
        // Create JSON containing the settings currently shown in the dialog
        QJsonObject newSettings = settingsJson.object();
        QJsonObject specialDesignations = newSettings["specialDesignations"].toObject();
        specialDesignations["spec1"] = spec1LineEdit.text();
        specialDesignations["spec2"] = spec2LineEdit.text();
        specialDesignations["spec3"] = spec3LineEdit.text();
        specialDesignations["spec4"] = spec4LineEdit.text();
        specialDesignations["spec5"] = spec5LineEdit.text();
        specialDesignations["spec6"] = spec6LineEdit.text();
        specialDesignations["spec7"] = spec7LineEdit.text();
        newSettings["specialDesignations"] = specialDesignations;
        newSettings["name"] = nameLineEdit.text();
        newSettings["includeSpecial"] = includeSpecialCheckBox.isChecked();
        settingsJson.setObject(newSettings);

        QList<QLabel*> specLabels = mSpecialWidget->findChildren<QLabel*>();
        for (int i = 1; i < specialDesignations.keys().size(); i++) {
            QString key = specialDesignations.keys()[i - 1];
            specLabels[i]->setText(specialDesignations[key].toString());
        }

        // Recalculate the totals in case include special was changed
        calculateTotals();
        saveSettings();
    }
}

void MainWindow::initComponents() {
    /**
     * Creates and lays out the visual components of the GUI
     * @return void
     */
    setWindowTitle("Weekly Giving v." + version);
    setWindowIcon(QIcon(":/resources/icon.png"));

    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    auto* centralLayout = new QVBoxLayout(centralWidget);
    centralLayout->addWidget(makeToolbarWidget());
    centralLayout->addWidget(makeInfoWidget());

    auto* dataWidget = new QWidget();
    centralLayout->addWidget(dataWidget);
    auto* dataLayout = new QGridLayout(dataWidget);
    dataLayout->setColumnStretch(0, 2);
    dataLayout->setColumnStretch(1, 2);
    dataLayout->setColumnStretch(2, 3);
    dataLayout->setColumnStretch(3, 3);
    dataLayout->setContentsMargins(0, 0, 0, 0);

    dataLayout->addWidget(makeCashWidget(), 0, 0);
    mChecksWidget = makeChecksWidget();
    dataLayout->addWidget(mChecksWidget, 0, 1);
    mSpecialWidget = makeSpecialWidget();
    dataLayout->addWidget(mSpecialWidget, 0, 2);
    dataLayout->addWidget(makeNotesWidget(), 0, 3);

    auto* statusWidget = new QWidget();
    centralLayout->addWidget(statusWidget);
    auto* statusLayout = new QHBoxLayout(statusWidget);

    mStatusLabel = new QLabel("Changes Saved");
    mStatusLabel->setObjectName("statusLabel");
    statusLayout->addWidget(mStatusLabel);
    statusLayout->addStretch();
    mStatusLabel->setFont(mStandardFont);

    showMaximized();
}

QWidget* MainWindow::makeToolbarWidget() {
    /**
     * Creates the toolbar widget for the top of the GUI
     */
    auto* toolbarWidget = new QWidget();
    auto* toolbarLayout = new QHBoxLayout(toolbarWidget);
    toolbarLayout->setContentsMargins(0, 10, 0, 10);

    // Singular, consistent size for all button icons
    QSize iconSize(32, 32);

    auto* firstRecButton = new QPushButton();
    toolbarLayout->addWidget(firstRecButton);
    firstRecButton->setIcon(QIcon(":/resources/firstrec.png"));
    firstRecButton->setToolTip("Go to the first record");
    firstRecButton->setIconSize(iconSize);
    connect(
        firstRecButton,
        &QPushButton::clicked,
        this,
        [this] {
            bool goOn = checkChanges();
            // Only go to the first record if the user OKs it
            if (goOn) {
                QList<QPair<QString, QVariant>> resultSet = getById(mIds.first());
                populateData(resultSet);
            }
        }
    );

    auto* prevRecButton = new QPushButton();
    toolbarLayout->addWidget(prevRecButton);
    prevRecButton->setIcon(QIcon(":/resources/prevrec.png"));
    prevRecButton->setToolTip("Go to the previous record");
    prevRecButton->setIconSize(iconSize);
    connect(
        prevRecButton,
        &QPushButton::clicked,
        this,
        [this] {
            // Only go to the previous record if the user OKs it
            bool goOn = checkChanges();
            if (goOn) {
                QString currentId = mIdLabel->text();
                if (currentId.isEmpty()) return;
                int idIndex = mIds.indexOf(currentId);
                if (idIndex > 0) {
                QList<QPair<QString, QVariant>> resultSet = getById(mIds[idIndex - 1]);
                populateData(resultSet);
            }
            }
        }
    );

    auto* nextRecButton = new QPushButton();
    toolbarLayout->addWidget(nextRecButton);
    nextRecButton->setIcon(QIcon(":/resources/nextrec.png"));
    nextRecButton->setToolTip("Go to the next record");
    nextRecButton->setIconSize(iconSize);
    connect(
        nextRecButton,
        &QPushButton::clicked,
        this,
        [this] {
            bool goOn = checkChanges();
            // Only go to the next record if the user OKs it
            if (goOn) {
                QString currentId = mIdLabel->text();
                if (currentId.isEmpty()) return;
                int idIndex = mIds.indexOf(currentId);
                if (idIndex < mIds.length() - 1) {
                QList<QPair<QString, QVariant>> resultSet = getById(mIds[idIndex + 1]);
                populateData(resultSet);
            }
            }
        }
    );

    auto* lastRecButton = new QPushButton();
    toolbarLayout->addWidget(lastRecButton);
    lastRecButton->setIcon(QIcon(":/resources/lastrec.png"));
    lastRecButton->setToolTip("Go to the last record");
    lastRecButton->setIconSize(iconSize);
    connect(
        lastRecButton,
        &QPushButton::clicked,
        this,
        [this] {
            bool goOn = checkChanges();
            // Only go to the last record if the user OKs it
            if (goOn) {
                QList<QPair<QString, QVariant>> resultSet = getById(mIds.last());
                populateData(resultSet);
            }
        }
    );

    auto* newRecButton = new QPushButton();
    toolbarLayout->addWidget(newRecButton);
    newRecButton->setIcon(QIcon(":/resources/newrec.png"));
    newRecButton->setToolTip("Create a new record");
    newRecButton->setIconSize(iconSize);

    auto* deleteRecButton = new QPushButton();
    toolbarLayout->addSpacing(20);
    toolbarLayout->addWidget(deleteRecButton);
    deleteRecButton->setIcon(QIcon(":/resources/x_icon.png"));
    deleteRecButton->setToolTip("Delete this record");
    deleteRecButton->setIconSize(iconSize);

    toolbarLayout->addStretch(1);

    auto* idLabel = new QLabel("Choose Record by ID:");
    toolbarLayout->addWidget(idLabel);
    idLabel->setObjectName("toolbarLabel");
    idLabel->setFont(mBoldFont);

    auto* idComboBox = new QComboBox;
    idComboBox->setObjectName("idComboBox");
    idComboBox->setFont(mStandardFont);
    idComboBox->setFixedWidth(100);
    connect(
        idComboBox,
        &QComboBox::currentTextChanged,
        this,
        [this, idComboBox] {
            bool goOn = checkChanges();
            if (goOn) {
                populateData(getById(idComboBox->currentText()));
            }
        }
    );
    toolbarLayout->addWidget(idComboBox);

    auto* dateLabel = new QLabel("Choose Record by Date:");
    toolbarLayout->addWidget(dateLabel);
    dateLabel->setObjectName("toolbarLabel");
    dateLabel->setFont(mBoldFont);

    auto* dateComboBox = new QComboBox;
    dateComboBox->setObjectName("dateComboBox");
    dateComboBox->setFont(mStandardFont);
    dateComboBox->setFixedWidth(150);
    connect(
        dateComboBox,
        &QComboBox::currentTextChanged,
        this,
        [this, dateComboBox] {
            bool goOn = checkChanges();
            if (goOn) {
                populateData(getByDate(dateComboBox->currentText()));
            }
        }
    );
    toolbarLayout->addWidget(dateComboBox);

    toolbarLayout->addStretch(1);

    auto* printRecButton = new QPushButton();
    toolbarLayout->addWidget(printRecButton);
    printRecButton->setIcon(QIcon(":/resources/printrec.png"));
    printRecButton->setToolTip("Print this record");
    printRecButton->setIconSize(iconSize);
    connect(
        printRecButton,
        &QPushButton::clicked,
        this,
        [this] { printRecord(); }
    );

    auto* saveButton = new QPushButton();
    toolbarLayout->addWidget(saveButton);
    saveButton->setIcon(QIcon(":/resources/saverec.png"));
    saveButton->setToolTip("Save record");
    saveButton->setIconSize(iconSize);
    connect(
        saveButton,
        &QPushButton::clicked,
        this,
        [this] { saveRecord(); }
    );

    auto* graphButton = new QPushButton();
    toolbarLayout->addWidget(graphButton);
    graphButton->setIcon(QIcon(":/resources/graph.png"));
    graphButton->setToolTip("Create a graph of giving for a given date range");
    graphButton->setIconSize(iconSize);
    connect(
        graphButton,
        &QPushButton::clicked,
        this,
        [this] { makeGraph(); }
    );

    auto* settingsButton = new QPushButton();
    toolbarLayout->addWidget(settingsButton);
    settingsButton->setIcon(QIcon(":/resources/settings.png"));
    settingsButton->setToolTip("Edit settings");
    settingsButton->setIconSize(iconSize);
    connect(settingsButton, &QPushButton::clicked, this, [this] { editSettings(); });

    connect(
        newRecButton,
        &QPushButton::clicked,
        this,
        [this] {
            bool goOn = checkChanges();
            // Only create a new record if the user OKs it
            if (goOn) {
                QString newId = createNewRecord();

                if (newId.isEmpty()) {
                    QMessageBox::information(
                        this,
                        "Database Error",
                        "There was an error creating the new record."
                    );
                    return;
                }

                QList<QPair<QString, QVariant>> resultSet = getById(newId);
                populateData(resultSet);
                populateComboBoxes();
            }
        }
    );

    connect(
        deleteRecButton,
        &QPushButton::clicked,
        this,
        [this] {
            int response = QMessageBox::question(
                this,
                "Really Delete?",
                "You are about do delete the current record. This cannot be undone. Continue?",
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel
            );
            if (response != QMessageBox::Yes) return;

            QString currentId = mIdLabel->text();
            QString result = deleteRec(currentId);
            if (!result.isEmpty()) {
                QMessageBox::information(
                    this,
                    "Database Error",
                    "There was an error deleting the record: " + result + "."
                );
            } else {
                // Load the record prior to the one that was deleted
                int previousIdIndex = mIds.indexOf(currentId) - 1;
                if (previousIdIndex < 0) previousIdIndex = 0;

                QList<QPair<QString, QVariant>> resultSet = getById(mIds[previousIdIndex]);
                populateData(resultSet);
                populateComboBoxes();
            }
        }
    );

    return toolbarWidget;
}

QWidget* MainWindow::makeInfoWidget() {
    /**
     * Creates the widget containing the record ID number, the date, and the "prepared by" info
     * @return the info widget
     */
    auto* infoWidget = new QWidget;
    infoWidget->setObjectName("container");
    auto* infoLayout = new QHBoxLayout(infoWidget);

    auto* idTitleLabel = new QLabel("ID:");
    infoLayout->addWidget(idTitleLabel);
    idTitleLabel->setFont(mBoldFont);

    mIdLabel = new QLabel();
    infoLayout->addWidget(mIdLabel);
    mIdLabel->setFont(mStandardFont);

    infoLayout->addSpacing(20);

    auto* dateTitleLabel = new QLabel("Date:");
    infoLayout->addWidget(dateTitleLabel);
    dateTitleLabel->setFont(mBoldFont);

    mDateWidget = new QDateEdit;
    infoLayout->addWidget(mDateWidget);
    mDateWidget->setCalendarPopup(true);
    mDateWidget->setFont(mStandardFont);

    auto* preparedLabel = new QLabel("Prepared by:");
    infoLayout->addWidget(preparedLabel);
    preparedLabel->setFont(mBoldFont);

    mPreparedLineEdit = new QLineEdit;
    infoLayout->addWidget(mPreparedLineEdit);
    mPreparedLineEdit->setFont(mStandardFont);
    mPreparedLineEdit->setMaximumWidth(300);

    infoLayout->addStretch();

    return infoWidget;
}

QWidget* MainWindow::makeCashWidget() {
    /**
     * Creates the widget containing cash donations
     * @return the cash widget
     */
    int lineEditMaxWidth = 50;

    auto* cashWidget = new QWidget(this);
    cashWidget->setObjectName("container");

    auto* cashLayout = new QVBoxLayout(cashWidget);
    cashLayout->setSpacing(20);

    auto* cashHeaderLabel = new QLabel("Cash");
    cashLayout->addWidget(cashHeaderLabel);
    cashHeaderLabel->setFont(mHeaderFont);
    cashHeaderLabel->setObjectName("headerLabel");

    auto* billsWidget = new QGroupBox(cashWidget);
    billsWidget->setTitle("Bills");
    billsWidget->setFont(mTitleFont);
    cashLayout->addWidget(billsWidget);

    auto* billsLayout = new QGridLayout(billsWidget);
    billsLayout->setColumnStretch(0, 1);
    billsLayout->setColumnStretch(1, 2);

    auto* hundredsLabel = new QLabel("Hundreds");
    hundredsLabel->setFont(mBoldFont);
    billsLayout->addWidget(hundredsLabel, 0, 0);

    mHundredsLineEdit = new QLineEdit(billsWidget);
    mHundredsLineEdit->setFont(mStandardFont);
    mHundredsLineEdit->setMaximumWidth(lineEditMaxWidth);
    billsLayout->addWidget(mHundredsLineEdit, 0, 1);

    auto* fiftiesLabel = new QLabel("Fifties");
    fiftiesLabel->setFont(mBoldFont);
    billsLayout->addWidget(fiftiesLabel, 1, 0);

    mFiftiesLineEdit = new QLineEdit(billsWidget);
    mFiftiesLineEdit->setFont(mStandardFont);
    mFiftiesLineEdit->setMaximumWidth(lineEditMaxWidth);
    billsLayout->addWidget(mFiftiesLineEdit, 1, 1);

    auto* twentiesLabel = new QLabel("Twenties");
    twentiesLabel->setFont(mBoldFont);
    billsLayout->addWidget(twentiesLabel, 2, 0);

    mTwentiesLineEdit = new QLineEdit(billsWidget);
    mTwentiesLineEdit->setFont(mStandardFont);
    mTwentiesLineEdit->setMaximumWidth(lineEditMaxWidth);
    billsLayout->addWidget(mTwentiesLineEdit, 2, 1);

    auto* tensLabel = new QLabel("Tens");
    tensLabel->setFont(mBoldFont);
    billsLayout->addWidget(tensLabel, 3, 0);

    mTensLineEdit = new QLineEdit(billsWidget);
    mTensLineEdit->setFont(mStandardFont);
    mTensLineEdit->setMaximumWidth(lineEditMaxWidth);
    billsLayout->addWidget(mTensLineEdit, 3, 1);

    auto* fivesLabel = new QLabel("Fives");
    fivesLabel->setFont(mBoldFont);
    billsLayout->addWidget(fivesLabel, 4, 0);

    mFivesLineEdit = new QLineEdit(billsWidget);
    mFivesLineEdit->setFont(mStandardFont);
    mFivesLineEdit->setMaximumWidth(lineEditMaxWidth);
    billsLayout->addWidget(mFivesLineEdit, 4, 1);

    auto* onesLabel = new QLabel("Ones");
    onesLabel->setFont(mBoldFont);
    billsLayout->addWidget(onesLabel, 5, 0);

    mOnesLineEdit = new QLineEdit(billsWidget);
    mOnesLineEdit->setFont(mStandardFont);
    mOnesLineEdit->setMaximumWidth(lineEditMaxWidth);
    billsLayout->addWidget(mOnesLineEdit, 5, 1);

    auto* coinsWidget = new QGroupBox(cashWidget);
    coinsWidget->setTitle("Coins");
    coinsWidget->setFont(mTitleFont);
    cashLayout->addWidget(coinsWidget);

    auto* coinsLayout = new QGridLayout(coinsWidget);
    coinsLayout->setColumnStretch(0, 1);
    coinsLayout->setColumnStretch(1, 2);

    auto* dollarsLabel = new QLabel("Dollars");
    coinsLayout->addWidget(dollarsLabel, 0, 0);
    dollarsLabel->setFont(mBoldFont);

    mDollarsLineEdit = new QLineEdit(coinsWidget);
    mDollarsLineEdit->setFont(mStandardFont);
    mDollarsLineEdit->setMaximumWidth(lineEditMaxWidth);
    coinsLayout->addWidget(mDollarsLineEdit, 0, 1);

    auto* quartersLabel = new QLabel("Quarters");
    quartersLabel->setFont(mBoldFont);
    coinsLayout->addWidget(quartersLabel, 1, 0);

    mQuartersLineEdit = new QLineEdit(coinsWidget);
    mQuartersLineEdit->setFont(mStandardFont);
    mQuartersLineEdit->setMaximumWidth(lineEditMaxWidth);
    coinsLayout->addWidget(mQuartersLineEdit, 1, 1);

    auto* dimesLabel = new QLabel("Dimes");
    dimesLabel->setFont(mBoldFont);
    coinsLayout->addWidget(dimesLabel, 2, 0);

    mDimesLineEdit = new QLineEdit(coinsWidget);
    mDimesLineEdit->setFont(mStandardFont);
    mDimesLineEdit->setMaximumWidth(lineEditMaxWidth);
    coinsLayout->addWidget(mDimesLineEdit, 2, 1);

    auto* nickelsLabel = new QLabel("Nickels");
    nickelsLabel->setFont(mBoldFont);
    coinsLayout->addWidget(nickelsLabel, 3, 0);

    mNickelsLineEdit = new QLineEdit(coinsWidget);
    mNickelsLineEdit->setFont(mStandardFont);
    mNickelsLineEdit->setMaximumWidth(lineEditMaxWidth);
    coinsLayout->addWidget(mNickelsLineEdit, 3, 1);

    auto* penniesLabel = new QLabel("Pennies");
    penniesLabel->setFont(mBoldFont);
    coinsLayout->addWidget(penniesLabel, 4, 0);

    mPenniesLineEdit = new QLineEdit(coinsWidget);
    mPenniesLineEdit->setFont(mStandardFont);
    mPenniesLineEdit->setMaximumWidth(lineEditMaxWidth);
    coinsLayout->addWidget(mPenniesLineEdit, 4, 1);

    cashLayout->addStretch();

    return cashWidget;
}

QWidget* MainWindow::makeChecksWidget() {
    /**
     * Makes the widget containing the list of checks
     * @return the checks widget
     */
    auto* checksWidget = new QWidget();
    checksWidget->setObjectName("container");
    auto* checksLayout = new QVBoxLayout(checksWidget);

    auto* checksTitleLabel = new QLabel("Checks");
    checksTitleLabel->setObjectName("headerLabel");
    checksTitleLabel->setFont(mHeaderFont);
    checksLayout->addWidget(checksTitleLabel);
    checksLayout->addSpacing(20);

    mChecksContainer = new QWidget();
    mChecksContainer->setObjectName("container");
    checksContainerLayout = new QVBoxLayout(mChecksContainer);
    checksContainerLayout->addStretch();

    auto* addCheckButton = new QPushButton("Add Check");
    checksLayout->addWidget(addCheckButton);
    addCheckButton->setFont(mStandardFont);
    addCheckButton->setMaximumWidth(100);
    connect(addCheckButton, &QPushButton::clicked, this, &MainWindow::addCheck);

    auto* scrollArea = new QScrollArea();
    scrollArea->setObjectName("checksScrollArea");
    checksLayout->addWidget(scrollArea);
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(mChecksContainer);

    return checksWidget;
}

void MainWindow::addCheck(double value) {
    /**
     * Creates an individual check widget to be added to the checks container
     * @return void
     */
    auto* checkWidget = new QWidget();
    checkWidget->setObjectName("checkWidget");
    auto* checkLayout = new QHBoxLayout(checkWidget);
    checkLayout->setContentsMargins(0, 0, 0, 0);

    // Label for numbering the check
    auto* checkLabel = new QLabel();
    checkLabel->setObjectName("checkLabel");
    checkLabel->setFont(mBoldFont);
    checkLayout->addWidget(checkLabel);

    // Line edit for inputting the amount of the check
    auto* checkLineEdit = new AutoSelectLineEdit(checkWidget, this);
    checkLineEdit->setObjectName("checkLineEditLast");
    checkLineEdit->setFont(mStandardFont);
    checkLineEdit->isCurrency = true;
    checkLineEdit->setMaximumWidth(100);
    checkLayout->addWidget(checkLineEdit);

    // Button to allow deleting this check
    auto* deleteButton = new QPushButton();
    checkLayout->addWidget(deleteButton);
    deleteButton->setIcon(QIcon(":/resources/x_icon.svg"));
    deleteButton->setIconSize(QSize(12, 12));
    deleteButton->setMaximumSize(QSize(16, 16));

    connect(deleteButton, &QPushButton::clicked, this, [this] {
        // Empty the object name of the check widget to be deleted so it is not found when renumbering
        sender()->parent()->setObjectName("");
        sender()->parent()->deleteLater();
        int checkNumber = 1;
        // Recurse the checks widget to renumber the individual check labels
        for (const QWidget* thisCheckWidget : mChecksWidget->findChildren<QWidget*>("checkWidget")) {
            auto* thisCheckLabel = thisCheckWidget->findChild<QLabel*>("checkLabel");
            thisCheckLabel->setText("Check " + QString::number(checkNumber));
            checkNumber++;
        }
    });

    // Recalculate the totals as the user edits
    connect(checkLineEdit, &QLineEdit::textChanged, this, [this] { calculateTotals(); });
    connect(checkLineEdit, &QLineEdit::textEdited, this, [this] { calculateTotals(); });

    // Recurse the checks widget to determine the next check number
    int nextCheckNumber = 1;
    for (const QWidget* thisCheckWidget : mChecksWidget->findChildren<QWidget*>("checkWidget")) {
        auto* thisCheckLabel = thisCheckWidget->findChild<QLabel*>("checkLabel");
        QString text = thisCheckLabel->text();
        QList textSplit = text.split(" ");
        nextCheckNumber = textSplit.last().toInt() + 1;
    }

    checkLabel->setText("Check " + QString::number(nextCheckNumber));
    checkLineEdit->setText(QString::asprintf("$%.2f", value));

    // Ensure the widget is inserted at the bottom of the list of checks
    int insertIndex = checksContainerLayout->count() - 1;
    if (insertIndex < 0) insertIndex = 0;

    // Ensure that none of the current check widgets carry the "checkLineEditLast" name
    for (AutoSelectLineEdit* lineEdit : mChecksWidget->findChildren<AutoSelectLineEdit*>()) {
        lineEdit->setObjectName("checkLineEdit");
    }
    checksContainerLayout->insertWidget(insertIndex, checkWidget);
}

QWidget* MainWindow::makeSpecialWidget() {
    /**
     * Creates the widget containing the specially-designated offerings
     * @return the special offering widget
     */
    auto* specialWidget = new QWidget();
    specialWidget->setObjectName("checksWidget");
    auto* specialLayout = new QVBoxLayout(specialWidget);
    specialLayout->setSpacing(10);

    auto* specialTitleLabel = new QLabel("Special Designations");
    specialTitleLabel->setObjectName("headerLabel");
    specialTitleLabel->setFont(mHeaderFont);
    specialLayout->addWidget(specialTitleLabel);

    // Name each designation according to what is stored in the settings
    for (QString &key : settingsJson.object()["specialDesignations"].toObject().keys()) {
        auto* widget = new QWidget();
        specialLayout->addWidget(widget);
        auto* layout = new QHBoxLayout(widget);

        auto* label = new QLabel(settingsJson.object()["specialDesignations"].toObject()[key].toString());
        label->setFont(mBoldFont);
        layout->addWidget(label);

        auto* lineEdit = new AutoSelectLineEdit();
        lineEdit->setFont(mStandardFont);
        lineEdit->setMaximumWidth(150);
        lineEdit->isCurrency = true;
        layout->addWidget(lineEdit, Qt::AlignmentFlag::AlignRight);
    }

    specialLayout->addStretch();

    return specialWidget;
}

QWidget* MainWindow::makeNotesWidget() {
    /**
     * Create the widget containing the notes text edit and the totals widget
     * @return the notes widget
     */
    auto* notesWidget = new QWidget();
    notesWidget->setObjectName("container");
    auto* notesLayout = new QVBoxLayout(notesWidget);

    auto* notesGroupBox = new QGroupBox();
    notesLayout->addWidget(notesGroupBox);
    notesGroupBox->setTitle("Notes");
    notesGroupBox->setFont(mTitleFont);

    auto* notesGroupBoxLayout = new QVBoxLayout(notesGroupBox);

    mNotesTextEdit = new QTextEdit();
    notesGroupBoxLayout->addWidget(mNotesTextEdit);
    mNotesTextEdit->setFont(mStandardFont);
    mNotesTextEdit->setMaximumHeight(200);

    auto* totalsWidget = new QGroupBox();
    totalsWidget->setTitle("Totals");
    totalsWidget->setFont(mTitleFont);
    notesLayout->addWidget(totalsWidget);
    auto* totalsLayout = new QGridLayout(totalsWidget);

    auto* coinsTitleLabel = new QLabel("Total Coins:");
    totalsLayout->addWidget(coinsTitleLabel, 0, 0);
    coinsTitleLabel->setFont(mBoldFont);

    mTotalCoinsLabel = new QLabel();
    totalsLayout->addWidget(mTotalCoinsLabel, 0, 1);
    mTotalCoinsLabel->setFont(mStandardFont);

    auto* billsTitleLabel = new QLabel("Total Bills:");
    totalsLayout->addWidget(billsTitleLabel, 1, 0);
    billsTitleLabel->setFont(mBoldFont);

    mTotalBillsLabel = new QLabel();
    totalsLayout->addWidget(mTotalBillsLabel, 1, 1);
    mTotalBillsLabel->setFont(mStandardFont);

    auto* checksTitleLabel = new QLabel("Total Checks:");
    totalsLayout->addWidget(checksTitleLabel, 2, 0);
    checksTitleLabel->setFont(mBoldFont);

    mTotalChecksLabel = new QLabel();
    totalsLayout->addWidget(mTotalChecksLabel, 2, 1);
    mTotalChecksLabel->setFont(mStandardFont);

    auto* specialTitleLabel = new QLabel("Total Special Designations:");
    totalsLayout->addWidget(specialTitleLabel, 3, 0);
    specialTitleLabel->setFont(mBoldFont);

    mTotalSpecialLabel = new QLabel();
    totalsLayout->addWidget(mTotalSpecialLabel, 3, 1);
    mTotalSpecialLabel->setFont(mStandardFont);

    auto* totalTitleLabel = new QLabel("Total:");
    totalsLayout->addWidget(totalTitleLabel, 4, 0);
    totalTitleLabel->setFont(mBoldFont);

    mTotalLabel = new QLabel();
    totalsLayout->addWidget(mTotalLabel, 4, 1);
    mTotalLabel->setFont(mBoldFont);

    auto* numChecksTitleLabel = new QLabel("Number of Checks:");
    totalsLayout->addWidget(numChecksTitleLabel, 5, 0);
    numChecksTitleLabel->setFont(mBoldFont);

    mNumChecksLabel = new QLabel();
    totalsLayout->addWidget(mNumChecksLabel, 5, 1);
    mNumChecksLabel->setFont(mStandardFont);

    notesLayout->addStretch();

    return notesWidget;
}

void MainWindow::populateData(QList<QPair<QString, QVariant>> resultSet) {
    /**
     * Takes the result set obtained from the database and changes the GUI's values accordingly
     * @return void
     */
    mIdLabel->setText(resultSet[0].second.toString());
    mDateWidget->setDate(resultSet[1].second.toDate());
    mPreparedLineEdit->setText(resultSet[2].second.toString());

    // Set the cash values. Set the values to "0" should the string be empty.
    QString hundreds = resultSet[3].second.toString();
    if (hundreds.trimmed().isEmpty()) hundreds = "0";
    mHundredsLineEdit->setText(hundreds);
    QString fifties = resultSet[4].second.toString();
    if (fifties.trimmed().isEmpty()) fifties = "0";
    mFiftiesLineEdit->setText(fifties);
    QString twenties = resultSet[5].second.toString();
    if (twenties.trimmed().isEmpty()) twenties = "0";
    mTwentiesLineEdit->setText(twenties);
    QString tens = resultSet[6].second.toString();
    if (tens.trimmed().isEmpty()) tens = "0";
    mTensLineEdit->setText(tens);
    QString fives = resultSet[7].second.toString();
    if (fives.trimmed().isEmpty()) fives = "0";
    mFivesLineEdit->setText(fives);
    QString ones = resultSet[8].second.toString();
    if (ones.trimmed().isEmpty()) ones = "0";
    mOnesLineEdit->setText(ones);

    QString dollars = resultSet[9].second.toString();
    if (dollars.trimmed().isEmpty()) dollars = "0";
    mDollarsLineEdit->setText(dollars);
    QString quarters = resultSet[10].second.toString();
    if (quarters.trimmed().isEmpty()) quarters = "0";
    mQuartersLineEdit->setText(quarters);
    QString dimes = resultSet[11].second.toString();
    if (dimes.trimmed().isEmpty()) dimes = "0";
    mDimesLineEdit->setText(dimes);
    QString nickels = resultSet[12].second.toString();
    if (nickels.trimmed().isEmpty()) nickels = "0";
    mNickelsLineEdit->setText(nickels);
    QString pennies = resultSet[13].second.toString();
    if (pennies.trimmed().isEmpty()) pennies = "0";
    mPenniesLineEdit->setText(pennies);

    // Recurse the line edits in the special offering widget, first clearing them then setting their value
    int currentIndex = 14;
    for (QLineEdit* lineEdit : mSpecialWidget->findChildren<QLineEdit*>()) {
        lineEdit->clear();

        bool ok;
        QString valueString = resultSet[currentIndex].second.toString().replace("$", "");
        double value = valueString.toDouble(&ok);
        if (ok) {
            lineEdit->setText(QString::asprintf("$%.2f", value));
        }

        currentIndex++;
    }

    // Remove all individal check widgets in the checks container
    for (int i = checksContainerLayout->count() - 1; i >= 0; --i) {
        QLayoutItem *item = checksContainerLayout->itemAt(i);

        // Only target items that manage a QWidget
        if (item && item->widget()) {
            QLayoutItem *takenItem = checksContainerLayout->takeAt(i);
            delete takenItem->widget();
            delete takenItem;
        }
    }

    // Recurse the checks values, adding a check widget for each one
    currentIndex = 21;
    for (int i = 0; i < 30; i++) {
        QString stringValue = resultSet[currentIndex].second.toString().replace("$", "");
        bool ok;
        double value = stringValue.toDouble(&ok);
        if (ok && value > 0) {
            addCheck(value);
        }
        currentIndex ++;
    }

    mNotesTextEdit->setText(resultSet[51].second.toString());
    calculateTotals();

    // If no checks were added from this result set, add an empty check widget
    if (mNumChecksLabel->text() == "0") addCheck(0);
    centralWidget()->update();

    auto* idComboBox = findChild<QComboBox*>("idComboBox");
    idComboBox->blockSignals(true);
    idComboBox->setCurrentIndex(idComboBox->findText(resultSet[0].second.toString()));
    idComboBox->blockSignals(false);

    auto* dateComboBox = findChild<QComboBox*>("dateComboBox");
    dateComboBox->blockSignals(true);
    dateComboBox->setCurrentIndex(dateComboBox->findText(resultSet[1].second.toString()));
    dateComboBox->blockSignals(false);

    mChanges = false;
    mStatusLabel->setText("No Changes");
}

void MainWindow::populateComboBoxes() {
    // Refresh the dates and ids list and update their comboboxes
    auto [dates, ids] = getIndices();
    mDates = dates;
    mIds = ids;

    auto* idComboBox = this->findChild<QComboBox*>("idComboBox");
    idComboBox->blockSignals(true);
    idComboBox->clear();
    for (int i = mIds.size() - 1; i >= 0; i--) {
        idComboBox->addItem(mIds.at(i));
    }
    idComboBox->blockSignals(false);

    auto* dateComboBox = this->findChild<QComboBox*>("dateComboBox");
    dateComboBox->blockSignals(true);
    dateComboBox->clear();
    for (int i = mDates.size() - 1; i >= 0; i--) {
        dateComboBox->addItem(mDates.at(i));
    }
    dateComboBox->blockSignals(false);
}

void MainWindow::printRecord() {
    /**
     * Creates an instance of CustomPrintDialog and executes it
     * @return void
     */
    CustomPrintDialog printDialog(
        this,
        this,
        false,
        0.0,
        0.0
    );
    printDialog.setObjectName("printDialog");
    printDialog.exec();
}

void MainWindow::makeGraph() {
    /**
     * Creates a graph of total offerings within the user's chosen date range. First creates a dialog asking for
     * that date range then creates and displays a pixmap graph.
     * @return void
     */
    QDialog dialog(this);
    QVBoxLayout layout(&dialog);

    QLabel titleLabel("Choose date range to graph:");
    layout.addWidget(&titleLabel);
    titleLabel.setFont(mTitleFont);

    QWidget dateWidget;
    layout.addWidget(&dateWidget);
    QHBoxLayout dateLayout(&dateWidget);

    QDateEdit startDateEdit;
    dateLayout.addWidget(&startDateEdit);
    dateLayout.addSpacing(20);
    startDateEdit.setCalendarPopup(true);
    startDateEdit.setFont(mStandardFont);
    startDateEdit.setDate(QDate::currentDate());

    QDateEdit endDateEdit;
    dateLayout.addWidget(&endDateEdit);
    dateLayout.addStretch();
    endDateEdit.setCalendarPopup(true);
    endDateEdit.setFont(mStandardFont);
    endDateEdit.setDate(QDate::currentDate());

    QWidget buttonWidget;
    layout.addWidget(&buttonWidget);
    QHBoxLayout buttonLayout(&buttonWidget);

    QPushButton okButton("Ok");
    buttonLayout.addStretch();
    buttonLayout.addWidget(&okButton);
    buttonLayout.addSpacing(20);
    okButton.setFont(mStandardFont);

    connect(&okButton, &QPushButton::clicked, this, [&] { dialog.done(0); });

    QPushButton cancelButton("Cancel");
    buttonLayout.addWidget(&cancelButton);
    buttonLayout.addStretch();
    cancelButton.setFont(mStandardFont);

    connect(&cancelButton, &QPushButton::clicked, this, [&] { dialog.done(1); });

    // Don't continue if the user clicked "Cancel" or closed the dialog
    int result = dialog.exec();
    if (result == 1) {
        return;
    }

    QList<QList<QString>> reportList;
    QDate startDate = startDateEdit.date();
    QDate endDate = endDateEdit.date();

    // Recurse the date indices and add that date as well as the total deposit for that date stored in the database
    for (int i = 0; i < mDates.size(); i++) {
        QDate thisDate = QDate::fromString(mDates[i], "yyyy-MM-dd");
        if (!thisDate.isValid()) {
            thisDate = QDate::fromString(mDates[i], "M/d/yyyy");
        }
        if (endDate >= thisDate && thisDate >= startDate) {
            QList<QPair<QString, QVariant>> resultSet = getById(mIds[i]);
            QString totalDeposit = "$" + resultSet[56].second.toString().replace("$", "");
            reportList.append(QList({ thisDate.toString("M/d/yyyy"), totalDeposit}));
        }
    }

    // Don't go any further if no records appear in that date range
    if (reportList.isEmpty()) {
        QMessageBox::information(
            this,
            "No Results",
            "No records exist within the given date range"
        );
        return;
    }

    int numDates = reportList.size();

    // Determine the highest and lowest deposits in order to set the graph's range
    double highestDeposit = 0;
    double lowestDeposit = 1000000;
    for (QList<QString> data : reportList) {
        double deposit = data.last().replace("$", "").replace(",", "").trimmed().toDouble();
        if (deposit < lowestDeposit) lowestDeposit = deposit;
        if (deposit > highestDeposit) highestDeposit = deposit;
    }

    QPixmap pixmap(1100, 850);
    pixmap.fill(Qt::GlobalColor::white);
    QPainter painter(&pixmap);
    painter.setWindow(0, 0, 1100, 850);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    QFont titleFont("Arial");
    titleFont.setBold(true);
    titleFont.setPixelSize(20);

    QFont graphFont("Arial");
    graphFont.setPixelSize(12);

    QFontMetrics graphFontMetrics = painter.fontMetrics();
    // Create a rect representing the paintable area of the page, namely a rect with a 50px border from the edges
    QRect paintRect(
        50,
        50,
        1000,
        750
    );
    // Create the rect that will contain the graph's scale
    QRect scaleRect(
        50,
        100,
        1000,
        625
    );
    QPen pen(Qt::GlobalColor::black, 1, Qt::SolidLine);
    painter.setPen(pen);

    // Draw the title, which includes the user's chosen dates. \u2014 is an Em-Dash.
    painter.setFont(titleFont);
    QString text = "Weekly Deposits: " + reportList.first().first() + "\u2014" + reportList.last().first();
    QRect textRect = painter.fontMetrics().tightBoundingRect(text);
    painter.drawText(paintRect.left(), paintRect.top() + textRect.height(), text);

    painter.setFont(graphFont);

    // Draw the y-axis scale representing the amount of each deposit. Step the scale by 200, rounding the highest
    // value up to the nearest 200 and the lowest value down to the nearest 200.
    int scaleTop = (std::round(highestDeposit / 100) * 100) + 100;
    if (scaleTop % 200 != 0) scaleTop += 100;
    int scaleBottom = (std::round(lowestDeposit / 100) * 100) - 100;
    if (scaleBottom % 200 != 0) scaleBottom -= 100;
    if (scaleBottom < 0) scaleBottom = 0;

    int numScale = (scaleTop - scaleBottom) / 200;
    int verticalScaleSpacing = std::round(scaleRect.height() / numScale);

    double scaleToPixelRatio = 200.0 / verticalScaleSpacing;
    int textWidth = graphFontMetrics.boundingRect("00-00-0000").width();

    int xpos = scaleRect.left();
    int ypos = scaleRect.bottom();
    // Recurse scaleBottom to scaleTop by 200s and draw the value
    for (int i = scaleBottom; i <= scaleTop; i += 200) {
        painter.drawText(xpos, ypos, QString::number(i));
        painter.drawLine(
            xpos + textWidth,
            ypos,
            scaleRect.right(),
            ypos
        );
        ypos -= verticalScaleSpacing;
    }

    QBrush brush(Qt::GlobalColor::blue);
    painter.setBrush(brush);

    // Draw the x-axis dates and the corresponding point for the total deposit for that date
    xpos = scaleRect.left() + textWidth;
    ypos = scaleRect.bottom();
    int horizontalScaleSpacing = (scaleRect.right() - xpos) / numDates;
    xpos += horizontalScaleSpacing / 2;
    int bulletSize = 8;
    QPoint previousBullet;
    for (QList data : reportList) {
        // Get a double representation of the string total deposit
        double value = data.last().replace("$", "").replace(",", "").trimmed().toDouble();

        // draw the dates as vertical text
        text = data.first();
        textRect = graphFontMetrics.boundingRect(text);
        painter.save();
        painter.translate(xpos + (textRect.height() / 2), scaleRect.bottom() + textRect.width() + 10);
        painter.rotate(-90);
        painter.drawText(0, 0, text);
        painter.restore();

        // Draw the point for the value using the scaleToPixelRation value to properly position it vertically
        pen.setColor(Qt::GlobalColor::blue);
        painter.setPen(pen);
        int yOffset = ypos - ((value - scaleBottom) / scaleToPixelRatio);
        painter.drawEllipse(xpos, yOffset - (bulletSize / 2), bulletSize, bulletSize);

        // Draw the line connecting this point to the previous point
        QPoint thisBullet(xpos + (bulletSize / 2), yOffset);
        if (previousBullet.x() != 0 && previousBullet.y() != 0) {
            painter.drawLine(previousBullet, thisBullet);
        }
        previousBullet = thisBullet;

        // Label the point with the value
        QString text = QString::asprintf("$%.2f", value);
        QRect textRect = painter.fontMetrics().boundingRect(text);

        QRect translatedTextRect(
            textRect.translated(xpos + 4 - (textRect.width() / 2),
                yOffset + 8 + textRect.height())
            );
        painter.fillRect(translatedTextRect, Qt::GlobalColor::white);

        pen.setColor(Qt::GlobalColor::black);
        painter.setPen(pen);
        painter.drawText(
            translatedTextRect.left(),
            translatedTextRect.bottom(),
            text
        );

        xpos += horizontalScaleSpacing;
    }

    painter.end();
    QDialog previewDialog(this);
    QVBoxLayout previewLayout(&previewDialog);
    QLabel label;
    previewLayout.addWidget(&label);
    label.setPixmap(pixmap);
    previewDialog.exec();
}

void MainWindow::saveRecord() {
    /**
     * Creates an sql statement by pulling all the values currently stored in the GUI. Passes that statement to
     * the saveRec function.
     * @return void
     */
    QString sql = "UPDATE weekly_giving SET ";
    sql += "id=" + mIdLabel->text() + ", ";
    sql += "date=\"" + mDateWidget->date().toString("yyyy-MM-dd") + "\", ";
    sql += "prepared_by=\"" + mPreparedLineEdit->text() + "\", ";

    sql += "bills_100=\"" + mHundredsLineEdit->text() + "\", ";
    sql += "bills_50=\"" + mFiftiesLineEdit->text() + "\", ";
    sql += "bills_20=\"" + mTwentiesLineEdit->text() + "\", ";
    sql += "bills_10=\"" + mTensLineEdit->text() + "\", ";
    sql += "bills_5=\"" + mFivesLineEdit->text() + "\", ";
    sql += "bills_1=\"" + mOnesLineEdit->text() + "\", ";

    sql += "coins_100=\"" + mDollarsLineEdit->text() + "\", ";
    sql += "coins_25=\"" + mQuartersLineEdit->text() + "\", ";
    sql += "coins_10=\"" + mDimesLineEdit->text() + "\", ";
    sql += "coins_5=\"" + mNickelsLineEdit->text() + "\", ";
    sql += "coins_1=\"" + mPenniesLineEdit->text() + "\", ";

    int columnNumber = 1;
    for (QLineEdit* lineEdit : mSpecialWidget->findChildren<QLineEdit*>()) {
        sql += "spec" + QString::number(columnNumber) + "=\" " + lineEdit->text() + "\", ";
        columnNumber++;
    }

    columnNumber = 0;
    for (QLineEdit* lineEdit : mChecksContainer->findChildren<QLineEdit*>()) {
        sql += "checks_" + QString::number(columnNumber) + "=\" " + lineEdit->text() + "\", ";
        columnNumber++;
    }

    sql += "notes=\"" + mNotesTextEdit->toPlainText().replace("\"", "\"\"") + "\", ";
    sql += "total_designated_offerings=\"" + mTotalSpecialLabel->text() + "\", ";
    sql += "coins_total=\"" + mTotalCoinsLabel->text() + "\", ";
    sql += "bills_total=\"" + mTotalBillsLabel->text() + "\", ";
    sql += "checks_total=\"" + mTotalChecksLabel->text() + "\", ";
    sql += "total_deposit=\"" + mTotalLabel->text() + "\", ";
    sql += "quantity_of_checks=\"" + mNumChecksLabel->text() + "\" ";
    sql += "WHERE id=" + mIdLabel->text() + ";";

    QString result = saveRec(sql);
    if (!result.isEmpty()) {
        QMessageBox::information(
            this,
            "Database Error",
            "There was a problem saving this record: " + result
        );
    } else {
        QMessageBox::information(
            this,
            "Record Saved",
            "Record has been saved."
        );
        mChanges = false;
        mStatusLabel->setText("Changes Saved");
    }
}

void MainWindow::addListeners() {
    /**
     * Adds connections for all lineEdit/autoSelectLineEdit textChanged and textEdited events.
     */
    QList lineEdits = findChildren<QLineEdit*>();
    for (QLineEdit* lineEdit : lineEdits) {
        connect(lineEdit, &QLineEdit::textChanged, this, [this] {
            calculateTotals();
            mStatusLabel->setText("Unsaved Changes");
            mChanges = true;
        });
        connect(lineEdit, &QLineEdit::textEdited, this, [this] {
            calculateTotals();
            mStatusLabel->setText("Unsaved Changes");
            mChanges = true;
        });
    }
    QList autoSelectLineEdits = findChildren<AutoSelectLineEdit*>();
    for (QLineEdit* lineEdit : autoSelectLineEdits) {
        connect(lineEdit, &QLineEdit::textChanged, this, [this] {
            calculateTotals();
            mStatusLabel->setText("Unsaved Changes");
            mChanges = true;
        });
        connect(lineEdit, &QLineEdit::textEdited, this, [this] {
            calculateTotals();
            mStatusLabel->setText("Unsaved Changes");
            mChanges = true;
        });
    }
}

void MainWindow::calculateTotals() {
    /**
     * Pulls all values stored in the GUI and calculates the totals for each type, updating the appropriate total
     * labels.
     * @return void
     */
    double billsTotal = 0.0;
    bool ok;
    int hundreds = mHundredsLineEdit->text().toInt(&ok);
    if (ok) billsTotal += hundreds * 100;
    int fifties = mFiftiesLineEdit->text().toInt(&ok);
    if (ok) billsTotal += fifties * 50;
    int twenties = mTwentiesLineEdit->text().toInt(&ok);
    if (ok) billsTotal += twenties * 20;
    int tens = mTensLineEdit->text().toInt(&ok);
    if (ok) billsTotal += tens * 10;
    int fives = mFivesLineEdit->text().toInt(&ok);
    if (ok) billsTotal += fives * 5;
    int ones = mOnesLineEdit->text().toInt(&ok);
    if (ok) billsTotal += ones;

    mTotalBillsLabel->setText(QString::number(billsTotal).asprintf("$%.2f", billsTotal));

    double coinsTotal = 0.0;
    int dollars = mDollarsLineEdit->text().toInt(&ok);
    if (ok) coinsTotal += dollars;
    int quarters = mQuartersLineEdit->text().toInt(&ok);
    if (ok) coinsTotal += quarters * 0.25;
    int dimes = mDimesLineEdit->text().toInt(&ok);
    if (ok) coinsTotal += dimes * 0.1;
    int nickels = mNickelsLineEdit->text().toInt(&ok);
    if (ok) coinsTotal += nickels * 0.05;
    int pennies = mPenniesLineEdit->text().toInt(&ok);
    if (ok) coinsTotal += pennies * 0.01;

    mTotalCoinsLabel->setText(QString::number(coinsTotal).asprintf("$%.2f", coinsTotal));

    double checksTotal = 0.0;
    int numChecks = 0;
    QList checkLineEdits = mChecksWidget->findChildren<QLineEdit*>();
    for (QLineEdit* checkLineEdit : checkLineEdits) {
        QString strippedText = checkLineEdit->text().replace("$", "").trimmed();
        double value = strippedText.toDouble(&ok);
        if (ok) {
            checksTotal += value;
            numChecks++;
        }
    }

    mTotalChecksLabel->setText(QString::number(numChecks).asprintf("$%.2f", checksTotal));
    mNumChecksLabel->setText(QString::number(numChecks));

    double specialTotal = 0.0;
    QList specialLineEdits = mSpecialWidget->findChildren<QLineEdit*>();
    for (QLineEdit* lineEdit : specialLineEdits) {
        QString strippedText = lineEdit->text().replace("$", "").trimmed();
        double value = strippedText.toDouble(&ok);
        if (ok) specialTotal += value;
    }

    mTotalSpecialLabel->setText(QString::number(specialTotal).asprintf("$%.2f", specialTotal));

    double total = coinsTotal + billsTotal + checksTotal;
    // Only include special designations in the total deposit if the user desires it
    if (settingsJson.object()["includeSpecial"].toBool()) {
        total += specialTotal;
    }
    mTotalLabel->setText(QString::number(total).asprintf("$%.2f", total));
}

void MainWindow::createReport(QPainter& painter) {
    /**
     * Takes a QPainter and draws a text report onto it for previewing and printing
     * @return void
     */

    // set the painter window and render hints
    painter.setWindow(0, 0, 850, 1100);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    //set the fonts and their font metrics
    auto titleFont = QFont("Helvetica");
    titleFont.setPixelSize(20);
    titleFont.setBold(true);

    auto standardFont = QFont("Book Antiqua");
    standardFont.setPixelSize(16);

    auto boldFont = QFont("Book Antiqua");
    boldFont.setPixelSize(16);
    boldFont.setBold(true);

    auto italicFont = QFont("Book Antiqua");
    italicFont.setPixelSize(16);
    italicFont.setItalic(true);

    auto boldItalicFont = QFont("Book Antiqua");
    boldItalicFont.setPixelSize(18);
    boldItalicFont.setBold(true);
    boldItalicFont.setItalic(true);

    auto titleFontMetrics = QFontMetrics(titleFont);
    auto standardFontMetrics = QFontMetrics(standardFont);
    auto boldFontMetrics = QFontMetrics(boldFont);

    // set the measurements and dimensions used in the layout
    double hMargin = 1;
    double vMargin = 1;
    double columnGap = 0.25;
    int inch = painter.window().width() / 8.5;
    int xpos = inch * hMargin;
    int ypos = inch * vMargin;
    int lineHeightTight = boldFontMetrics.lineSpacing();
    int lineHeightStandard = lineHeightTight * 1.25;
    int lineHeightWide = lineHeightTight * 1.5;
    int titleLineHeight = titleFontMetrics.lineSpacing();
    int spaceWidth = boldFontMetrics.horizontalAdvance(" ");
    int singleColumnWidth = ((8.5 - (hMargin * 2) - (columnGap * 2)) / 4) * inch;
    int columnHeight = inch * ((11 - (vMargin * 2) / 3 * 2));
    QRect leftColumnRect = QRect(
        inch * hMargin,
        inch * vMargin,
        singleColumnWidth,
        columnHeight
    );
    QRect middleColumnRect = QRect(
        leftColumnRect.right() + (inch * columnGap),
        inch * vMargin,
        singleColumnWidth * 2,
        columnHeight
    );
    QRect rightColumnRect = QRect(
        middleColumnRect.right() + (inch * columnGap),
        inch * vMargin,
        singleColumnWidth,
        columnHeight
    );

    painter.setPen(Qt::GlobalColor::black);

    xpos = leftColumnRect.left();
    ypos = leftColumnRect.top();
    painter.setFont(titleFont);
    painter.drawText(xpos, ypos, settingsJson.object()["name"].toString() + " Weekly Giving Report");

    ypos += titleLineHeight;
    painter.setFont(boldFont);
    painter.drawText(xpos, ypos, "Date:");
    xpos += boldFontMetrics.boundingRect("Date:").width() + spaceWidth;

    painter.setFont(standardFont);
    painter.drawText(QPointF(xpos, ypos), mDateWidget->date().toString());

    xpos = middleColumnRect.left();
    painter.setFont(boldFont);
    painter.drawText(xpos, ypos, "Prepared By:");
    xpos += boldFontMetrics.boundingRect("Prepared By:").width() + spaceWidth;

    painter.setFont(standardFont);
    painter.drawText(QPointF(xpos, ypos), mPreparedLineEdit->text());

    xpos = rightColumnRect.left();
    painter.drawText(xpos, ypos, "ID:");
    xpos +=boldFontMetrics.boundingRect("ID:").width() + spaceWidth;

    painter.drawText(QPointF(xpos, ypos), mIdLabel->text());

    xpos = leftColumnRect.left();
    ypos += lineHeightStandard;
    painter.setFont(boldFont);
    painter.drawText(xpos, ypos, "Signature:");

    ypos += lineHeightTight;
    painter.drawRect(xpos, ypos, 300, lineHeightStandard * 2);

    ypos += lineHeightStandard * 4;
    int dataTop = ypos;

    // ########################
    //      LEFT-SIDE DATA
    // ########################

    QVector<QString> labels = {
        "$100 Bills:",
        "$50 Bills:",
        "$20 Bills:",
        "$10 Bills:",
        "$5 Bills:",
        "$1 Bills:"
    };

    QVector<QString> values = {
        mHundredsLineEdit->text(),
        mFiftiesLineEdit->text(),
        mTwentiesLineEdit->text(),
        mTensLineEdit->text(),
        mFivesLineEdit->text(),
        mOnesLineEdit->text(),
    };

    int textWidth;
    for (int i = 0; i < labels.size(); i++) {
        xpos = leftColumnRect.left();
        painter.setFont(boldFont);
        painter.drawText(xpos, ypos, labels.at(i));

        xpos = leftColumnRect.right();
        painter.setFont(standardFont);
        textWidth = standardFontMetrics.boundingRect(values.at(i)).width();
        painter.drawText(xpos - textWidth, ypos, values.at(i));

        ypos += lineHeightStandard;
    }

    ypos += lineHeightStandard;

    labels = {
        "Dollar Coins:",
        "Quarters:",
        "Dimes:",
        "Nickels:",
        "Pennies:"
    };

    values = {
        mDollarsLineEdit->text(),
        mQuartersLineEdit->text(),
        mDimesLineEdit->text(),
        mNickelsLineEdit->text(),
        mPenniesLineEdit->text()
    };

    int highestYpos = 0;

    for (int i = 0; i < labels.size(); i++) {
        xpos = leftColumnRect.left();
        painter.setFont(boldFont);
        painter.drawText(xpos, ypos, labels.at(i));

        xpos = leftColumnRect.right();
        painter.setFont(standardFont);
        int textWidth = painter.fontMetrics().tightBoundingRect(values.at(i)).width();
        painter.drawText(xpos - textWidth, ypos, values.at(i));

        ypos += lineHeightStandard;
        if (ypos > highestYpos) highestYpos = ypos;
    }

    // ########################
    //      MIDDLE DATA
    // ########################

    labels = {
        settingsJson.object()["specialDesignations"].toObject()["spec1"].toString() + ":",
        settingsJson.object()["specialDesignations"].toObject()["spec2"].toString() + ":",
        settingsJson.object()["specialDesignations"].toObject()["spec3"].toString() + ":",
        settingsJson.object()["specialDesignations"].toObject()["spec4"].toString() + ":",
        settingsJson.object()["specialDesignations"].toObject()["spec5"].toString() + ":",
        settingsJson.object()["specialDesignations"].toObject()["spec6"].toString() + ":",
        settingsJson.object()["specialDesignations"].toObject()["spec7"].toString() + ":"
    };

    values = {};
    for (QLineEdit* lineEdit : mSpecialWidget->findChildren<QLineEdit*>()) {
        values.append(lineEdit->text().replace("$", "").trimmed());
    }

    ypos = dataTop;
    for (int i = 0; i < labels.size(); i++) {
        xpos = middleColumnRect.left();
        painter.setFont(boldFont);
        painter.drawText(xpos, ypos, labels.at(i));

        painter.setFont(standardFont);
        bool ok;
        double value = values.at(i).toDouble(&ok);
        if (!ok) {
            value = 0;
        }
        QString valueString = QString::asprintf("$%.2f", value);
        int textWidth = painter.fontMetrics().tightBoundingRect(valueString).width();

        xpos = middleColumnRect.right();
        painter.drawText(xpos - textWidth, ypos, valueString);

        ypos += lineHeightStandard;
        if (ypos > highestYpos) highestYpos = ypos;
    }

    // ########################
    //     RIGHT-SIDE DATA
    // ########################

    values = {};
    for (QLineEdit* lineEdit : mChecksContainer->findChildren<QLineEdit*>()) {
        values.append(lineEdit->text().replace("$", "").trimmed());
    }

    ypos = dataTop;
    for (int i = 0; i < values.size(); i++) {
        xpos = rightColumnRect.left();
        painter.setFont(boldFont);
        painter.drawText(xpos, ypos, "Check " + QString::number(i) + ":");

        painter.setFont(standardFont);
        bool ok;
        double value = values.at(i).toDouble(&ok);
        if (!ok) {
            value = 0;
        }
        QString valueString = QString::asprintf("$%.2f", value);
        int textWidth = painter.fontMetrics().tightBoundingRect(valueString).width();

        xpos = rightColumnRect.right();
        painter.drawText(xpos - textWidth, ypos, valueString);

        ypos += lineHeightStandard;
    }

    // ########################
    //          TOTALS
    // ########################

    ypos = highestYpos + (lineHeightStandard);

    QRect totalsRect = QRect(
        leftColumnRect.left(),
        ypos,
        middleColumnRect.right() - leftColumnRect.left() - (inch * columnGap),
        lineHeightStandard * 8
    );

    QPen pen = QPen(Qt::GlobalColor::black, 3, Qt::SolidLine);
    painter.setPen(pen);
    painter.drawRect(
        totalsRect.left() - 20,
        totalsRect.top() - 30,
        totalsRect.width() + 40,
        totalsRect.height() + 60
    );

    xpos = totalsRect.left();
    ypos = totalsRect.top();
    painter.setFont(boldFont);
    painter.drawText(xpos, ypos, "Number of Checks:");

    painter.setFont(standardFont);
    textWidth = standardFontMetrics.boundingRect(mNumChecksLabel->text()).width();
    xpos = totalsRect.right() - textWidth;
    painter.drawText(xpos, ypos, mNumChecksLabel->text());

    xpos = totalsRect.left();
    ypos += lineHeightStandard * 2;

    labels = {
        "Total Designated Offerings:",
        "",
        "Total Bills:",
        "Total Coins:",
        "Total Checks:",
        "",
        "Total Deposit:"
    };

    values = {
        mTotalSpecialLabel->text(),
        "",
        mTotalBillsLabel->text(),
        mTotalCoinsLabel->text(),
        mTotalChecksLabel->text(),
        "",
        mTotalLabel->text(),
    };

    for (int i = 0; i < labels.size(); i++) {
        xpos = totalsRect.left();
        painter.setFont(boldFont);
        if (i == labels.size() - 1) painter.setFont(boldItalicFont);
        painter.drawText(xpos, ypos, labels.at(i));

        painter.setFont(standardFont);
        if (i == labels.size() - 1) painter.setFont(boldItalicFont);
        textWidth = standardFontMetrics.boundingRect(values.at(i)).width();
        xpos = totalsRect.right() - textWidth;
        painter.drawText(QPointF(xpos, ypos), values.at(i));

        ypos += lineHeightStandard;
    }

    painter.end();
}

bool MainWindow::checkChanges() {
    /**
     * Checks the state of mChanges and creates a dialog if "true" asking if the user would like to save the changes.
     * Returns "true" if there are no changes or the user clicks "yes" or "no". Returns false if the user clicks
     * "cancel".
     * @return the result of the user's input, or "true" if there are no changes
     */
    if (mChanges) {
        int result = QMessageBox::question(
            this,
            "Save Changes",
            "The current record has been changes. Save the changes?",
            QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::Cancel
        );

        if (result == QMessageBox::StandardButton::Cancel) return false;

        // Save the current record on a "Yes" click
        if (result == QMessageBox::StandardButton::Yes) {
            saveRecord();
        }
    }

    return true;
}

void MainWindow::closeEvent(QCloseEvent* event) {
    /**
     * Checks for changes before closing the program
     * @return void
     */
    bool goOn = checkChanges();
    if (!goOn) {
        event->ignore();
        return;
    }

    // First, ensure that the database connection is closed
    bool ok = closeAndRemoveConnection();
    if (!ok) {
        int result = QMessageBox::warning(
            this,
            "Cannot Close Database",
            "The database could not be safely closed. Continuing may result in data loss. Continue?",
            QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::Cancel
        );

        if (result != QMessageBox::StandardButton::Yes) {
            event->ignore();
            return;
        }
    }

    // Create a backup of the database
    QString dbFileLoc = settingsJson.object()["fileLoc"].toString();
    QFileInfo fileInfo(dbFileLoc);
    QString backupFileLoc(
        fileInfo.absolutePath() +
        "/weekly_giving_backup_" +
        QDateTime::currentDateTime().toString("yyyy-MM-dd.hh.mm.ss") + ".db"
    );
    QFile dbFile(dbFileLoc);
    if (!dbFile.copy(backupFileLoc)) {
        std::cerr << "Failed to create a backup of the database!!" << std::endl;
        std::cerr << dbFile.errorString().toStdString() << std::endl;
        QMessageBox::warning(
            this,
            "Unable to Create Backup",
            "Unable to create a backup copy of your database."
        );
    }
    QFile::setPermissions(backupFileLoc, QFileDevice::ReadOwner | QFileDevice::WriteOwner);

    // Count the number of backups and delete the oldest ones if there are more than 10 backup files
    QDir dbDir(fileInfo.absolutePath());
    int numBackups = 0;
    for (QString fileName : dbDir.entryList(QDir::NoFilter, QDir::SortFlag::Time)) {
        if (fileName.contains("backup")) numBackups++;
        if (numBackups > 10) {
            QFile::remove(dbDir.absolutePath() + "/" + fileName);
        }
    }

    // Copy the temporary (working) database file back to the original location, removing the original first
    dbFile.close();
    QFile origDbFile(dbFileLoc);
    if (origDbFile.exists()) {
        if (!origDbFile.remove()) {
            std::cerr << "Failed to delete the old database file!" << std::endl;
            std::cerr << origDbFile.errorString().toStdString() << std::endl;

            // Handle the failure/abort the copy so you don't get an overwrite error
            int result = QMessageBox::warning(
                this,
                "Cannot Remove Old Database",
                "The old database could not be replaced with the new. "
                    "Continuing may result in data loss. Continue?",
                QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::Cancel
            );
            if (result != QMessageBox::StandardButton::Yes) {
                event->ignore();
                return;
            }
        }
    }

    QFile tempDbFile(mTempDbPath);
    if (tempDbFile.copy(dbFileLoc)) {
        QFile::setPermissions(dbFileLoc, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        tempDbFile.close();
    } else {
        std::cerr << "Failed to copy temp file to original database!" << std::endl;
        std::cerr << tempDbFile.errorString().toStdString() << std::endl;

        int result = QMessageBox::warning(
            this,
            "Cannot Copy Database",
            "The temporary database could not be copied. Continuing may result in data loss. Continue?",
            QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::Cancel
        );

        if (result != QMessageBox::StandardButton::Yes) {
            event->ignore();
            return;
        }
    }

    // Complete the closeEvent
    QMainWindow::closeEvent(event);
}