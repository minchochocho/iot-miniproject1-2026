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
#include "ui_codeshelf.h"

class CodeShelf : public QMainWindow
{
    Q_OBJECT

public:
    CodeShelf(QWidget *parent = nullptr);
    ~CodeShelf();

private slots:
    void toggleSearchMode();    // 검색 모드 전환 슬롯
    void showHome();             // 홈 화면 전환 슬롯

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void setupTopBar();
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
    QPushButton* btnHome;
    QPushButton* btnSearchToggle;

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

