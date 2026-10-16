//
// Created by jeremy on 8/19/26.
//

#include "autoSelectWidgets.h"
#include "gui.h"

#include <iostream>
#include <qcborvalue.h>
#include <qevent.h>
#include <QWidget>
#include <QSpinBox>
#include <QLineEdit>
#include <QApplication>

AutoSelectSpinBox::AutoSelectSpinBox(QWidget *parent) : QSpinBox(parent) {
    // Use the below AutoSelectLineEdit as the line edit of the spinbox
    setLineEdit(new AutoSelectLineEdit(this));
}


AutoSelectLineEdit::AutoSelectLineEdit(QWidget *parent, MainWindow *mainWindow) : QLineEdit(parent) {
    mMainWindow = mainWindow;
    mAllSelected = false;
    isCurrency = false;
}

void AutoSelectLineEdit::mouseReleaseEvent(QMouseEvent *event) {
    /**
     * On mouse release, if the text is already selected, set the cursor position to the end of the text, otherwise
     * select all of the text
     * @param event the QMouseEvent
     * @return void
     */
    if (mAllSelected) {
        setCursorPosition(text().length());
        mAllSelected = false;
    } else {
        selectAll();
        mAllSelected = true;
    }
    // allow mouseReleaseEvent to continue
    QLineEdit::mouseReleaseEvent(event);
}

void AutoSelectLineEdit::focusOutEvent(QFocusEvent *event) {
    /**
     * When the line edit loses focus, make sure it is no longer showing as selected, then format the value for currency
     * if isCurrency is true
     * @param event the QFocusEvent
     * @return void
     */
    mAllSelected = false;
    if (isCurrency) {
        QString strippedText = this->text().replace("$", "").trimmed();
        if (!strippedText.isEmpty()) {
            bool ok;
            double value = strippedText.toDouble(&ok);
            if (ok) {
                setText(QString::number(value).asprintf("$%.2f", value));
                setStyleSheet("color: black;");
            } else {
                setStyleSheet("color: red;");
            }
        }
    QLineEdit::focusOutEvent(event);
    }
}

bool AutoSelectLineEdit::event(QEvent *event) {
    /**
     * Check the events for a keyPressEvent involving Return, Enter, or Tab. If it's one of these and this happens to be
     * the last checkLineEdit in the checks widget, run MainWindow's addCheck function and call focusNextChild to
     * advance the focus to the newly-created check widget.
     * @event the QEvent
     * @return the boolean result of calling QLineEvent::event(event)
     */
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (objectName() == "checkLineEditLast" && mMainWindow != nullptr) {
            if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Tab) {
                mMainWindow->addCheck(0.0);
                QApplication::processEvents();
                if (keyEvent->key() != Qt::Key_Tab) {
                    focusNextChild();
                }
            }
        }
    }
    return QLineEdit::event(event); // Let QLineEdit handle other events
}