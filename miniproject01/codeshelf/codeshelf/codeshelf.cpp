#include "codeshelf.h"

CodeShelf::CodeShelf(QWidget *parent)
    : QMainWindow(parent)
{
    /* 데이터베이스 접속*/
    initDatabase();
    /* [1] 전체 베이스 초기설정 */
    centerWidget = new QWidget(this);
    mainLayout = new QVBoxLayout(centerWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0); // 상하좌우 마진제거
    mainLayout->setSpacing(0);  // 위젯 간의 간격을 0으로
    setCentralWidget(centerWidget);
    this->installEventFilter(this);

    /* [2] 상단바 영역 */
    setupTopBar();

    connect(btnSelectRoot, &QPushButton::clicked, this, &CodeShelf::onSelectRootFolder);

    connect(categoryTree, &QTreeWidget::itemClicked, this, &CodeShelf::onTreeItemClicked);

    /* [3] 메인 콘텐츠 영역(Stacked Widget )*/
    mainStackedWidget = new QStackedWidget();
    mainLayout->addWidget(mainStackedWidget);

    // [page 0] 홈 대시보드(임시로)~~
    setupDashboard();

    // [page 1]3분할 설정
    setupManagementPage();

    // [4] 스플리터 초기 비율 설정 (2:3:5)
    mainSplitter->setStretchFactor(0, 2);
    mainSplitter->setStretchFactor(1, 4);
    mainSplitter->setStretchFactor(2, 4);

    mainStackedWidget->addWidget(mainSplitter);

    // [5] 시그널/슬롯 연결
    connect(btnHome, &QPushButton::clicked, this, &CodeShelf::showHome);
    connect(btnSearchToggle, &QPushButton::clicked, this, &CodeShelf::toggleSearchMode);

    resize(1200, 800);

    QTimer::singleShot(100, this, &CodeShelf::initApp);
}

/* 상단바 영역 */
void CodeShelf::setupTopBar() {
    topBar = new QWidget();

    topBar->setFixedHeight(60); // 최대높이 60고정
    topBar->setStyleSheet("background-color: #333; color:white;");  // 배경색 지정
    QHBoxLayout* topLayout = new QHBoxLayout(topBar);

    QLabel* logo = new QLabel("CodeShelf");

    btnHome = new QPushButton("홈");
    btnSearchToggle = new QPushButton("검색/관리");

    logo->setStyleSheet("font-weight: bold; font-size: 18px; margin-left:10px;");
    btnSelectRoot = new QPushButton("폴더 선택");
    btnSelectRoot->setStyleSheet("background-color:#555; padding:5px;");

    QLabel* userInfo = new QLabel("user 1234 👤");

    topLayout->addWidget(logo);
    topLayout->addStretch();    // 중간 여백
    topLayout->addWidget(btnHome);
    topLayout->addWidget(btnSearchToggle);
    topLayout->addStretch();    // 우측 여백
    topLayout->addWidget(btnSelectRoot);
    topLayout->addStretch();    // 우측 여백
    topLayout->addWidget(userInfo);

    mainLayout->addWidget(topBar);
}

// 폴더 선택 시 저장(쓰기)
void CodeShelf::onDirSelected(const QString& path) {
    qDebug() << "저장했어요" << path;
    // 회사이름, 앱이름 형식
    QSettings settings("MinSoft", "CodeShelf");

    // lastFolderPath라는 키값으로 경로 저장
    settings.setValue("lastFolderPath", path);
}

// 시작시 읽기
void CodeShelf::initApp() {
    QSettings settings("MinSoft", "CodeShelf");
    QString lastPath = settings.value("lastFolderPath", "C:/SourceBank").toString();

    lastPath = QDir::fromNativeSeparators(lastPath);    // 슬래시로 정규화

    if (!lastPath.isEmpty() && QDir(lastPath).exists()) {
            qDebug() << "경로 복원 시도" << lastPath;

            this->currentRootPath = lastPath;

            mainStackedWidget->setCurrentWidget(mainSplitter);

            // 스캔 함수 호출
            scanDirectory(lastPath);

            statusBar()->showMessage("이전 작업 폴더를 불러왔습니다: " + lastPath, 3000);
    }
    else {
        qDebug() << "못 불러왔어요" << lastPath;

    }

}

/* [1] 폴더 선택 */
void CodeShelf::onSelectRootFolder() {
    QString dirPath = QFileDialog::getExistingDirectory(this, "루트 폴더 선택", QDir::homePath());
    
    if (!dirPath.isEmpty()) {
        // [2]
        scanDirectory(dirPath);

        // 경로 저장
        onDirSelected(dirPath);
    }

}

