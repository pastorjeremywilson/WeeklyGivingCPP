//
// Created by jeremy on 8/30/26.
//

#include "db.h"
#include "autoSelectWidgets.h"

#include <iostream>
#include <qdatetime.h>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDir>
#include <QJsonObject>
#include <QSqlRecord>
#include <QJsonArray>


QPair<bool, QString> createConnection(const QString &dbPath) {
    // First make sure the SQLite driver is available
    if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
        std::cerr << "QSQLITE driver is NOT available! Available drivers: "
                  << QSqlDatabase::drivers().join(", ").toStdString() << std::endl;
        return { false, "" };
    }

    // Run closeAndRemoveConnection in case a database instance already exists
    closeAndRemoveConnection();

    const QString tempPath(QDir::tempPath());
    const QString tempDbPath(tempPath + "/" + dbPath.split("/").last());

    QFile dbFile(dbPath);
    if (dbFile.exists()) {
        if (QFile::exists(tempDbPath)) {
            QFile::remove(tempDbPath);
        }

        if (dbFile.copy(tempDbPath)) {
            QFile::setPermissions(tempDbPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
            dbFile.close();
        } else {
            std::cerr << "Failed to copy file to temp directory!" << std::endl;
            return { false, "" };
        }
    } else {
        std::cerr << "File not found at " << dbPath.toStdString() << std::endl;
        return { false, "" };
    }

    std::cout << "Adding fresh default connection" << std::endl;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(tempDbPath);

    std::cout << "Opening database" << std::endl;
    if (!db.open()) {
        qCritical() << "Failed to open database: " << db.lastError().text();
        std::cout << "Failed to open database: " << db.lastError().text().toStdString() << std::endl;
        return { false, "" };
    }

    qDebug() << "Database opened successfully!";
    std::cout << "Database opened successfully!" << std::endl;
    return { true, tempDbPath };
}

QPair<QList<QString>, QList<QString>> getIndices() {
    /**
     * Get all of the values of the date and id columns in the database and return them as lists
     * @return a pair of QLists containing the strings for dates and ids
     */
    QList<QString> dates;
    QList<QString> ids;

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery query(db);
    QString sql = "SELECT date, id FROM weekly_giving";
    query.exec(sql);

    while (query.next()) {
        dates.append(query.value(0).toString());
        ids.append(query.value(1).toString());
    }

    return { dates, ids };
}

QList<QPair<QString, QVariant>> getById(const QString &id) {
    /**
     * Get all of the columns for the record matching the given id. Return them as pairs matching the column name
     * to its value
     * @return a QPair containing the column name and its value
     */
    QList<QPair<QString, QVariant>> resultSet;

    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        return { resultSet };
    }

    QString sql = "SELECT * FROM weekly_giving WHERE id=\"" + id + "\" ORDER BY id";
    QSqlQuery query(db);
    query.exec(sql);

    while (query.next()) {
        QSqlRecord record = query.record();
        int columnCount = record.count();

        for (int i = 0; i < columnCount; ++i) {
            resultSet.append(QPair(record.fieldName(i), query.value(i)));
        }
    }

    return resultSet;
}

QList<QPair<QString, QVariant>> getByDate(const QString &date) {
    /**
     * Get all of the columns for the record matching the given date. Return them as pairs matching the column name
     * to its value
     * @return a QPair containing the column name and its value
     */
    QList<QPair<QString, QVariant>> resultSet;
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        return { resultSet };
    }

    QString sql = "SELECT * FROM weekly_giving WHERE date=\"" + date + "\" ORDER BY date";
    QSqlQuery query(db);
    query.exec(sql);

    while (query.next()) {
        QSqlRecord record = query.record();
        int columnCount = record.count();

        for (int i = 0; i < columnCount; ++i) {
            resultSet.append(QPair(record.fieldName(i), query.value(i)));
        }
    }

    return resultSet;
}

QString createNewRecord() {
    /**
     * Creates a new database entry with a new id and today's date.
     * @return a QString of the new ID number
     */
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        return { "Unable to open database" };
    }

    QString sql = "SELECT id FROM weekly_giving";
    QSqlQuery query(db);
    query.exec(sql);

    // Iterate through all values in the id column to find the highest one
    int highestId = 0;
    while (query.next()) {
        bool ok;
        int thisId = query.value(0).toInt(&ok);
        if (ok) {
            if (thisId > highestId) highestId = thisId;
        }
    }

    // Create a new ID that is one larger than the largest existent ID
    int nextId = highestId + 1;
    query.clear();
    sql =
        "INSERT INTO weekly_giving (id, date) VALUES (\"" +
        QString::number(nextId) + "\", \"" +
        QDate::currentDate().toString("yyyy-MM-dd") +"\");";
    bool ok = query.exec(sql);
    if (!ok) {
        qDebug() << query.lastError().text();
    }

    if (ok) {
        return QString::number(nextId);
    }
    return {""};
}

