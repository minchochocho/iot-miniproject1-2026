-- 1. 기존 테이블 삭제 (외래키 제약 때문에 역순으로 삭제하거나 제약 해제)
SET FOREIGN_KEY_CHECKS = 0;
DROP TABLE IF EXISTS code_tags;
DROP TABLE IF EXISTS tags;
DROP TABLE IF EXISTS codes;
DROP TABLE IF EXISTS storage_roots;
SET FOREIGN_KEY_CHECKS = 1;

-- 2. 루트 경로 저장 테이블
CREATE TABLE storage_roots (
    id INT PRIMARY KEY AUTO_INCREMENT,
    root_path VARCHAR(512) NOT NULL UNIQUE, -- 절대 경로 중복 방지
    alias VARCHAR(100),
    past_scanned DATETIME
);

-- 3. 코드 파일 저장 테이블
CREATE TABLE codes (
    id INT PRIMARY KEY AUTO_INCREMENT,
    root_id INT,
    filepath VARCHAR(512) NOT NULL UNIQUE, -- rel_path 대신 filepath로 명칭 변경!
    file_name VARCHAR(255) NOT NULL,
    extension VARCHAR(10),
    file_size BIGINT,
    content LONGTEXT,
    tags VARCHAR(255) DEFAULT '',
    last_modified DATETIME,
    FOREIGN KEY (root_id) REFERENCES storage_roots(id) ON DELETE CASCADE
);

-- 4. 개별 태그 관리 (중복 없는 태그 목록)
CREATE TABLE tags (
    id INT PRIMARY KEY AUTO_INCREMENT,
    tag_name VARCHAR(32) NOT NULL UNIQUE
);

-- 5. 코드와 태그의 연결 (다대다 관계 해소)
CREATE TABLE code_tags (
    code_id INT,
    tag_id INT,
    PRIMARY KEY (code_id, tag_id), -- 똑같은 태그가 한 파일에 중복 걸리는 것 방지
    FOREIGN KEY (code_id) REFERENCES codes(id) ON DELETE CASCADE,
    FOREIGN KEY (tag_id) REFERENCES tags(id) ON DELETE CASCADE
);

-- 6. 성능 향상을 위한 인덱스
CREATE INDEX idx_extension ON codes(extension);
CREATE INDEX idx_file_name ON codes(file_name);