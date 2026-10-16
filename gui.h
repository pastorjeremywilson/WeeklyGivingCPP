//
// Created by jeremy on 8/29/26.
//

#pragma once

#include <qboxlayout.h>
#include <QMainWindow>
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QJsonDocument>
#include <QLabel>
#include <QTextEdit>
#include <QDateEdit>
#include <qprinter.h>


class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

    QString version;
    QJsonDocument settingsJson;
    QFont mStandardFont;
    QFont mBoldFont;
    QFont mHeaderFont;
    QFont mTitleFont;
    QPrinter* printer;
    QVBoxLayout* checksContainerLayout = nullptr;
    void createReport(QPainter& painter);
    void addCheck(double value);

private:
    bool mChanges;
    QString mTempDbPath;
    QList<QString> mDates;
    QList<QString> mIds;

    QLabel* mIdLabel = nullptr;
    QDateEdit* mDateWidget = nullptr;
    QLineEdit* mPreparedLineEdit = nullptr;

    QLineEdit* mHundredsLineEdit = nullptr;
    QLineEdit* mFiftiesLineEdit = nullptr;
    QLineEdit* mTwentiesLineEdit = nullptr;
    QLineEdit* mTensLineEdit = nullptr;
    QLineEdit* mFivesLineEdit = nullptr;
    QLineEdit* mOnesLineEdit = nullptr;

    QLineEdit* mDollarsLineEdit = nullptr;
    QLineEdit* mQuartersLineEdit = nullptr;
    QLineEdit* mDimesLineEdit = nullptr;
    QLineEdit* mNickelsLineEdit = nullptr;
    QLineEdit* mPenniesLineEdit = nullptr;

    QWidget* mChecksWidget = nullptr;
    QWidget* mChecksContainer = nullptr;

    QWidget* mSpecialWidget = nullptr;

    QTextEdit* mNotesTextEdit = nullptr;

    QLabel* mTotalCoinsLabel = nullptr;
    QLabel* mTotalBillsLabel = nullptr;
    QLabel* mTotalChecksLabel = nullptr;
    QLabel* mTotalSpecialLabel = nullptr;
    QLabel* mTotalLabel = nullptr;
    QLabel* mNumChecksLabel = nullptr;

    QLabel* mStatusLabel = nullptr;

    QWidget* makeSplashWidget();
    void loadSettings();
    void saveSettings();
    void editSettings();
    void initComponents();
    QWidget* makeToolbarWidget();
    QWidget* makeInfoWidget();
    QWidget* makeCashWidget();
    QWidget* makeChecksWidget();
    QWidget* makeSpecialWidget();
    QWidget* makeNotesWidget();
    void populateData(QList<QPair<QString, QVariant>> resultSet);
    void populateComboBoxes();
    void printRecord();
    void makeGraph();
    void saveRecord();
    void addListeners();
    void calculateTotals();
    bool checkChanges();

protected:
    void closeEvent(QCloseEvent* event) override;
};
