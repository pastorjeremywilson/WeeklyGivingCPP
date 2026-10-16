//
// Created by jeremy on 8/19/26.
//

#pragma once
#include "gui.h"
#include <QSpinBox>
#include <QLineEdit>


class AutoSelectSpinBox : public QSpinBox {
    Q_OBJECT

public:
    explicit AutoSelectSpinBox(QWidget *parent = nullptr);
};


class AutoSelectLineEdit : public QLineEdit {
    Q_OBJECT

public:
    explicit AutoSelectLineEdit(QWidget *parent = nullptr, MainWindow *mainWindow = nullptr);
    bool isCurrency;
    bool event(QEvent *event) override;

private:
    bool mAllSelected;
    MainWindow* mMainWindow;

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
};