QString deleteRec(QString id) {
    /**
     * Deletes from the database a record with an id that matches the given id
     * @return an empty QString if successful or the query's last error if not
     */
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        return { "Unable to open database" };
    }
    QString sql = "DELETE FROM weekly_giving WHERE id=\"" + id + "\"";
    QSqlQuery query(db);
    bool ok = query.exec(sql);
    if (ok) {
        return { "" };
    }

    return query.lastError().text();
}

QString saveRec(QString sql) {
    /**
     * Takes an sql UPDATE command an runs it on the database
     * @return an empty QString if successful or the query's last error if not
     */
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        return { "" };
    }

    QSqlQuery query(db);
    bool ok = query.exec(sql);
    if (ok) {
        return { "" };
    }

    return query.lastError().text();
}

int convertDatabase(MainWindow* mainWindow) {
    /**
     * Creates a new table in the database containing only an id and data column. Walks through the current database
     * converting each row's data into a json object that will become the entry of the data column.
     * @param mainWindow The current instance of MainWindow
     * @return 0 if successful
     */
    QSqlDatabase db = QSqlDatabase::database();
    QSqlRecord record = db.record("weekly_giving");
    QStringList columnNames;
    for (int i = 0; i < record.count(); ++i) {
        columnNames.append(record.fieldName(i));
    }

    QSqlQuery query(db);
    QString sql("SELECT * FROM weekly_giving;");
    bool ok = query.exec(sql);
    if (ok) {
        QList<QPair<QString, QJsonObject>> entries;
        while (query.next()) {
            QString thisId = query.value(1).toString();
            QJsonObject recordJson;
            recordJson["bills"] = QJsonObject();
            recordJson["coins"] = QJsonObject();
            recordJson["checks"] = QJsonArray();
            recordJson["special"] = QJsonObject();
            for (int i = 0; i < query.record().count(); i++) {
                QString fieldName = query.record().fieldName(i);
                if (fieldName.contains("total") || fieldName.contains("quantity")) {
                    recordJson[fieldName] = query.value(i).toString();
                } else if (fieldName.contains("bills")) {
                    QJsonObject billsObject = recordJson["bills"].toObject();
                    billsObject[columnNames.at(i)] = query.value(i).toString();
                    recordJson["bills"] = billsObject;
                } else if (fieldName.contains("coins")) {
                    QJsonObject coinsObject = recordJson["coins"].toObject();
                    coinsObject[columnNames.at(i)] = query.value(i).toString();
                    recordJson["coins"] = coinsObject;
                } else if (fieldName.contains("checks") ) {
                    QString checkString = query.value(i).toString();
                    checkString.replace("$", "");
                    checkString.replace(",", "");
                    checkString = checkString.trimmed();
                    bool ok;
                    double value = checkString.toDouble(&ok);
                    if (ok && value > 0) {
                        QJsonArray checksArray = recordJson["checks"].toArray();
                        checksArray.append(checkString);
                        recordJson["checks"] = checksArray;
                    }
                } else if (fieldName.contains("spec")) {
                    QJsonObject specObject = recordJson["special"].toObject();
                    QString designationString =
                        mainWindow->settingsJson.object()["specialDesignations"].toObject()[fieldName].toString();
                    specObject[designationString] = query.value(i).toString();
                    recordJson["special"] = specObject;
                } else {
                    recordJson[fieldName] = query.value(i).toString();
                }
            }
            QPair thisEntry = {thisId, recordJson };
            entries.append(thisEntry);
        }

        sql = "CREATE TABLE new_table (id TEXT, data TEXT);";
        query.exec(sql);

        db.transaction(); // Begin transaction

        QSqlQuery insertQuery;
        insertQuery.prepare("INSERT INTO new_table (id, data) VALUES (:id, :data)");

        for (const auto& entry : entries) {
            insertQuery.bindValue(":id", entry.first);
            insertQuery.bindValue(":data", QString::fromUtf8(QJsonDocument(entry.second).toJson(QJsonDocument::Compact)));

            if (!insertQuery.exec()) {
                qDebug() << "Insert error on ID" << entry.first << ":" << insertQuery.lastError().text();
            }
        }

        db.commit();
    }
    query.finish();
    return 0;
}

bool closeAndRemoveConnection() {
    // --- CLEAN UP EXISTING DEFAULT CONNECTION ---
    // If a default connection exists, remove it completely
    if (QSqlDatabase::contains(QSqlDatabase::defaultConnection)) {
        {
            // Enclose in scope so 'oldDb' is destroyed before calling removeDatabase
            QSqlDatabase oldDb = QSqlDatabase::database(QSqlDatabase::defaultConnection, false);
            if (oldDb.isOpen()) {
                oldDb.close();
            }
        }
        QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
    }

    return true;
}