#include"DatabaseManager.h"
#include<QDebug>

// [1] DB 연결(프로그램 시작 시 한번 호출)
bool DatabaseManager::connectDB(const QString& host, const QString& user, const QString& pass, const QString& dbName) {
	// 드라이버 로드
	db = QSqlDatabase::addDatabase("QMYSQL");
	
	// 접속 정보
	db.setHostName(host);
	db.setUserName(user);
	db.setPassword(pass);
	db.setDatabaseName(dbName);

	// 연결 확인
	if (!db.open()) {
		qDebug() << "DB 연결 실패 사유: " << db.lastError().text();
		return false;
	}
	else {
		qDebug() << "DB 연결 성공!";
		return true;
	}
}

// [2] 초기 연결
QList<FileItem> DatabaseManager::fetchFiles(const SearchOptions& opt) {
	QList<FileItem> results;
	QSqlQuery q;

	// (1) 공통 쿼리
	QString whereClause = " WHERE root_id = :rid";
	
	// (2) 조건에 따른 SQL 조립
	if (!opt.extension.isEmpty()) {
		whereClause += " AND UPPER(extension) = :ext ";
	}

	if (!opt.keyword.trimmed().isEmpty() ) {
		if (opt.searchMode == "all") {
			// 제목 + 내용 검색
			whereClause += " AND MATCH(file_name, content) AGAINST(:keyword IN NATURAL LANGUAGE MODE)";
		}
		else {
			// 제목 검색
			whereClause += " AND file_name LIKE :keyword ";
		}
	}

	QString sql = "SELECT id, file_name, extension, filepath, last_modified, tags "
				  " FROM codes " + whereClause 
				+ " ORDER BY last_modified DESC LIMIT :limit OFFSET :offset";

	q.prepare(sql);

	// (3) 바인딩
	q.bindValue(":rid", opt.rootId);
	if (!opt.extension.isEmpty()) q.bindValue(":ext", opt.extension.toUpper());

	if (!opt.keyword.trimmed().isEmpty()) { 
		if( opt.searchMode == "all") {
		q.bindValue(":keyword", opt.keyword.trimmed());
		}
		else {
			q.bindValue(":keyword", "%" + opt.keyword.trimmed() + "%");
		}
	}

	q.bindValue(":limit", opt.limit);
	q.bindValue(":offset", opt.offset);

	// (4) 데이터 담기
	if (q.exec()) {
		while (q.next()) {
			FileItem item;
			item.id = q.value("id").toInt();
			item.name = q.value("file_name").toString();
			item.extension = q.value("extension").toString();
			item.path = q.value("filepath").toString();
			item.lastModified = q.value("last_modified").toDateTime();
			item.tags = q.value("tags").toString();
			results.append(item);
			qDebug() << item.name;
		}
	}
	else {
		qDebug() << "Fetch 에러: " << q.lastError().text();
	}
	qDebug() << sql;

	return results;
}

// [3] 파일 레코드 삽입 및 업데이트(키워드 분류)
bool DatabaseManager::insertFileRecord(const FileRecord& record) {
	if (!db.isOpen()) return false;
	
	// 확장자별 자동 태그 생성 로직
	QString autoTag = "";
	QString ext = record.extension;
	if (ext == "c" || ext == "cpp" || ext == "h") autoTag = "#C #C++ #Source";
	else if (ext == "py") autoTag = "#Python # Script";
	else if (ext == "sql") autoTag = "#Database #SQL";

	QSqlQuery q;
	q.prepare(
		"INSERT INTO codes(root_id, filepath, file_name, extension, file_size, content, last_modified, tags) "
		"VALUES (:rid, :file_path, :name, :ext, :size, :content, :modified, :tags) "
		"ON DUPLICATE KEY UPDATE "
		"file_size = :size, content = :content, last_modified = :modified, tags = :tags"
	);

	q.bindValue(":rid", record.rootId);
	q.bindValue(":file_path", record.filePath);
	q.bindValue(":name", record.fileName);
	q.bindValue(":ext", record.extension);
	q.bindValue(":size", record.fileSize);
	q.bindValue(":content", record.content);
	// MYSQL의 DATETIME 형식에 맞게 변환
	q.bindValue(":modified", record.lastModified.toString("yyyy-MM-dd HH:mm:ss"));
	q.bindValue(":tags", autoTag);
	if (!q.exec()) {
		qDebug() << "DB 저장 에러 (" << record.fileName << "): " << q.lastError().text();
		return false;
	}
	return true;
}

