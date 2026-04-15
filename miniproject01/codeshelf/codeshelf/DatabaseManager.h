#ifndef _DATABASEMANAGER_H_
#define _DATABASEMANAGER_H_

#include <QString>
#include <QDateTime>
#include <QList>
#include <QStringList>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QtGlobal>

// 파일 리스트 출력용
struct FileItem {
	int id;
	QString name;
	QString extension;
	QString path;
	QDateTime lastModified;
	QString tags;
};

struct SearchOptions {
	int rootId;
	QString extension = "";
	QString keyword = "";
	int limit = 20;
	int offset = 0;
	QString searchMode;
};

// 파일 DB 삽입/업데이트용
struct FileRecord {
	int rootId;
	QString filePath;
	QString fileName;
	QString extension;
	qint64 fileSize;
	QString content;
	QDateTime lastModified;
};

class DatabaseManager {
public:
		static DatabaseManager& instance() {// 싱글톤 패턴
		static DatabaseManager inst;
		return inst;
	}
	
	/*** [1] DB 연결 설정 ***/
	bool connectDB(const QString& host = "127.0.0.1",
		const QString& user = "shl_user",
		const QString& pass = "rcg6510@",
		const QString& dbName = "CODESHELF");

	/*** [2] 초기 데이터 로딩 ***/
	QList<FileItem> fetchFiles(const SearchOptions& opt);

	/*** [3] 파일 레코드 삽입 및 업데이트 ***/
	bool insertFileRecord(const FileRecord& record);
	
	/*** [4] Root ID 조회 및 생성 + UPDATE ***/
	int getOrCreateRootID(const QString& rootPath);

	// [4_1] 특정 루트의 파일경로와 수정시간 맵을 가져오기
	QHash<QString, QDateTime> getFileModificationMap(int rootId);

	// 트랜잭션 시작/종료
	bool beginTransaction() { return db.transaction(); }
	bool commitTransaction() { return db.commit(); }
	void rollbackTransaction() { db.rollback(); }

	// [5] Updatepagination을 위한 개수구하기
	int getFileCount(const SearchOptions& opt);

	// [6] loadTagsFromDb
	QStringList getExtensionByRootId(int rootId);

private:
	// 싱글톤을 위한 생성자 private에 두기
	DatabaseManager() = default;
	~DatabaseManager() = default;

	// 복사 방지(C+11이상만 가능)
	DatabaseManager(const DatabaseManager&) = delete;
	DatabaseManager& operator = (const DatabaseManager&) = delete;

	QSqlDatabase db;
};


#endif // !_DATABASEMANAGER_H_
