#ifndef CODEHIGHLIGHTER_H
#define CODEHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>

class CodeHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit CodeHighlighter(QTextDocument* parent = nullptr);
    void setLanguage(const QString&);

protected:
    // 이 함수가 줄 단위로 텍스트를 검사해서 색을 입힙니다.
    void highlightBlock(const QString& text) override;

private:
    QString commentStartExpression;
    QString commentEndExpression;
    QMap<QString, QStringList>languageMaps;
    void initLanguageMaps();
    void setupcommonRules();
    void addRule(const QString& pattern, const QTextCharFormat& format);
    void addKeywords(const QStringList& keywords, const QTextCharFormat& format);
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    // 자주 쓰이는 서식들
    QTextCharFormat keywordFormat;
    QTextCharFormat functionFormat;
    QTextCharFormat classFormat;
    QTextCharFormat commentFormat;
    QTextCharFormat quotationFormat;
    QTextCharFormat numberFormat;
};

#endif