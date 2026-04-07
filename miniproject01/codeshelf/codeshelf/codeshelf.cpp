#include "CodeShelf.h"

CodeShelf::CodeShelf(QWidget *parent)
    : QMainWindow(parent)
{
    /* [1] 전체 베이스 초기설정 */
    centerWidget = new QWidget(this);
    mainLayout = new QVBoxLayout(centerWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0); // 상하좌우 마진제거
    mainLayout->setSpacing(0);  // 위젯 간의 간격을 0으로
    setCentralWidget(centerWidget);

    /* [2] 상단바 영역 */
    setupTopBar();

    /* [3] 메인 콘텐츠 영역(Stacked Widget )*/     // 배고프다얼ㄹ얼어어엉어엉
    mainStackedWidget = new QStackedWidget();
    mainLayout->addWidget(mainStackedWidget);

    // [page 0] 홈 대시보드(임시로)~~
    setupDashboard();

    // [page 1]3분할 설정
    setupManagementPage();

    // [4] 스플리터 초기 비율 설정 (2:3:5)
    mainSplitter->setStretchFactor(0, 2);
    mainSplitter->setStretchFactor(1, 3);
    mainSplitter->setStretchFactor(2, 5);

    mainStackedWidget->addWidget(mainSplitter);

    // [5] 시그널/슬롯 연결
    connect(btnHome, &QPushButton::clicked, this, &CodeShelf::showHome);
    connect(btnSearchToggle, &QPushButton::clicked, this, &CodeShelf::toggleSearchMode);

    resize(1200, 800);
}