/* [2] 파일 스캔 및 트리 업뎃 */
void CodeShelf::scanDirectory(const QString& path) {    // path가 변하면 안되니깐
    QElapsedTimer timer;
    timer.start();

    // 절대 경로로 기준 정규화
    QFileInfo rootInfo(path);
    QString abRootPath = rootInfo.absoluteFilePath();
    currentRootPath = abRootPath;

    // storage_roots에 경로를 저장하고 ID를 가져옴
    QSqlQuery query;    // 쿼리
    // 1. 해당 경로가 있는지 확인하고 ID가져오기
    query.prepare("SELECT id FROM storage_roots WHERE root_path = :path");
    query.bindValue(":path", abRootPath);

    if (query.exec() && query.next()) {
        qDebug() <<"Update"<< query.lastError().text();
        // 이미 있는 경로면 ID가져오기 아니면 시간 업뎃
        currentRootId = query.value(0).toInt();
        QSqlQuery up;
        up.prepare("UPDATE storage_roots SET past_scanned = NOW() WHERE id = :id");
        up.bindValue(":id", currentRootId);
        up.exec();
    }
    else {
        // 처음 등록하는 경로 INSERT
        qDebug() <<"insert"<< query.lastError().text();
        query.prepare(
            "INSERT INTO storage_roots (root_path, past_scanned)"
            "VALUES (:path, NOW())"
        );
        query.bindValue(":path", abRootPath);
        if (query.exec()) {
            currentRootId = query.lastInsertId().toInt();
        }
    }

    if(currentRootId<=0){
        qDebug() << "RootId확보 실패: " << query.lastError().text();
        return;
    }

    // 캐시 맵 생성
    QHash<QString, QDateTime> dbFileMap;
    QSqlQuery cacheQuery;
    cacheQuery.prepare("SELECT filepath, last_modified FROM codes WHERE root_id = :rid");
    cacheQuery.bindValue(":rid", currentRootId);
    if (cacheQuery.exec()) {
        while (cacheQuery.next()) {
            dbFileMap.insert(cacheQuery.value(0).toString(), cacheQuery.value(1).toDateTime());
        }
    }

    categoryTree->setUpdatesEnabled(false);

    // 트리 정리 및 재귀 스캔 start
    categoryTree->clear();

    // 최상위 루트 아이템 생성
    QTreeWidgetItem* rootItem = new QTreeWidgetItem(categoryTree);
    rootItem->setText(0, rootInfo.fileName());
    rootItem->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
    rootItem->setExpanded(true);    // 일단 펼쳐두기

    // ** 트랜잭션과 재귀 호출 여기서 관리!!**
    // 트랜잭션 기반 스캔 부분
    QSqlDatabase db = QSqlDatabase::database();
    QDir rootDir(currentRootPath);
    if (db.transaction()) {
        if (scanDirRecursive(abRootPath,rootItem, rootDir,dbFileMap)) {
            if (!db.commit()) {
                qDebug() << "커밋 실패!" << db.lastError().text();
                categoryTree->clear();  // UI정리
            }
            loadTagsFromDb();
        }
        else {
            db.rollback();
            categoryTree->clear();  // UI정리
        }
    }
    else {
        qDebug() << "트랜잭션 시작 실패!";
    }
    categoryTree->setUpdatesEnabled(true);
    QApplication::restoreOverrideCursor();

    // [2] 타이머 종료 및 결과 출력
    qint64 elapsed = timer.elapsed(); // 밀리초(ms) 단위
    double seconds = elapsed / 1000.0; // 초 단위 변환

    qDebug() << "========================================";
    qDebug() << "스캔 및 DB 저장 완료!";
    qDebug() << "소요 시간:" << elapsed << "ms (" << seconds << "초)";
    qDebug() << "========================================";
}

