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

ALTER TABLE codes ADD COLUMN content LONGTEXT;

SELECT * FROM codes;

