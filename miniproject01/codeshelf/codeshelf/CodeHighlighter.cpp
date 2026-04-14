#include "CodeHighlighter.h"

CodeHighlighter::CodeHighlighter(QTextDocument* parent) :QSyntaxHighlighter(parent) {
	// 서식 미리 정의
	keywordFormat.setForeground(QColor("#569cd6"));
	keywordFormat.setFontWeight(QFont::Bold);

	functionFormat.setForeground(QColor("#dcdcaa"));
	commentFormat.setForeground(QColor("#6a9955"));
	quotationFormat.setForeground(QColor("#ce9178"));
	numberFormat.setForeground(QColor("#b5cea8"));

	// 규칙 등록
	initLanguageMaps();
}

void CodeHighlighter::addRule(const QString& pattern, const QTextCharFormat& format) {
	HighlightingRule rule;
	rule.pattern = QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
	rule.format = format;
	highlightingRules.append(rule);
}

void CodeHighlighter::addKeywords(const QStringList& keywords, const QTextCharFormat& format) {
	for (const QString& keyword : keywords) {
		// 단어 경계(\\b)를 자동으로 붙여 정확한 단어 매칭
		addRule("\\b" + keyword + "\\b", format);
	}
}

void CodeHighlighter::highlightBlock(const QString& text) {
	for (const HighlightingRule& rule : highlightingRules) {
		QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
		while (matchIterator.hasNext()) {
			QRegularExpressionMatch match = matchIterator.next();
			setFormat(match.capturedStart(), match.capturedLength(), rule.format);
		}
	}

	if (commentStartExpression.isEmpty()) return;

	setCurrentBlockState(0);

	int startIndex = 0;
	if (previousBlockState() !=1) {
		startIndex = text.indexOf(commentStartExpression);
	}

	while (startIndex>=0) {
		// 끝나는 기호 찾기
		int endIndex = text.indexOf(commentEndExpression, startIndex + commentStartExpression.length());
		int commentLength;

		if (endIndex==-1) {
			setCurrentBlockState(1);
			commentLength = text.length() - startIndex;
		}
		else {
			commentLength = endIndex - startIndex + commentEndExpression.length();
		}

		setFormat(startIndex, commentLength, commentFormat);
		startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
	}
}

void CodeHighlighter::initLanguageMaps() {
	// C++ 키워드
	languageMaps["cpp"] << "int" << "char" << "void" << "class" << "public" << "private"
						<< "protected" << "virtual" << "return" << "if" << "else" << "for"
						<< "while" << "do" << "switch" << "case" << "break" << "continue"
						<< "static" << "const" << "struct" << "enum" << "typedef" << "new"
						<< "delete" << "friend" << "inline" << "namespace" << "using"
						<< "template" << "typename" << "this" << "try" << "catch" << "throw"
						<< "bool" << "double" << "float" << "long" << "short" << "signed"
						<< "unsigned" << "nullptr" << "override" << "final";

	// Python 키워드
	languageMaps["sql"]  << "SELECT" << "INSERT" << "UPDATE" << "DELETE" << "FROM" << "WHERE"
						<< "JOIN" << "GROUP" << "BY" << "ORDER" << "LIMIT" << "HAVING"
						<< "AND" << "OR" << "NOT" << "IN" << "IS" << "NULL" << "CREATE"
						<< "TABLE" << "DROP" << "ALTER" << "INTO" << "VALUES" << "DISTINCT";

	// SQL 키워드
	languageMaps["py"] << "def" << "class" << "import" << "from" << "if" << "elif" << "else"
						<< "return" << "print" << "in" << "as" << "for" << "while" << "try"
						<< "except" << "finally" << "with" << "lambda" << "yield" << "pass"
						<< "break" << "continue" << "True" << "False" << "None" << "and" << "or" << "not";
}

// 언어 전환
void CodeHighlighter::setLanguage(const QString& extension) {
	highlightingRules.clear();


	QString lang = extension.toLower();

	if (lang == "cpp" || lang == "h" || lang=="sql" || lang=="c") {
		commentStartExpression = "/*";
		commentEndExpression = "*/";
	}
	// python은 """ 3개
	else if (lang == "py") {
		commentStartExpression = "\"\"\"";
		commentEndExpression = "\"\"\"";
	}
	else {
		commentStartExpression = "";
		commentEndExpression = "";
	}

	if (lang == "h" || lang == "hpp" || lang == "c") {
		lang = "cpp";
	}

	if (languageMaps.contains(lang)) {
		addKeywords(languageMaps[lang], keywordFormat);
	}
	setupcommonRules();

	rehighlight();
}

void CodeHighlighter::setupcommonRules() {
	QList<QPair<QString, QTextCharFormat>> commonPatterns;
	addRule("'.*'", quotationFormat);
	addRule("\".*\"", quotationFormat);
	addRule("\\b[0-9]+\\b", numberFormat);

	QTextCharFormat preprocessorFormat;
	preprocessorFormat.setForeground(QColor("#c586c0"));
	addRule("^\\s*#.*", preprocessorFormat);
	addRule("//[^\n]*", commentFormat);

	addRule("\\b[A-Za-z0-9_]+(?=\\s*\\()", functionFormat);
}