// [3] 재귀적으로 폴더 훑기
bool CodeShelf::scanDirRecursive(const QString& path, QTreeWidgetItem* parentItem, const QDir& rootDir, const QHash<QString, QDateTime>& DbFilemap) {
    QDir directory(path);

    // 1. 파일 및 폴더 리스트 가져오기 (숨김파일 제외, 이름순 정렬)
    QFileInfoList entries = directory.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::Name);

    //QApplication::setOverrideCursor(Qt::WaitCursor);
    for (const QFileInfo& info : entries) {
        QString absolutePath = info.absoluteFilePath();

        if (info.isDir()) {
            // [폴더인 경우] 아이콘 설정 후 재귀 호출
            QTreeWidgetItem* folderitem = new QTreeWidgetItem(parentItem);
            folderitem->setText(0, info.fileName());

            static QIcon dirIcon = style()->standardIcon(QStyle::SP_DirIcon);
            folderitem->setIcon(0, dirIcon);

            if (!scanDirRecursive(absolutePath, folderitem, rootDir, DbFilemap)) return false;
        }
        else {
            // [파일인 경우] 확장자 필터링 (C++, SQL, Python 등)
            QString suffix = info.suffix().toLower();
            if (suffix == "cpp" || suffix == "h" || suffix == "sql" || suffix == "py") {
                if (!insertFileRecord(info, absolutePath,DbFilemap)) {
                    return false;   // 하나라도 실패하면 전체 롤백 해야하니깐
                }

                static QIcon fileIcon = style()->standardIcon(QStyle::SP_FileIcon);

                // 트리 UI에 추가(왼쪽편)
                QTreeWidgetItem* item = new QTreeWidgetItem(parentItem);
                item->setText(0, info.fileName());
                item->setIcon(0, fileIcon);
                item->setData(0, Qt::UserRole, absolutePath); // 실제 경로 저장
            }
            
        }
    }
    return true;
}

// [4] 파일 내용읽어서 넣음
bool CodeShelf::insertFileRecord(const QFileInfo& info, const QString absolutePath, const QHash<QString, QDateTime>& dbFilemap) {

    QDateTime fileTime = info.lastModified();

    if (dbFilemap.contains(absolutePath)) {
        QDateTime dbTime = dbFilemap.value(absolutePath);

        // 일단 스트링으로 대조 해보자
        if (dbTime.toString("yyyy-MM-dd HH:mm:ss") == fileTime.toString("yyyy-MM-dd HH:mm:ss")) {
            return true;
        }
        qDebug() << "변경됨" << absolutePath;
    }
    else {

        qDebug() << "신규파일" << absolutePath;
    }


    // [3] 새로 추가되거나 수정된 파일 실행
    QFile file(info.absoluteFilePath());
    QString fileContent = "";
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        in.setEncoding(QStringConverter::Utf8);
        fileContent = in.readAll();
        file.close();
    }

    // SQL(INSERT) 실행
    QSqlQuery q;
    QString extension = info.suffix().toLower();
    QString autoTag = "";

    q.prepare(
        "INSERT INTO codes(root_id, filepath, file_name, extension, file_size, content, last_modified)"
        "VALUES (:root_id, :file_path, :name, :ext, :size, :content, :modified)"
        "ON DUPLICATE KEY UPDATE "  // 중복될때 로직
        "file_size = :size, content = :content, last_modified = :modified"
    );

    q.bindValue(":root_id", currentRootId);
    q.bindValue(":file_path", absolutePath);
    q.bindValue(":name", info.fileName());
    q.bindValue(":ext", info.suffix().toLower());   // suffix로 확장자 추출 및 toLoser로 소문자로 통일
    q.bindValue(":size", info.size()); 
    q.bindValue(":content", fileContent);   // 아까 읽었던 파일 내용
    q.bindValue(":modified", info.lastModified().toString("yyyy-MM-dd HH:mm:ss"));  // 마지막 수정시간을 가져와서 tostring의 형태로 바꿈

    if (extension == "cpp" || extension == "h") autoTag = "#C++ #Source";
    else if (extension == "py") autoTag = "#Python #Script";
    else if (extension == "sql") autoTag = "#Database #SQL";

    q.bindValue(":tags", autoTag);


    if (!q.exec()) {
        // 현재 처리중인 파일이름에서 SQL의 실행 실패(q.lastError) 이유를 반환 해줌 
        qDebug() << "DB 저장 에러 (" << info.fileName() << "): " << q.lastError().text(); // 마지막으로 일어난 에러를 보여줌
        return false;
    }
    return true;
}

