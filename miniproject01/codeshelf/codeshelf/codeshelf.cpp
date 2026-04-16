#include "codeshelf.h"

CodeShelf::CodeShelf(QWidget *parent)
    : QMainWindow(parent)
{
    /* 데이터베이스 접속*/
    if (!DatabaseManager::instance().connectDB()) {
        QMessageBox::critical(this, "에러", "데이터베이스에 연결할 수 없습니다");
    }
    /* [1] 전체 베이스 초기설정 */
    centerWidget = new QWidget(this);
    mainLayout = new QVBoxLayout(centerWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0); // 상하좌우 마진제거
    mainLayout->setSpacing(0);  // 위젯 간의 간격을 0으로
    setCentralWidget(centerWidget);
    this->installEventFilter(this);

    /* [2] 상단바 영역 */
    setupTopBar();



    /* [3] 메인 콘텐츠 영역(Stacked Widget )*/
    mainStackedWidget = new QStackedWidget();
    mainLayout->addWidget(mainStackedWidget);
    mainStackedWidget->setStyleSheet("background-color:#333");

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
    connect(btnSelectRoot, &QPushButton::clicked, this, &CodeShelf::onSelectRootFolder);
    connect(btnHome, &QPushButton::clicked, this, &CodeShelf::showHome);
    connect(btnSearchToggle, &QPushButton::clicked, this, &CodeShelf::toggleSearchMode);
    connect(categoryTree, &QTreeWidget::itemClicked, this, &CodeShelf::onTreeItemClicked);

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

    //btnHome = new QPushButton("홈");
    //btnSearchToggle = new QPushButton("검색/관리");

    logo->setStyleSheet("font-weight: bold; font-size: 18px; margin-left:10px;");

    QLabel* userInfo = new QLabel("user 1234 👤");

    topLayout->addWidget(logo);
    topLayout->addStretch();    // 중간 여백
    topLayout->addWidget(btnHome);
    topLayout->addWidget(btnSearchToggle);
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
void CodeShelf::scanDirectory(const QString& path) {
    // 트리 업데이트 시간 간격 주기용
    QElapsedTimer timer;
    timer.start();

    // (1) 경로 정규화 및 UI 설정
    QFileInfo rootInfo(path);
    currentRootPath = rootInfo.absoluteFilePath();

    // (2) 루트 ID 확보 (manager에 넣을)
    currentRootId = DatabaseManager::instance().getOrCreateRootID(currentRootPath);
    if (currentRootId <= 0) return;

    // (3) 캐시 맵 가져오기
    QHash<QString, QDateTime> dbFileMap = DatabaseManager::instance().getFileModificationMap(currentRootId);

    // (4) UI 초기화
    categoryTree->setUpdatesEnabled(false);
    categoryTree->clear();

    // 최상위 루트 아이템 생성
    QTreeWidgetItem* rootItem = new QTreeWidgetItem(categoryTree);
    rootItem->setText(0, rootInfo.fileName());
    // rootItem->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
    rootItem->setExpanded(true);    // 일단 펼쳐두기

    // (5) 스캔 및 트랜잭션 관리 
    if (DatabaseManager::instance().beginTransaction()) { 
        QDir rootDir(currentRootPath);
        if (scanDirRecursive(currentRootPath,rootItem, rootDir, dbFileMap)) {
            DatabaseManager::instance().commitTransaction();
            loadTagsFromDb();
        }
        else {
            DatabaseManager::instance().rollbackTransaction();
            categoryTree->clear();
        }
    }
    categoryTree->setUpdatesEnabled(true);
    filterBySearch("", "", "all");
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
            //folderitem->setIcon(0, dirIcon); 
            folderitem->setData(0, Qt::UserRole, absolutePath);

            if (!scanDirRecursive(absolutePath, folderitem, rootDir, DbFilemap)) return false;
        }
        else {
            // [파일인 경우] 확장자 필터링 (C++, SQL, Python 등)
            QString suffix = info.suffix().toLower();
            if (suffix == "c" || suffix == "cpp" || suffix == "h" || suffix == "sql" || suffix == "py") {
                if (!insertFileRecord(info, absolutePath,DbFilemap)) {
                    return false;   // 하나라도 실패하면 전체 롤백 해야하니깐
                }

                static QIcon fileIcon = style()->standardIcon(QStyle::SP_FileIcon);

                // 트리 UI에 추가(왼쪽편)
                QTreeWidgetItem* item = new QTreeWidgetItem(parentItem);
                item->setText(0, info.fileName());
                //item->setIcon(0, fileIcon);
                item->setData(0, Qt::UserRole, absolutePath); // 실제 경로 저장
            }
            
        }
    }

    return true;
}

