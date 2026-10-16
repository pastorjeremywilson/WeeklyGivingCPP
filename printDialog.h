//
// Created by jeremy on 8/18/26.
//

#pragma once

#include "gui.h"
#include "autoSelectWidgets.h"
#include <QDialog>
#include <QLabel>
#include <QHash>
#include <QVariant>
#include <QButtonGroup>

class CustomPrintDialog : public QDialog {
    Q_OBJECT

public:
    explicit CustomPrintDialog(
        QWidget* parent,
        MainWindow* mainWindow,
        bool landscape = false,
        float pageMarginsHorizontal = 0.0f,
        float pageMarginsVertical = 0.0f);
    ~CustomPrintDialog() override = default;

private:
    MainWindow* mMainWindow;
    bool mLandscape;
    float mPageMarginsHorizontal;
    float mPageMarginsVertical;
    int mCurrentPage;
    QList<QImage> mPages = {QImage()};
    QHash<QString, QVariant> mPrinterProperties;
    QLabel* mPreviewLabel = nullptr;
    QLabel* mPageLabel = nullptr;
    QComboBox* mPrinterCombobox = nullptr;
    AutoSelectSpinBox* mCopiesSpinbox = nullptr;
    QButtonGroup* mOrientationButtonGroup = nullptr;
    QComboBox* mDuplexCombobox = nullptr;
    AutoSelectLineEdit* mRangeLineEdit = nullptr;

    void initComponents();
    void setPrinterProperties();
    void drawPreview(QImage image);
    void orientationChanged();
    std::vector<int> parsePageRange(QString pageRange);
    void makePreview();
    void doPrint();
};