// 칩생성
void CodeShelf::loadTagsFromDb() {
    // 1. 기존에 있던 칩 위젯 비우기
    QLayoutItem* item; 
    while ((item = flowlayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }

    // 2. DB연결 재확인 및 쿼리 실행
    QSqlQuery query;
    query.prepare("SELECT DISTINCT upper(extension) FROM codes WHERE root_id = :rid;");
    query.bindValue(":rid", currentRootId);
    if (query.exec()) {
        while (query.next()) {
            QString ext = query.value(0).toString();
            if (ext.isEmpty()) {
                continue;
            }
            // 3. 칩 생성 및 스타일 적용
            QPushButton* chip = new QPushButton("#" + ext);
            chip->setCheckable(true);
            chip->setCursor(Qt::PointingHandCursor);

            // 스타일 시트
            chip->setStyleSheet(
                "QPushButton {"
                "  background-color: #f1f3f4; color: #3c4043; border: 1px solid #dadce0;"
                "  border-radius: 12px; padding: 4px 12px; font-size: 11px; font-weight: 500;"
                "}"
                "QPushButton:hover { background-color: #e8eaed; }"
                "QPushButton:checked { background-color: #e8f0fe; color: #1967d2; border-color: #8ab4f8; }"
            );

            // 4. 레이아웃 추가
            flowlayout->addWidget(chip);

            // 5. 칩 클릭 시 해당 확장자 파일만 필터링
            connect(chip, &QPushButton::clicked, this, [=]() {
                if (chip->isChecked()) {
                    currentPage = 0;
                    filterByExt(ext, 0);
                    updatePagination(ext);
                }
                else {
                    clearCenterLayout();
                    //clearPagination();
                    centerLayout->addStretch(1);
                }
            });
        }
    }
    
}

void CodeShelf::filterByExt(const QString& ext, int offset) {
    // (1) 중앙 레이아웃 비우기 및 현재 확장자 기억
    clearCenterLayout();
    currentSelectedExt = ext;

    // (2) 해당 확장자를 가진 데이터를 SELECT
    QSqlQuery query;
    query.prepare("SELECT file_name, last_modified, extension, filepath FROM codes "
                  "WHERE root_id = :rid AND upper(extension) = :ext "
                  "ORDER BY last_modified DESC "
                  "LIMIT :limit OFFSET :offset"
    
    );
    query.bindValue(":rid", currentRootId);
    query.bindValue(":ext", ext.toUpper());
    query.bindValue(":limit", pageSize);
    query.bindValue(":offset", offset);

    if (query.exec()) {
        int cnt = 0;
        while (query.next()) {
            QDateTime modifiedTime = query.value(1).toDateTime();
            QString formmatedDate = modifiedTime.toString("yyyy-MM-dd");
            QString fullPath = query.value(3).toString();
            addItem(
                centerLayout,
                query.value(0).toString(),
                formmatedDate,
                query.value(2).toString(),
                fullPath);
                
        }
        qDebug() << "검색된 아이템 개수:" << cnt; // 0이 나오면 조건절 문제!
    }
    else {
        qDebug() << "쿼리 실행 실패:" << query.lastError().text();
    }

    centerLayout->addStretch(1);
}

void CodeShelf::updatePagination(const QString& ext, const QString& keyword) {
    // (1) 기존 레이아웃 비우기
    QLayoutItem* child;
    while ((child = paginationBar->takeAt(0)) != nullptr) {
        if (child->widget())  delete child->widget();
        delete child;
    }

    // (2) 해당 확장자의 전체 아이템 개수 가져오기
    QSqlQuery query;
    QString sql = "SELECT COUNT(*) FROM codes WHERE root_id = :rid ";

    if (!ext.isEmpty()) {
        sql += " AND UPPER(extension) = UPPER(:ext) ";
    }

    if (!keyword.isEmpty()) {
        sql += " AND UPPER(file_name) LIKE UPPER(:keyword)";
    }

    query.prepare(sql);
    query.bindValue(":rid", currentRootId);

    if (!ext.isEmpty()) {
        query.bindValue(":ext", ext.toUpper());
    }

    if (!keyword.isEmpty()) {
        query.bindValue(":keyword", "%" + keyword + "%");
    }

    if (query.exec() && query.next()) {
        int totalCnt = query.value(0).toInt();
        int totalPages = (totalCnt + pageSize - 1) / pageSize;
        // (3) 페이지 그룹 계산
        int startPage = (currentPage / pageGroupSize) * pageGroupSize;
        int endPage = qMin(startPage + pageGroupSize, totalPages);

        // (4) [이전] 페이지 버튼 생성
        QPushButton* prevBtn = new QPushButton("<");
        prevBtn->setFixedSize(30, 30);
        prevBtn->setEnabled(startPage > 0);
        connect(prevBtn, &QPushButton::clicked, this, [=]() {
            currentPage = startPage - 1;    // 이전 그룹의 마지막 페이지로 이동
            filterByExt(ext, currentPage * pageSize);
            updatePagination(ext);
        });
        paginationBar->addWidget(prevBtn);

        // (5) [숫자] 페이지 버튼 생성

        for (int i = startPage; i < endPage; i++) {
            QPushButton* pageBtn = new QPushButton(QString::number(i + 1)); // 시작이 0이라
            pageBtn->setFixedSize(30, 30);
            pageBtn->setCheckable(true);

            // 현재 페이지 버튼 강조
            if (i == currentPage) {
                pageBtn->setCheckable(true);
            }
            connect(pageBtn, &QPushButton::clicked, this, [=]() {
                currentPage = i;
                filterBySearch(ext, keyword);
                // filterByExt(ext, i * pageSize); // 해당 페이지 데이터 로드
                updatePagination(ext,keyword);
                });

            paginationBar->addWidget(pageBtn);
        }

        // (6) [다음] 페이지 버튼 생성
        QPushButton* nextBtn = new QPushButton(">");
        nextBtn->setFixedSize(30, 30);
        nextBtn->setEnabled(endPage<totalPages);
        connect(nextBtn, &QPushButton::clicked, this, [=]() {
            currentPage = endPage;    // 이전 그룹의 마지막 페이지로 이동
            filterByExt(ext, currentPage * pageSize);
            updatePagination(ext);
            });
        paginationBar->addWidget(nextBtn);
    }
}

void CodeShelf::clearCenterLayout() {
    if (!centerLayout) return;

    // 뒤에서부터 지우기
    // 그래야 오류가 안남
    // i > 0 으로 설정, 검색창 보호
    while(centerLayout->count() >1) {
        QLayoutItem* item = centerLayout->takeAt(0);
        
        if (item) {
            // 1. 위젯
            if (QWidget* widget = item->widget()) {
                widget->hide();
                delete widget;
            }
            else if(item->spacerItem()){
                // 2. 스페이서(addStretch)인 경우도 삭제 대상
                delete item->spacerItem();
            }
            else {
                delete item;
            }
        }
    }
}

void CodeShelf::clearPagination() {
    if (!paginationBar) return;

    QLayoutItem* item;
    while ((item = paginationBar->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->hide();
            delete item->widget();
        }
        delete item;
    }
}

void CodeShelf :: onTreeItemClicked(QTreeWidgetItem* item, int column) {
    // 상대경로가 나옴
    QString fullPath = item->data(0, Qt::UserRole).toString();   // (1) 꺼내기
    if (fullPath.isEmpty()) return;// 경로 is Empty == 폴더, -> 무시

    QFile file(fullPath);

    // 파일 열어
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {    // readonly, 텍스트모드
        qDebug() << "파일을 열 수 없습니다." << file.errorString();
        codePreview->setPlainText("// Error: 파일을 찾을 수 없습니다.\n// 경로: " + fullPath);
        return;
    }

    // 텍스트 스트림으로 내용 읽기
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8); // 한글깨짐을 방지해주자
    QString content = in.readAll();

    // 미리보기 창 데이터 세팅
    codePreview->setPlainText(content);

    // 상단 정보 레이블을 업뎃
    
    QFileInfo fileInfo(fullPath);
    lblPName->setText("File : " + fileInfo.fileName());
    lblComment->setText("// Path : " + fileInfo.absoluteFilePath());

    file.close();
    
}