// [4] 파일 내용읽어서 넣음
bool CodeShelf::insertFileRecord(const QFileInfo& info, const QString absolutePath, const QHash<QString, QDateTime>& dbFilemap) {

    QDateTime fileTime = info.lastModified();

    // (1) 변경 여부 확인: 기존 DB 시간과 파일 수정 시간이 같으면 스캔을 skip
    if (dbFilemap.contains(absolutePath)) {
        QDateTime dbTime = dbFilemap.value(absolutePath);
        if (dbTime.toString("yyyy-MM-dd HH:mm:ss") == fileTime.toString("yyyy-MM-dd HH:mm:ss")) {
            return true;
        }
        qDebug() << "변경됨" << absolutePath;
    }
    else qDebug() << "신규파일" << absolutePath;


    // (2) 파일 내용 읽기
    QFile file(info.absoluteFilePath());
    QString fileContent = "";
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        in.setEncoding(QStringConverter::Utf8);
        fileContent = in.readAll();
        file.close();
    }
    
    // (3) DatabaseManager에 전달할 데이터 묶기
    FileRecord record;
    record.rootId = currentRootId;
    record.filePath = absolutePath;
    record.fileName = info.fileName();
    record.extension = info.suffix().toLower();
    record.fileSize = info.size();
    record.content = fileContent;
    record.lastModified = fileTime;

    // (4) 매니저에게 저장 요청
    return DatabaseManager::instance().insertFileRecord(record);

}

// 칩생성
void CodeShelf::loadTagsFromDb() {
    if (!tagList) {
        qDebug() << "tagList가 NULL입니다!";
        return;
    }
    if (!tagList) return;
    // 1. 기존에 있던 위젯 비우기
    tagList->clear();

    qDebug() << "여기요";

    SearchOptions opt;
    opt.rootId = currentRootId;
    opt.searchMode = "name";
    opt.keyword = "";

    // ALL 버튼
    allTagBtn();

    QStringList extension = DatabaseManager::instance().getExtensionByRootId(currentRootId);
    qDebug() << "확장자 리스트" << extension;
    qDebug() << "currentRootId:" << currentRootId;
    for (const QString& ext : extension) {
        opt.extension = ext;
        int count = DatabaseManager::instance().getFileCount(opt);

        QListWidgetItem* item = new QListWidgetItem(tagList);
        item->setData(Qt::UserRole, ext);

        QWidget* row = new QWidget();
        QHBoxLayout* layout = new QHBoxLayout(row);
        layout->setContentsMargins(8, 4, 8, 4);

        QLabel* tagLabel = new QLabel("#" + ext);
        QLabel* countLabel = new QLabel(QString::number(count));
        countLabel->setStyleSheet("color: gray;");

        layout->addWidget(tagLabel);
        layout->addStretch();
        layout->addWidget(countLabel);

        item->setSizeHint(row->sizeHint());
        tagList->addItem(item);
        tagList->setItemWidget(item, row);
    }

    disconnect(tagList, nullptr, this, nullptr);

    connect(tagList, &QListWidget::itemClicked, this, [=](QListWidgetItem* item) {
        QString ext = item->data(Qt::UserRole).toString();

        if (ext == "ALL") ext = "";

        this->currentSelectedExt = ext;
        this->currentPage = 0;

        QString mode = searchFilterCombo->currentData().toString();

        filterBySearch(ext, searchEdit->text(), mode);
        updatePagination(ext, searchEdit->text(), mode);

    });

}
// ALL 태그 버튼
void CodeShelf::allTagBtn() {
    qDebug() << "allTagBtn 시작";
    SearchOptions opt;
    opt.rootId = currentRootId;
    opt.searchMode = "name";
    opt.extension = "";
    opt.keyword = "";

    int count = DatabaseManager::instance().getFileCount(opt);
    
    QListWidgetItem* item = new QListWidgetItem(tagList);
    item->setData(Qt::UserRole, "ALL");

    QWidget* row = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(row);
    layout->setContentsMargins(8, 4, 8, 4);

    QLabel* tagLabel = new QLabel("#ALL");
    QLabel* countLabel = new QLabel(QString::number(count));

    layout->addWidget(tagLabel);
    layout->addStretch();
    layout->addWidget(countLabel);

    item->setSizeHint(row->sizeHint());
    tagList->addItem(item);
    tagList->setItemWidget(item, row); qDebug() << "allTagBtn 끝";
}