// [4]
int DatabaseManager::getOrCreateRootID(const QString& rootPath) {
	if (!db.isOpen()) return -1;
	QSqlQuery query;
	
	// (1) 기존 경로 확인
	query.prepare("SELECT id FROM storage_roots WHERE root_path = :path");
	query.bindValue(":path", rootPath);

	if (query.exec() && query.next()) {
		int id = query.value(0).toInt();
		// 마지막 스캔 시간 업데이트
		QSqlQuery up;
		up.prepare("UPDATE storage_roots SET past_scanned = NOW() WHERE id=:id");
		up.bindValue(":id", id);
		up.exec();
		return id;
	}
	else {
		// (2) 신규 등록
		query.prepare("INSERT INTO storage_roots (root_path, past_scanned) VALUES (:path, NOW()) ");
		query.bindValue(":path", rootPath);
		if (query.exec()) {
			return query.lastInsertId().toInt();
		}
	}
	return -1;
}

// [4_1]
QHash<QString, QDateTime> DatabaseManager::getFileModificationMap(int rootId) {
	QHash<QString, QDateTime> fileMap;

	QSqlQuery q;
	q.prepare("SELECT filepath, last_modified FROM codes WHERE root_id = :rid");
	q.bindValue(":rid", rootId);

	if (q.exec()) {
		while(q.next()){
			fileMap.insert(q.value(0).toString(), q.value(1).toDateTime());		
		}
	}
	return fileMap;
}

// [5]
int DatabaseManager::getFileCount(const SearchOptions& opt) {
	QSqlQuery q;
	QString sql = "SELECT COUNT(*) FROM codes WHERE root_id = :rid ";

	if (!opt.extension.isEmpty()) {
		sql += " AND UPPER(extension) = :ext ";
	}

	if (!opt.keyword.trimmed().isEmpty()) {
		if (opt.searchMode == "all") {
			sql += " AND MATCH(file_name, content) AGAINST(:keyword IN NATURAL LANGUAGE MODE)";
		}
		else {
			sql += " AND UPPER(file_name) LIKE UPPER(:keyword)";
		}
	}

	q.prepare(sql);
	q.bindValue(":rid", opt.rootId);


	if (!opt.extension.isEmpty()) q.bindValue(":ext", opt.extension.toUpper());

	if (!opt.keyword.trimmed().isEmpty()) {
		if (opt.searchMode == "all") {
			q.bindValue(":keyword", opt.keyword.trimmed());
		}
		else {
			q.bindValue(":keyword", "%" + opt.keyword.trimmed() + "%");
		}
	}

	if (q.exec() && q.next()) {
		return q.value(0).toInt();
	}
	return 0;
}

//[6]
QStringList DatabaseManager::getExtensionByRootId(int rootId) {
	QStringList extensions;
	if (!db.isOpen()) return extensions;

	QSqlQuery q;
	// root_id를 직접 사용하여 중복 없는 확장자 목록 추출
	q.prepare("SELECT DISTINCT UPPER(extension) FROM codes WHERE root_id = :rid AND extension != ''");
	q.bindValue(":rid", rootId);

	if (q.exec()) {
		while (q.next()) {
			QString ext = q.value(0).toString();
			if (!ext.isEmpty()) {
				extensions << ext;
			}
		}
		qDebug() << "RootID:" << rootId << "에서 추출된 확장자 목록:" << extensions;
	}
	else {
		qDebug() << "확장자 조회 실패:" << q.lastError().text();
	}

	return extensions; // 반드시 결과 리스트를 반환해야 함
}