#pragma once

#include <QObject>
#include <QHash>
#include <QDateTime>
#include <QSqlDatabase>
#include <QFileInfo>

class ScanWorker : public QObject
{
    Q_OBJECT

public:
    explicit ScanWorker(QObject* parent = nullptr);

public slots:
    void scan(const QString& rootPath, int rootId, QHash<QString, QDateTime> dbFileMap);

signals:
    void scanProgress(int processedCount);
    void scanFinished(bool success, int changedCount, int skippedCount);
    void scanFailed(const QString& message);

private:
    bool scanDirRecursive(
        const QString& path,
        int rootId,
        const QHash<QString, QDateTime>& dbFileMap,
        QSqlDatabase& db,
        int& changedCount,
        int& skippedCount,
        int& processedCount);

    bool upsertFileMetadata(
        const QFileInfo& info,
        int rootId,
        const QHash<QString, QDateTime>& dbFileMap,
        QSqlDatabase& db,
        int& changedCount,
        int& skippedCount);
};