/* [page 0] 대쉬보드 홈 */
void CodeShelf::setupDashboard() {
    QLabel* homeLabel = new QLabel("대시보드 화면 (준비 중)");
    homeLabel->setAlignment(Qt::AlignCenter);
    mainStackedWidget->addWidget(homeLabel);
}
/* [page 1]3분할 메인 */
void CodeShelf::setupManagementPage() {
    // [page 1] 3분할 관리 화면
    mainSplitter = new QSplitter(Qt::Horizontal);

    // left : 카테고리 트리, 태그
    leftWidget = new QWidget();
    leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(10, 10, 10, 10); // 전체 여백 추가
    leftLayout->setSpacing(10);   // 아이템들 사이 간격 10px고정

    // left : 수직 스플리터
    QSplitter* leftVSplitter = new QSplitter(Qt::Vertical);

    // 카테고리 트리
    categoryTree = new QTreeWidget();
    categoryTree->setHeaderLabel("Categories");

    // 태그
    // 태그 제목
    QLabel* tagLabel = new QLabel("태그");
    tagLabel->setStyleSheet("font-weight : bold; color: #555;");
    leftLayout->addWidget(tagLabel);

    // 태그 : 스크롤 영역
    QScrollArea* tagScrollArea = new QScrollArea();
    tagScrollArea->setWidgetResizable(true);    // 내부 위젯 크기 조절(자동)
    tagScrollArea->setFrameShape(QFrame::NoFrame);   // 프레임 지우기(테두리)
    tagScrollArea->setStyleSheet("background-color: transparent;"); // 배경투명

    // 태그 : 실제로 칩들이 올라갈 컨테이너 위젯 부분
    QWidget* tagContainer = new QWidget();
    tagContainer->setStyleSheet("background-color: transparent;");

    // 태그 : 플로우 레이아웃 적용(자동 줄바꿈)
    flowlayout = new FlowLayout(tagContainer);
    flowlayout->setContentsMargins(0, 0, 0, 0);
    flowlayout->setSpacing(5);  // 칩들 사이 간격

    // 태그 : 스크롤 영역에 컨테이터
    tagScrollArea->setWidget(tagContainer);

    // 왼쪽 레이아웃에 붙이기
    leftVSplitter->addWidget(categoryTree);
    leftVSplitter->addWidget(tagScrollArea);

    // 스플리터 초기비율 (트리7, 태그3)
    leftVSplitter->setStretchFactor(0, 6);
    leftVSplitter->setStretchFactor(1, 4);


    leftLayout->addWidget(leftVSplitter);

    // center : 리스트, 검색창
    QWidget* centerWidget = new QWidget();
    centerMainLayout = new QVBoxLayout(centerWidget);

    // 검색 창 설정
    QLineEdit* searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("검색어를 입력하세요...");
    centerMainLayout->addWidget(searchEdit);    // 검색창 맨 위 고정
    connect(searchEdit, &QLineEdit::textChanged, this, &CodeShelf::onSearchTextChanged);

    // 중앙 스크롤 영역 생성
    QScrollArea* centerScroll = new QScrollArea();
    centerScroll->setWidgetResizable(true);
    QWidget* scrollContent = new QWidget();
    centerLayout = new QVBoxLayout(scrollContent);
    // 중간에 빈공간 하나
    centerLayout->addStretch(1);
    centerScroll->setWidget(scrollContent);
    centerMainLayout->addWidget(centerScroll);
    
    centerScroll->setFrameShape(QFrame::NoFrame);

    // 하단 바(페이지 버튼이 들어갈 곳)
    QWidget* bottomBarWidget = new QWidget();
    bottomBarWidget->setFixedHeight(50);

    paginationBar = new QHBoxLayout(bottomBarWidget);
    paginationBar->setContentsMargins(0, 0, 0, 0);
    paginationBar->setSpacing(5);
    paginationBar->setAlignment(Qt::AlignCenter);
    centerMainLayout->addWidget(bottomBarWidget);

    //addItem(centerLayout, "Login_Module.cpp", "2026-04-06", "#Network");
    //addItem(centerLayout, "Database_Connect.sql", "2026-04-05", "#SQL");


    // right : 미리보기 및 버튼
    QWidget* rightWidget = new QWidget();
    QVBoxLayout* rightMainLayout = new QVBoxLayout(rightWidget);

    // right-T 위쪽 위젯 (정보 영역)
    QWidget* infoWidget = new QWidget();
    QVBoxLayout* infoLayout = new QVBoxLayout(infoWidget);  // 정보들도 위아래로 쌓을꺼니깐
    infoLayout->setContentsMargins(0, 0, 0, 5); // 아래쪽 마진주기
    infoWidget->setStyleSheet("background-color:white");

    lblPName = new QLabel("Project : Code-1231");
    lblPName->setStyleSheet("font-size:18px; font-weight:bold");

    lblComment = new QLabel("// 파일입출력 기본예제");
    lblComment->setStyleSheet("color:gray");

    // 태그들을 위한 공간(가로로나열)
    QWidget* tagBar = new QWidget();
    QHBoxLayout* tagLayout = new QHBoxLayout(tagBar);
    tagLayout->setContentsMargins(0, 0, 0, 0);

    infoLayout->addWidget(lblPName);
    infoLayout->addWidget(lblComment);
    infoLayout->addWidget(tagBar);

    // 테스트용 칩 추가
    //QStringList testTags = { "#CPP", "#SQL", "#PYTrHON", "#H", "#QT6", "#VS2022" };
    //for (const QString& tag : testTags) {
    //    QPushButton* chip = new QPushButton(tag);
    //    chip->setStyleSheet(
    //        "QPushButton{"
    //        "   background-color: #ffffff; color: #01579b; border: 1px solid #b3e5fc;"
    //        "   border-radius: 12px; padding: 4px 10px; font-size: 11px; "
    //        "}"
    //        "QPushButton:hover { background-color: #b3e5fc; }"
    //    );
    //    flowlayout->addWidget(chip);   
    //}

    loadTagsFromDb();

    // right-B 아래쪽 위젯(코드 영역)
    codePreview = new QTextEdit();
    codePreview->setReadOnly(true); // 수정불가하게
    QPushButton* btnCopy = new QPushButton("클립보드 복사");

    // right 합체
    rightMainLayout->addWidget(infoWidget);
    rightMainLayout->addWidget(codePreview);
    rightMainLayout->addWidget(btnCopy);
    rightMainLayout->setStretch(1, 1);

    // 코드창과 정보창 크기 비율 조정
    rightMainLayout->setStretchFactor(infoWidget, 2);
    rightMainLayout->setStretchFactor(codePreview, 8);


    mainSplitter->addWidget(leftWidget);
    mainSplitter->addWidget(centerWidget);
    mainSplitter->addWidget(rightWidget);

    leftWidget->setMinimumWidth(200);
    centerWidget->setMinimumWidth(400);
    rightWidget->setMinimumWidth(300);

    mainStackedWidget->addWidget(mainSplitter);

    mainStackedWidget->setCurrentIndex(0);
}
// 아이템 추가
void CodeShelf::addItem(QVBoxLayout* targetLayout, QString name, QString date, QString tag, QString fullPath) {
    // 2. 아이템용 Widget과 Layout 만들기
    QWidget* itemWidget = new QWidget();
    itemWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    QVBoxLayout* cItemLayout = new QVBoxLayout(itemWidget);

    // 3. 위쪽 줄 구성
    QHBoxLayout* ctopLayout = new QHBoxLayout();
    QLabel* cIconLabel = new QLabel("📄");   // 나중에 교체할 것
    QLabel* fileName = new QLabel(name);
    fileName->setStyleSheet("font-weight:bold");

    QLabel* dateLabel = new QLabel();
    dateLabel->setStyleSheet("color:gray; font-size:11px;");

    dateLabel->setText(formatDate(date));
    itemWidget->setProperty("fullPath", fullPath);

    cIconLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    fileName->setAttribute(Qt::WA_TransparentForMouseEvents);
    dateLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

    ctopLayout->addWidget(cIconLabel);
    ctopLayout->addWidget(fileName);
    ctopLayout->addStretch();    // 오른쪽으로 밀기
    ctopLayout->addWidget(dateLabel);

    // 4. 아래쪽줄 구성(tag)
    QHBoxLayout* ctagLayout = new QHBoxLayout();
    QLabel* tag1 = new QLabel(tag);
    tag1->setStyleSheet("background: #eee; border-radius: 5px; padding: 2px;");
    ctagLayout->addWidget(tag1);
    ctagLayout->addStretch();   // 왼쪽 태그정렬
    tag1->setAttribute(Qt::WA_TransparentForMouseEvents);

    // 5. 아이템 레이아웃 합체
    cItemLayout->addLayout(ctopLayout);
    cItemLayout->addLayout(ctagLayout);

    // 위젯 감시 및 데이터 심기
    itemWidget->installEventFilter(this);
    itemWidget->setProperty("fileName", name);
    itemWidget->setProperty("fileDate", date); 


    itemWidget->setAttribute(Qt::WA_Hover); // 호버 효과 활성화
    itemWidget->setMouseTracking(true);     // 마우스 추적 활성화 (선택사항)
    itemWidget->setCursor(Qt::PointingHandCursor);      // 마우스를 올리면 손가락 모양
    
    itemWidget->setStyleSheet(
        "QWidget { background-color: white; border: 1px solid #ddd; border-radius: 5px; }"
        "QWidget:hover { background-color: #f0f7ff; border: 1px solid #0078d7; }" // 호버 시 푸른 테두리 (선택)
        "QLabel { border: none; background: transparent; }" // 자식 라벨들의 테두리와 배경 제거
    );

    // 완성된 위젯을 전체 센터 레이아웃에 추가
    targetLayout->insertWidget(targetLayout->count() - 1, itemWidget);
}