/* [2] 상단바 영역 */
void CodeShelf::setupTopBar() {
    topBar = new QWidget();

    topBar->setFixedHeight(60); // 최대높이 60고정
    topBar->setStyleSheet("background-color: #333; color:white;");  // 배경색 지정
    QHBoxLayout* topLayout = new QHBoxLayout(topBar);

    QLabel* logo = new QLabel("CodeShelf");
    logo->setStyleSheet("font-weight: bold; font-size: 18px; margin-left:10px;");

    btnHome = new QPushButton("홈");
    btnSearchToggle = new QPushButton("검색/관리");

    QLabel* userInfo = new QLabel("user 1234 👤");

    topLayout->addWidget(logo);
    topLayout->addStretch();    // 중간 여백
    topLayout->addWidget(btnHome);
    topLayout->addWidget(btnSearchToggle);
    topLayout->addStretch();    // 우측 여백
    topLayout->addWidget(userInfo);

    mainLayout->addWidget(topBar);
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

    // left : 카테고리 트리
    categoryTree = new QTreeWidget();
    categoryTree->setHeaderLabel("Categories");

    // center : 리스트, 검색창
    QWidget* centerWidget = new QWidget();
    QVBoxLayout* centerLayout = new QVBoxLayout(centerWidget);
    centerLayout->setContentsMargins(10, 10, 10, 10); // 전체 여백 추가
    centerLayout->setSpacing(10);   // 아이템들 사이 간격 10px고정

    // 검색 창 설정
    QLineEdit* searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("겁색어를 입력하세요...");
    centerLayout->addWidget(searchEdit);    // 검색창 맨 위 고정

    // 중간에 빈공간 하나
    centerLayout->addStretch(1);

    addItem(centerLayout, "Login_Module.cpp", "2026-04-06", "#Network");
    addItem(centerLayout, "Database_Connect.sql", "2026-04-05", "#SQL");


    // right : 미리보기 및 버튼
    QWidget* rightWidget = new QWidget();
    QVBoxLayout* rightMainLayout = new QVBoxLayout(rightWidget);

    // right-T 위쪽 위젯 (정보 영역)
    QWidget* infoWidget = new QWidget();
    QVBoxLayout* infoLayout = new QVBoxLayout(infoWidget);  // 정보들도 위아래로 쌓을꺼니깐
    infoLayout->setContentsMargins(0, 0, 0, 5); // 아래쪽 마진주기
    infoWidget->setStyleSheet("background-color:white");

    lblPName = new QLabel("Project : Code-1231");
    lblPName->setStyleSheet("font-size:18px; fonr-weight:bold");

    lblComment = new QLabel("// 파일입출력 기본예제");
    lblComment->setStyleSheet("color:gray");

    // 태그들을 위한 공간(가로로나열)
    QWidget* tagBar = new QWidget();
    QHBoxLayout* tagLayout = new QHBoxLayout(tagBar);
    tagLayout->setContentsMargins(0, 0, 0, 0);

    infoLayout->addWidget(lblPName);
    infoLayout->addWidget(lblComment);
    infoLayout->addWidget(tagBar);


    // right-B 아래쪽 위젯(코드 영역)
    codePreview = new QTextEdit();
    codePreview->setReadOnly(true); // 수정불가하게
    QPushButton* btnCopy = new QPushButton("클립보드 복사");

    // right 합체
    rightMainLayout->addWidget(infoWidget);
    rightMainLayout->addWidget(codePreview);
    rightMainLayout->addWidget(btnCopy);

    // 코드창과 정보창 크기 비율 조정
    rightMainLayout->setStretchFactor(infoWidget, 2);
    rightMainLayout->setStretchFactor(codePreview, 8);


    mainSplitter->addWidget(categoryTree);
    mainSplitter->addWidget(centerWidget);
    mainSplitter->addWidget(rightWidget);

    mainStackedWidget->addWidget(mainSplitter);

    mainStackedWidget->setCurrentIndex(0);
}
// 아이템 추가
void CodeShelf::addItem(QVBoxLayout* targetLayout, QString name, QString date, QString tag) {
    // 2. 아이템용 Widget과 Layout 만들기
    QWidget* itemWidget = new QWidget();
    itemWidget->setStyleSheet("background-color: white; border: 1px solid #ddd; border-radius: 5px;"); // 테두리 추가
    QVBoxLayout* cItemLayout = new QVBoxLayout(itemWidget);

    // 3. 위쪽 줄 구성
    QHBoxLayout* ctopLayout = new QHBoxLayout();
    QLabel* cIconLabel = new QLabel("📄");   // 나중에 교체할 것
    QLabel* fileName = new QLabel(name);
    fileName->setStyleSheet("font-weight:bold");

    QLabel* dateLabel = new QLabel(date);
    dateLabel->setStyleSheet("color:gray; font-size:11px;");

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

    // 5. 아이템 레이아웃 합체
    cItemLayout->addLayout(ctopLayout);
    cItemLayout->addLayout(ctagLayout);

    // 위젯 감시 및 데이터 심기
    itemWidget->installEventFilter(this);
    itemWidget->setProperty("fileName", name);
    itemWidget->setProperty("fileDate", date);
    itemWidget->setCursor(Qt::PointingHandCursor);      // 마우스를 올리면 손가락 모양
    
    // 완성된 위젯을 전체 센터 레이아웃에 추가
    targetLayout->insertWidget(targetLayout->count() - 1, itemWidget);
}

// 클릭 감지(센터 아이템)
bool CodeShelf::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::MouseButtonPress) {
        QWidget* clickedWidget = qobject_cast<QWidget*>(obj);

        // 우리가 데이터를 심어놓은 그 위젯인지 확인하기
        if (clickedWidget && clickedWidget->property("fileName").isValid()) {
            QString name = clickedWidget->property("fileName").toString();
            QString date = clickedWidget->property("fileDate").toString();

            // 우측 레이아웃 업데이트
            lblPName->setText("Project : " + name);
            lblComment->setText("// Last Modified : " + date);
            codePreview->setPlainText(name + "의 상세 코드입니다.");

            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void CodeShelf::showHome() {
}

void CodeShelf::toggleSearchMode() {
    mainStackedWidget->setCurrentIndex(1);
}

CodeShelf::~CodeShelf()
{}