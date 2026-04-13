#include "CodeHighlighter.h"

CodeHighlighter::CodeHighlighter(QTextDocument* parent) :QSyntaxHighlighter(parent) {
	HighlightingRule rule;

	keywordFormat.setForeground(QColor("#569cd6"));
	keywordFormat.setFontWeight(QFont::Bold);
	QStringList keywordPatterns = {
		"\\bchar\\b", "\\bclass\\b", "\\bconst\\b", "\\bdouble\\b", "\\benum\\b",
		"\\bexplicit\\b", "\\bfriend\\b", "\\binline\\b", "\\bint\\b", "\\blong\\b",
		"\\bnamespace\\b", "\\boperator\\b", "\\bprivate\\b", "\\bprotected\\b",
		"\\bpublic\\b", "\\bshort\\b", "\\bsignals\\b", "\\bslots\\b", "\\bstatic\\b",
		"\\bstruct\\b", "\\btemplate\\b", "\\btypedef\\b", "\\btypename\\b",
		"\\bunion\\b", "\\bunsigned\\b", "\\bvirtual\\b", "\\bvoid\\b", "\\bvolatile\\b",
		"\\bif\\b", "\\belse\\b", "\\bfor\\b", "\\bwhile\\b", "\\breturn\\b"
	};

	for (const QString& pattern : keywordPatterns) {
		rule.pattern = QRegularExpression(pattern);
		rule.format = keywordFormat;
		highlightingRules.append(rule);
	}

	// 한줄 주석 (초록)
	commentFormat.setForeground(QColor("#6a9955"));
	rule.pattern = QRegularExpression("//[^\n]*");
	rule.format = commentFormat;
	highlightingRules.append(rule);

	// 문자열 (오렌지)
	quotationFormat.setForeground(QColor("#ce9178"));
	rule.pattern = QRegularExpression("\".*\"");
	rule.format = quotationFormat;
	highlightingRules.append(rule);
};

void CodeHighlighter::highlightBlock(const QString& text) {
	for (const HighlightingRule& rule : highlightingRules) {
		QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
		while (matchIterator.hasNext()) {
			QRegularExpressionMatch match = matchIterator.next();
			setFormat(match.capturedStart(), match.capturedLength(), rule.format);
		}
	}
}