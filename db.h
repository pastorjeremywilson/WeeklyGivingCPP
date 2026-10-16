//
// Created by jeremy on 8/30/26.
//

#pragma once
#include <QSqlDatabase>

#include "gui.h"

QPair<bool, QString> createConnection(const QString &dbPath);
QPair<QList<QString>, QList<QString>> getIndices();
QList<QPair<QString, QVariant>> getById(const QString &id);
QList<QPair<QString, QVariant>> getByDate(const QString &date);
QString createNewRecord();
QString deleteRec(QString id);
QString saveRec(QString sql);
int convertDatabase(MainWindow* mainWindow);
bool closeAndRemoveConnection();