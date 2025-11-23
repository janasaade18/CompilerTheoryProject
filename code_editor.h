#ifndef CODE_EDITOR_H
#define CODE_EDITOR_H

#include <QSyntaxHighlighter>
#include <QTextEdit>
#include <QRegularExpression>
#include <QToolTip>
#include <QMouseEvent>

// --- STRUCTURE TO HOLD ERROR INFO ---
struct ErrorInfo {
    int line;      // 0-indexed line number
    QString token; // The text causing error (e.g. "prin")
    QString message; // "Variable undefined"
};

// --- SYNTAX HIGHLIGHTER CLASS ---
class PyHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit PyHighlighter(QTextDocument *parent = nullptr) : QSyntaxHighlighter(parent) {
        // Define Keywords Rule
        HighlightingRule rule;
        keywordFormat.setForeground(QColor("#cc7832")); // Orange/Gold
        keywordFormat.setFontWeight(QFont::Bold);
        QStringList keywordPatterns = {
            "\\bdef\\b", "\\bif\\b", "\\belse\\b", "\\belif\\b", "\\bwhile\\b", "\\bfor\\b",
            "\\bin\\b", "\\breturn\\b", "\\bprint\\b", "\\btry\\b", "\\bexcept\\b", "\\band\\b",
            "\\bor\\b", "\\bnot\\b", "\\bTrue\\b", "\\bFalse\\b", "\\bNone\\b"
        };
        for (const QString &pattern : keywordPatterns) {
            rule.pattern = QRegularExpression(pattern);
            rule.format = keywordFormat;
            highlightingRules.append(rule);
        }

        // Define String Rule
        stringFormat.setForeground(QColor("#6a8759")); // Green
        rule.pattern = QRegularExpression("\".*\"");
        rule.format = stringFormat;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\'.*\'");
        highlightingRules.append(rule);

        // Define Function Name Rule (e.g. "calculate" in "def calculate")
        funcFormat.setForeground(QColor("#ffc66d")); // Light Yellow
        rule.pattern = QRegularExpression("\\bdef\\s+([A-Za-z_][A-Za-z0-9_]*)");
        rule.format = funcFormat;
        // Note: QRegularExpression matching for capturing groups in highlighter is tricky,
        // simple highlighters usually just match the whole pattern.
        // We stick to simple matching for stability here.
    }

    // Call this from MainWindow when Semantic Analysis fails
    void setError(const ErrorInfo& err) {
        currentError = err;
        rehighlight(); // Triggers highlightBlock immediately
    }

    void clearError() {
        currentError.line = -1;
        rehighlight();
    }

protected:
    void highlightBlock(const QString &text) override {
        // 1. Apply Standard Syntax Highlighting
        // FIX: Use index-based loop instead of range-based to prevent SIGSEGV
        for (int i = 0; i < highlightingRules.size(); ++i) {
            const HighlightingRule &rule = highlightingRules[i];
            QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
            while (matchIterator.hasNext()) {
                QRegularExpressionMatch match = matchIterator.next();
                setFormat(match.capturedStart(), match.capturedLength(), rule.format);
            }
        }

        // 2. Apply Red Squiggly Line if Error Exists on this line
        int currentLineNum = currentBlock().blockNumber();
        if (currentLineNum == currentError.line && !currentError.token.isEmpty()) {
            int index = text.indexOf(currentError.token);
            if (index != -1) {
                QTextCharFormat errorFormat;
                errorFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
                errorFormat.setUnderlineColor(Qt::red);
                errorFormat.setForeground(Qt::red);
                setFormat(index, currentError.token.length(), errorFormat);
            }
        }
    }

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QTextCharFormat keywordFormat;
    QTextCharFormat stringFormat;
    QTextCharFormat funcFormat;

    ErrorInfo currentError = {-1, "", ""};
};


// --- CUSTOM TEXT EDITOR FOR HOVERING ---
class CodeEditor : public QTextEdit {
    Q_OBJECT
public:
    explicit CodeEditor(QWidget *parent = nullptr) : QTextEdit(parent) {
        setMouseTracking(true); // Enable mouse move events without clicking
    }

    void setCurrentError(const ErrorInfo& err) {
        activeError = err;
    }

protected:
    void mouseMoveEvent(QMouseEvent *e) override {
        QTextEdit::mouseMoveEvent(e);

        if (activeError.line == -1) {
            return;
        }

        // Get cursor at mouse position
        QTextCursor cursor = cursorForPosition(e->pos());
        int blockNumber = cursor.block().blockNumber();

        // Check if mouse is over the error line
        if (blockNumber == activeError.line) {
            QString blockText = cursor.block().text();
            int column = cursor.positionInBlock();

            int errorStart = blockText.indexOf(activeError.token);
            int errorEnd = errorStart + activeError.token.length();

            // Check if mouse is horizontally over the token
            if (errorStart != -1 && column >= errorStart && column <= errorEnd) {
                QToolTip::showText(e->globalPosition().toPoint(), activeError.message, this);
            } else {
                QToolTip::hideText();
            }
        } else {
            QToolTip::hideText();
        }
    }

private:
    ErrorInfo activeError = {-1, "", ""};
};

#endif // CODE_EDITOR_H
