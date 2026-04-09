#pragma once
#include <QMainWindow>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QTreeWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QStackedWidget>
#include <QSqlError>
#include <QDebug>
#include <QSqlDatabase>
#include "ui_codeshelf.h"
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QSqlQuery>
#include <QDateTime>   // 시간 표시용
#include <QMessageBox> // 알림용

class CodeShelf : public QMainWindow
{
    Q_OBJECT

public:
    CodeShelf(QWidget *parent = nullptr);
    ~CodeShelf();

private slots:
    /* [1] 화면전환 및 기본 액션 */
    void toggleSearchMode();    // 검색 모드 전환 슬롯
    void showHome();             // 홈 화면 전환 슬롯
    void onSelectRootFolder();  // 슬롯 함수
    void onTreeItemClicked(QTreeWidgetItem* item, int column);



protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void initDatabase();
    void setupTopBar();
    void selectRootDirectory();
    void scanDirectory(const QString& path);
    void setupDashboard();
    void setupManagementPage();
    void addItem(QVBoxLayout* targetLayout, QString name, QString date, QString tag);
    // UI 구성요소
    /* 모든 UI요소가 담기는 그릇 */
    QWidget* centerWidget;
    QVBoxLayout* mainLayout;
    /* 홈, 관리화면 갈아끼우기 */
    QStackedWidget* mainStackedWidget;


    // 상단바 위젯
    QWidget* topBar;
    
    QPushButton* btnSelectRoot = new QPushButton;
    QPushButton* btnHome;
    QPushButton* btnSearchToggle;

    QString currentRootPath;
    int currentRootId;
    // 재귀적으로 폴더 훑기
    bool scanDirRecursive(const QString& path, QTreeWidgetItem* parentItem, const QDir& rootDir);  // 기준 루트폴더);
    // insert데이터 저장
    bool insertFileRecord(const QFileInfo& info, const QString relPath);
    QFileInfo rootInfo;

    // 3분할 영역
    /* 3분할 크기 조절용 경계선 */
    QSplitter* mainSplitter;
    /* 카테고리 목록 */
    QTreeWidget* categoryTree;
    /* 중앙 코드제목 리스트 */
    QTableWidget* snipperList;

    QLabel* lblPName;
    QLabel* lblComment;
    /* 실제 코드 창 */
    QTextEdit* codePreview;
};