QString CodeShelf::formatDate(const QString& rawDate) {
    if (rawDate.contains(" ")) return rawDate.split(" ")[0];
    return rawDate;
}

void CodeShelf::onSearchTextChanged(const QString& text) {
    // 현재 어떤 태그가 선택되어있는지
    // 현재 선택된 확장자가 없으면 전체검색으로
    // 
    // 검색시 첫페이지 부터 보여주기
    currentPage = 0;

    // 필터링 함수 호출(검색어 포함)
    filterBySearch(currentSelectedExt, text);

}
void CodeShelf::filterBySearch(const QString& ext, const QString& keyword) {
    clearCenterLayout();

    QSqlQuery q;
    QString sql = "SELECT file_name, last_modified, extension, filepath FROM codes "
                  "WHERE root_id = :rid ";

    if (!ext.isEmpty()) {
        sql += " AND UPPER(extension) = UPPER(:ext) ";
    }

    if (!keyword.trimmed().isEmpty()) {
        sql += " AND UPPER(file_name) LIKE UPPER(:keyword) ";
    }
    sql += " LIMIT :limit OFFSET :offset ";

    q.prepare(sql);
    q.bindValue(":rid", currentRootId);

    if (!ext.isEmpty()) {
        q.bindValue(":ext", ext.toUpper());
    }

    if (!keyword.trimmed().isEmpty()) {
        q.bindValue(":keyword", "%" + keyword + "%");
    }

    q.bindValue(":limit", pageSize);
    q.bindValue(":offset", currentPage*pageSize);

    if (q.exec()) {
        int cnt = 0;
        while (q.next()) {
            QDateTime modifiedTime = q.value(1).toDateTime();
            QString name = q.value(0).toString();
            QString date = modifiedTime.toString("yyyy-MM-dd");
            QString tag = q.value(2).toString();
            QString path = q.value(3).toString();

            // 리스트 아이템 생성 및 레이아웃 추가
            addItem(centerLayout, name, date, tag, path);
            cnt++;
        }
        centerLayout -> addStretch(1);
        if (cnt==0) {
            qDebug() << "검색결과가 없습니다";
        }
    }
    else {
        qDebug() << "SQL 에러" << q.lastError().text();

    }
    updatePagination(ext, keyword);
}

