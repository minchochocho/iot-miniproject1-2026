## miniproject01 - CodeShelf(코드 라이브러리 및 큐레이션)
-- DB 생성

CREATE DATABASE CODESHELF;

-- 사용자 생성
CREATE USER shl_user identified BY 'rcg6510@';

-- 권한
GRANT ALL PRIVILEGES ON CODESHELF.* TO 'shl_user'@'%';

-- 권한 적용
flush PRIVILEGES;
