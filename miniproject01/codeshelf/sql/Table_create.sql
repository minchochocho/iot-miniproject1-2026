-- miniproject01 - CodeShelf(코드 라이브러리 및 큐레이션)
USE CODESHELF;

CREATE TABLE storage_roots(
	id int PRIMARY KEY AUTO_INCREMENT,
	root_path varchar(512) NOT NULL,
	alias varchar(100),
	past_scanned datetime
);

-- 유니크 제약 조건
ALTER TABLE storage_roots ADD UNIQUE (root_path);

CREATE TABLE codes(
	id int PRIMARY KEY AUTO_INCREMENT,
	root_id int,
	rel_path varchar(512) NOT NULL,
	file_name varchar(255) NOT NULL,
	extension varchar(10),
	file_size BIGINT,
	last_modified datetime,	
	FOREIGN KEY (root_id) REFERENCES storage_roots(id) ON DELETE CASCADE
);

CREATE TABLE tags(
	id int PRIMARY KEY AUTO_INCREMENT,
	tag_name varchar(32)
);

CREATE TABLE code_tags(
	code_id int,
	tag_id int,
	FOREIGN KEY (code_id) REFERENCES codes(id) ON DELETE CASCADE,
	FOREIGN KEY (tag_id) REFERENCES tags(id) ON DELETE CASCADE
);

-- 경로 검색 속도 향상 (중복 체크 시 유용)
CREATE INDEX idx_rel_path ON codes(rel_path);

-- 확장자별 필터링 속도 향상
CREATE INDEX idx_extension ON codes(extension);

-- codes 테이블에 내용 저장 컬럼 추가
ALTER TABLE codes ADD COLUMN content LONGTEXT;

-- 같은 루트 안에서 상대경로가 중복되는 것을 방지
ALTER TABLE codes ADD UNIQUE idx_root_rel (root_id, rel_path);

SELECT * FROM codes;

UPDATE storage_roots (root_path, past_scanned) VALUES (:PATH, now())

ALTER TABLE codes ADD COLUMN tags VARCHAR(255) DEFAULT '';

-- 외래키 제약 조건 때문에 삭제가 안 될 경우를 대비해 체크 해제 후 실행
SET FOREIGN_KEY_CHECKS = 0;

TRUNCATE TABLE codes;
TRUNCATE TABLE storage_roots;

SET FOREIGN_KEY_CHECKS = 1;

-- 서버 설정 최적화
-- 현재 설정 확인
SHOW VARIABLES LIKE 'innodb_flush_log_at_trx_commit';

-- 임시 변경 (서버 재시작 전까지 유지)
SET GLOBAL innodb_flush_log_at_trx_commit = 2;

-- 대용량 패킷 허용
SET GLOBAL max_allowed_packet = 67108864; -- 64MB

SELECT DISTINCT upper(extension) FROM codes;

SELECT file_name, last_modified, tags FROM codes WHERE upper(extention) = :ext