// 클릭 감지(센터 아이템)
bool CodeShelf::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

        if (mouseEvent->button() == Qt::LeftButton) {
            QWidget* clikedWidget = qobject_cast<QWidget*>(obj);
            if (clikedWidget) {
                QString name = clikedWidget->property("fileName").toString();
                QString path = clikedWidget->property("fullPath").toString();
                qDebug() << "--- 클릭 감지 ---";
                qDebug() << "파일명:" << name;
                qDebug() << "경로:" << path;

                if (path.isEmpty()) {
                    qDebug() << "경로 데이터가 비어있습니다! addItem의 setProperty를 확인하세요.";
                    return false;
                }
                // 상세보기 함수 호출
                showDetail(name, path);
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
} 
void CodeShelf::showDetail(const QString& name, const QString& path) {
    lblPName->setText("File : " + name);
    lblComment->setText("// Location" + path);

    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        in.setEncoding(QStringConverter::Utf8);
        codePreview->setPlainText(in.readAll());
        file.close();
    }
    else {
        codePreview->setPlainText("// 파일을 열 수 없습니다\n// 경로 : "+ path);
    }
}

void CodeShelf::applyEditorTheme() {
    // 폰트 설정
    QFont codeFont("Consolas", 11);
    codeFont.setStyleHint(QFont::Monospace);
    codePreview->setFont(codeFont);

    // 탭 간격 설정
    const int tabStop = 4;
    QFontMetrics metrics(codeFont);
    codePreview->setTabStopDistance(tabStop * metrics.horizontalAdvance(' '));

    // 다크 테마 스타일 시트 적용
    codePreview->setStyleSheet(
        "QTextEdit {"
        "   background-color: #1e1e1e; color: #d4d4d4;"
        "   border: none; padding: 10px;"
        "}"
        "QScrollBar:vertical {"
        "   background: #252526; width: 12px;"
        "}"
        "QScrollBar::handle:vertical { background: #424242; min-height: 20px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
    );
}


void CodeShelf::showHome() {
}

void CodeShelf::toggleSearchMode() {
    mainStackedWidget->setCurrentIndex(1);
}
QString CodeShelf::elidePath(const QString& path, int maxLength) {
    if (path.length() <= maxLength) return path;

    return "..." + path.right(maxLength);
}

void CodeShelf::initDatabase() {
    // 현재 로드 가능한 드라이버 목록을 출력해서 확인 (디버깅용)
    qDebug() << "Available Drivers:" << QSqlDatabase::drivers();

    // MySQL 드라이버를 로드
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");

    // 접속 정보
    db.setHostName("127.0.0.1");
    db.setDatabaseName("CODESHELF"); // MySQL에서 생성한 DB 이름과 일치해야 함
    db.setUserName("shl_user");     // 생성한 계정명
    db.setPassword("rcg6510@");

    // 연결 확인
    if (!db.open()) {
        qDebug() << "DB 연결 실패 사유: " << db.lastError().text();
    }
    else {
        qDebug() << "DB 연결 성공!";
    }
}

CodeShelf::~CodeShelf()
{}