void CodeShelf::updatePagination(const QString& ext, const QString& keyword, const QString& mode) {
    // (1) 기존 레이아웃 비우기
    clearPagination();

    // (2) 해당 확장자의 전체 아이템 개수 가져오기

    int totalCnt = cachedResults.size();

    if (totalCnt <= 0) return;

    int totalPages = (totalCnt + pageSize - 1) / pageSize;

    // (3) 페이지 그룹 계산
    int startPage = (currentPage / pageGroupSize) * pageGroupSize;
    int endPage = qMin(startPage + pageGroupSize, totalPages);

    // (4) [이전] 페이지 버튼 생성
    QPushButton* prevBtn = new QPushButton("<");
    prevBtn->setFixedSize(30, 30);
    prevBtn->setEnabled(startPage > 0);

    connect(prevBtn, &QPushButton::clicked, this, [=]() {
        currentPage = qMax(0,startPage - pageGroupSize);    // 이전 그룹의 마지막 페이지로 이동
        renderPage();
        });

    paginationBar->addWidget(prevBtn);

    // (5) [숫자] 페이지 버튼 생성
    QButtonGroup* pageGroup = new QButtonGroup(this);
    pageGroup->setExclusive(true);
    for (int i = startPage; i < endPage; i++) {
        QPushButton* pageBtn = new QPushButton(QString::number(i + 1)); // 시작이 0이라
        pageBtn->setFixedSize(30, 30);
        pageBtn->setCheckable(true);

        
        pageGroup->addButton(pageBtn, i);
        if (i==currentPage) {
            pageBtn->setChecked(true);
        }

        connect(pageBtn, &QPushButton::clicked, this, [=]() {
            currentPage = i;
            renderPage();
            });

        paginationBar->addWidget(pageBtn);

        pageBtn->setStyleSheet("background-color: #54535E; color:white;");
    }

    // (6) [다음] 페이지 버튼 생성
    QPushButton* nextBtn = new QPushButton(">");
    nextBtn->setFixedSize(30, 30);
    nextBtn->setEnabled(endPage < totalPages);
    connect(nextBtn, &QPushButton::clicked, this, [=]() {
        currentPage = endPage;    // 이전 그룹의 마지막 페이지로 이동
        renderPage();
        });
    paginationBar->addWidget(nextBtn);

    prevBtn->setStyleSheet("background-color: #54535E; color:white;");
    nextBtn->setStyleSheet("background-color: #54535E; color:white;");
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
    // 1. UserRole에 저장된 경로 꺼내기
    QString path = item->data(0, Qt::UserRole).toString(); 
    if (path.isEmpty()) return;// 경로 is Empty == 폴더, -> 무시

    QFileInfo fileInfo(path);
    if (!fileInfo.isFile()) return;


    showDetail(fileInfo.fileName(), path);
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
    leftWidget->setStyleSheet("background-color: #3B3A42; color:white;");
    // left : 수직 스플리터
    QSplitter* leftVSplitter = new QSplitter(Qt::Vertical);

    // 카테고리 트리pp

    QWidget* treeSection = new QWidget();
    QVBoxLayout* treeLayout = new QVBoxLayout(treeSection);
    treeLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* treeLabel = new QLabel("Categories");
    treeLabel->setStyleSheet("font-weight: bold; color: #aaa;");

    categoryTree = new QTreeWidget();
    categoryTree->setHeaderHidden(true);

    treeLayout->addWidget(treeLabel);
    treeLayout->addWidget(categoryTree);

    // 태그
    // 태그 제목
    QLabel* tagLabel = new QLabel("태그");
    tagLabel->setStyleSheet("font-weight : bold; color: #555;");
    //leftLayout->addWidget(tagLabel);
    // leftLayout->addStretch();

    btnSelectRoot = new QPushButton("폴더 선택");
    leftLayout->addWidget(btnSelectRoot);
    btnSelectRoot->setStyleSheet("background-color:#555; padding:5px;");

    // 태그 : 스크롤 영역
    QWidget* tagSection = new QWidget();
    QVBoxLayout* tagSectionLayout = new QVBoxLayout(tagSection);
    tagSectionLayout->setContentsMargins(0, 0, 0, 0);

    // 제목
    tagLabel = new QLabel("태그");
    tagLabel->setStyleSheet("font-weight: bold; color: #aaa;");

    // 리스트
    tagList = new QListWidget();
    tagList->setStyleSheet("background: transparent; border: none;");

    // 스크롤
    QScrollArea* tagScrollArea = new QScrollArea();
    tagScrollArea->setWidgetResizable(true);
    tagScrollArea->setFrameShape(QFrame::NoFrame);
    tagScrollArea->setStyleSheet("background-color: #54535E;");
    tagScrollArea->setMinimumHeight(250);
    tagScrollArea->setWidget(tagList);  // ⭐ 핵심 (딱 하나만!)

    tagSectionLayout->addWidget(tagLabel);
    tagSectionLayout->addWidget(tagScrollArea);

    // 왼쪽 레이아웃에 붙이기
    leftVSplitter->addWidget(treeSection);
    leftVSplitter->addWidget(tagSection);

    // 스플리터 초기비율 (트리7, 태그3)
    leftVSplitter->setStretchFactor(0, 6);
    leftVSplitter->setStretchFactor(1, 4);


    leftLayout->addWidget(leftVSplitter);

    // center : 리스트, 검색창
    QWidget* centerWidget = new QWidget();
    centerMainLayout = new QVBoxLayout(centerWidget);
    centerWidget->setStyleSheet("background-color:#282828");

    // 검색 창 설정
    setupSearchUI(); 
    centerMainLayout->insertLayout(0, searchLayout);

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

    // right : 미리보기 및 버튼
    QWidget* rightWidget = new QWidget();
    QVBoxLayout* rightMainLayout = new QVBoxLayout(rightWidget);
    rightMainLayout->setContentsMargins(0, 0, 0, 0);
    rightMainLayout->setSpacing(0);

    // right-T 위쪽 위젯 (정보 영역)
    QWidget* infoWidget = new QWidget();
    infoWidget->setStyleSheet("background-color: #ffffff; border-bottom: 1px solid #ddd;");
    QVBoxLayout* infoLayout = new QVBoxLayout(infoWidget);  // 정보들도 위아래로 쌓을꺼니깐
    infoLayout->setContentsMargins(15, 15, 15, 15);
    infoLayout->setSpacing(8);

    // 첫째줄 파일이름, 날짜
    QWidget* titleLine = new QWidget();
    QVBoxLayout* titleLayout = new QVBoxLayout(titleLine);
    titleLayout->setContentsMargins(0, 0, 0, 0);

    lblFileName = new QLabel("Project : Code-1231");
    lblFileName->setStyleSheet("font-size:18px; font-weight:bold; color: white;");

    lblFileDate = new QLabel("2024-10-10 "); 
    lblFileDate->setStyleSheet("font-size: 12px; color: #ddd;");

    titleLayout->addWidget(lblFileName);
    titleLayout->addStretch();
    titleLayout->addWidget(lblFileDate);
    infoWidget->setStyleSheet("background-color:#54535E;");

    // 둘째줄 파일 위치
    lblFilePath = new QLabel("");
    lblFilePath->setStyleSheet("font-size: 12px; color: #ddd; font-family: 'Consolas';");
    lblFilePath->setWordWrap(true);

    // 셋째줄 태그 영역
    tagContainer = new QWidget();
    tagLayout = new QHBoxLayout(tagContainer);;
    tagLayout->setContentsMargins(0, 5, 0, 0);
    tagLayout->setSpacing(5);
    tagLayout->addStretch();

    // infoLayout에 추가
    infoLayout->addWidget(titleLine);
    infoLayout->addWidget(lblFilePath);
    infoLayout->addWidget(tagContainer);


    // right-B 아래쪽 위젯(코드 영역)
    codePreview = new QTextEdit();
    codePreview->setStyleSheet(
        "QTextEdit {"
        "   background-color: #1e1e1e; color: #d4d4d4;"
        "   font-family: 'Consolas', 'Monospace';"
        "   font-size: 10pt; border: none; padding: 11px;"
        "}"
    );
    new CodeHighlighter(codePreview->document());
    codePreview->setReadOnly(true); // 수정불가하게
    highlighter = new CodeHighlighter(codePreview->document());

    // 아래 버튼
    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* btnCopy = new QPushButton("코드 복사");
    QPushButton* btnDir = new QPushButton("폴더 열기");

    QString btnStyle = "color:white;";

    btnCopy->setStyleSheet(btnStyle);
    btnDir->setStyleSheet(btnStyle);

    btnLayout->addWidget(btnCopy);
    btnLayout->addWidget(btnDir);

    connect(btnCopy, &QPushButton::clicked, this, &CodeShelf::onCopyClicked);
    connect(btnDir, &QPushButton::clicked, this, &CodeShelf::onOpenDirClicked);

    // right 합체
    rightMainLayout->addWidget(infoWidget);
    rightMainLayout->addWidget(codePreview,1);
    rightMainLayout->addLayout(btnLayout);

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
    QLabel* tag1 = new QLabel("#"+tag);
    tag1->setStyleSheet("background: #FFD864; border-radius: 5px; padding: 2px 2px 4px 4px; color:black;");
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
    itemWidget->setMouseTracking(true);     // 마우스 추적 활성화
    itemWidget->setCursor(Qt::PointingHandCursor);      // 마우스를 올리면 손가락 모양
    
    itemWidget->setStyleSheet(
        "QWidget { background-color: #54535E; border: 1px solid #575757; border-radius: 5px; color:white }"
        "QWidget:hover { background-color: #302F36; border: 1px solid white; }"
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
    QString mode = searchFilterCombo->currentData().toString();
    // 검색시 첫페이지 부터 보여주기
    currentPage = 0;

    // 필터링 함수 호출(검색어 포함)
    filterBySearch(currentSelectedExt, text, searchFilterCombo->currentData().toString());

}
void CodeShelf::filterBySearch(const QString& ext, const QString& keyword, const QString& mode) {
    clearCenterLayout();

    currentSearch.rootId = currentRootId;
    currentSearch.extension = ext;
    currentSearch.keyword = keyword;
    currentSearch.mode = mode;

    currentPage = 0;

    SearchOptions opt;
    opt.rootId = currentRootId;
    opt.extension = ext;
    opt.keyword = keyword;
    opt.searchMode = mode;

    opt.limit = 100000;
    opt.offset = 0;

    // (2) 매니저에게 데이터 요청
    QList<FileItem> files = DatabaseManager::instance().fetchFiles(opt);
    cachedResults= DatabaseManager::instance().fetchFiles(opt);
    renderPage();

    updatePagination(ext, keyword, mode);
}

void CodeShelf::renderPage() {
    clearCenterLayout();

    int start = currentPage * pageSize;
    int end = qMin(start + pageSize, (int)cachedResults.size());

    for (int i = start; i < end; i++) {
        const auto& file = cachedResults[i];
        addItem(centerLayout,
            file.name,
            file.lastModified.toString("yyyy-MM-dd"),
            file.extension,
            file.path
        );
    }

    centerLayout->addStretch(1);

    updatePagination("", "", "");
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
    qDebug() << "showDetail 진입";
    qDebug() << "highlighter:" << highlighter;
    qDebug() << "lblFileName:" << lblFileName;
    qDebug() << "lblFileDate:" << lblFileDate;
    qDebug() << "lblFilePath:" << lblFilePath;
    qDebug() << "tagLayout:" << tagLayout;
    qDebug() << "codePreview:" << codePreview;
    QFileInfo fileInfo(path);
    currentFilePath = path;
    // 기본정보세팅
    lblFileName->setText(name);
    lblFileDate->setText(fileInfo.lastModified().toString("yyyy-MM-dd"));
    lblFilePath->setText(fileInfo.absolutePath());

    // 태그(확장자) 표시 업뎃
    // 기존 태그 삭제
    QLayoutItem* child;
    while ((child = tagLayout->takeAt(0)) != nullptr) {
        if (child->widget()) delete child->widget();
        delete child;
    }

    // 새 태그 추가
    // 파일 확장자 추출
    QString ext = fileInfo.suffix().toLower();
    if (!ext.isEmpty()) {
        QLabel* tagChip = new QLabel("#" + ext);

        // 스타일 시트
        tagChip->setStyleSheet(
            "QLabel {"
            "  background-color: #FFD864; color: black; border: 1px solid #dadce0;"
            "  border-radius: 12px; padding: 4px 12px; font-size: 11px; font-weight: 500;"
            "}"
        );
        tagLayout->addWidget(tagChip);
    }
    tagLayout->addStretch();

    // 하이라이팅 및 내용 로드
    highlighter->setLanguage(fileInfo.suffix().toLower());

    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        in.setEncoding(QStringConverter::Utf8);

        codePreview->setPlainText(in.readAll());
        codePreview->moveCursor(QTextCursor::Start);

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

void CodeShelf::setupSearchUI() {
    searchFilterCombo = new QComboBox();
    searchFilterCombo->addItem("제목", "title");
    searchFilterCombo->addItem("제목 + 내용", "all");
    searchFilterCombo->setStyleSheet("padding:5px; background-color: #3c3c3c; color: white; ");

    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("검색어를 입력하세요...");
    searchEdit->setClearButtonEnabled(true);    // 우측에 x버튼
    searchEdit->setStyleSheet("padding:5px; color:white;");

    // 검색 버튼
    QPushButton* searchBtn = new QPushButton("검색");
    searchBtn->setStyleSheet("padding:6px");

    // 레이아웃 배치
    searchLayout = new QHBoxLayout();
    searchLayout->addWidget(searchFilterCombo);
    searchLayout->addWidget(searchEdit);

    // 콤보박스 바꿀 때 실시간 검색
    connect(searchFilterCombo, &QComboBox::currentIndexChanged, this, [=]() {
        onSearchTextChanged(searchEdit->text());
        });

    // 텍스트 칠 때 실시간 검색
    connect(searchEdit, &QLineEdit::textChanged, this, &CodeShelf::onSearchTextChanged);

    // 엔터키나 버튼 눌렀을 때도 검색 실행
    connect(searchEdit, &QLineEdit::returnPressed, this, [=]() {
        onSearchTextChanged(searchEdit->text());
    });
    connect(searchBtn, &QPushButton::clicked, this, [=]() {
        filterBySearch (currentSelectedExt, searchEdit->text(), currentSearch.mode);
    });
}
void CodeShelf::onCopyClicked() {
    QString text = codePreview->toPlainText();
    if (text.isEmpty()) {
        return;
    }

    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(text);

    statusBar()->showMessage("클립보드에 복사되었습니다",2000);

}
void CodeShelf::onOpenDirClicked() {
    QFileInfo fileInfo(currentFilePath);

    if (currentFilePath.isEmpty() || !QFile::exists(currentFilePath)) {
        QMessageBox::warning(this, "알림", "파일 경로가 유효하지 않습니다");
        return;
    }
    QString nativePath = QDir::toNativeSeparators(fileInfo.absoluteFilePath());
    QStringList args;
    args << "/select," << nativePath;
    QProcess::startDetached("explorer.exe", args);
}

CodeShelf::~CodeShelf()
{}