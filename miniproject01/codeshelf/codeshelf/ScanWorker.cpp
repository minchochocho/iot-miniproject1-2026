#include "ScanWorker.h"

#include "DatabaseManager.h"

#include <QDir>
#include <QThread>
#include <QDebug>

ScanWorker::ScanWorker(QObject* parent)
    : QObject(parent)
{
}

void ScanWorker::scan(const QString& rootPath, int rootId, QHash<QString, QDateTime> dbFileMap)
{
    const QString connectionName =
        QStringLiteral("codeshelf_scan_%1").arg(quintptr(QThread::currentThreadId()));

    QSqlDatabase db = DatabaseManager::createThreadConnection(connectionName);
    if (!db.isOpen()) {
        emit scanFailed(QStringLiteral("스캔용 DB 연결에 실패했습니다."));
        emit scanFinished(false, 0, 0);
        return;
    }

    int changedCount = 0;
    int skippedCount = 0;
    int processedCount = 0;

    if (!db.transaction()) {
        db.close();
        QSqlDatabase::removeDatabase(connectionName);
        emit scanFailed(QStringLiteral("DB 트랜잭션을 시작할 수 없습니다."));
        emit scanFinished(false, 0, 0);
        return;
    }

    const bool ok = scanDirRecursive(
        DatabaseManager::normalizePath(rootPath),
        rootId,
        dbFileMap,
        db,
        changedCount,
        skippedCount,
        processedCount);

    if (ok) {
        db.commit();
    }
    else {
        db.rollback();
    }

    db.close();
    QSqlDatabase::removeDatabase(connectionName);

    emit scanFinished(ok, changedCount, skippedCount);
}

bool ScanWorker::scanDirRecursive(
    const QString& path,
    int rootId,
    const QHash<QString, QDateTime>& dbFileMap,
    QSqlDatabase& db,
    int& changedCount,
    int& skippedCount,
    int& processedCount)
{
    QDir directory(path);
    const QFileInfoList entries =
        directory.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::Name);

    for (const QFileInfo& info : entries) {
        if (info.isDir()) {
            if (!scanDirRecursive(
                    info.absoluteFilePath(),
                    rootId,
                    dbFileMap,
                    db,
                    changedCount,
                    skippedCount,
                    processedCount)) {
                return false;
            }
            continue;
        }

        const QString suffix = info.suffix().toLower();
        if (suffix != QLatin1String("c")
            && suffix != QLatin1String("cpp")
            && suffix != QLatin1String("h")
            && suffix != QLatin1String("sql")
            && suffix != QLatin1String("py")) {
            continue;
        }

        ++processedCount;
        if (processedCount % 25 == 0) {
            emit scanProgress(processedCount);
        }

        if (!upsertFileMetadata(info, rootId, dbFileMap, db, changedCount, skippedCount)) {
            return false;
        }
    }

    return true;
}

bool ScanWorker::upsertFileMetadata(
    const QFileInfo& info,
    int rootId,
    const QHash<QString, QDateTime>& dbFileMap,
    QSqlDatabase& db,
    int& changedCount,
    int& skippedCount)
{
    const QString absolutePath = DatabaseManager::normalizePath(info.absoluteFilePath());
    const QDateTime fileTime = info.lastModified();

    if (dbFileMap.contains(absolutePath)
        && DatabaseManager::isSameFileTime(dbFileMap.value(absolutePath), fileTime)) {
        ++skippedCount;
        return true;
    }

    FileRecord record;
    record.rootId = rootId;
    record.filePath = absolutePath;
    record.fileName = info.fileName();
    record.extension = info.suffix().toLower();
    record.fileSize = info.size();
    record.content = QString();
    record.lastModified = fileTime;

    if (!DatabaseManager::insertFileMetadata(record, db)) {
        return false;
    }

    ++changedCount;
    return true;
}
