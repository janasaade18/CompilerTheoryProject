#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QTextEdit>
#include <QProcess>
#include <QMap>
#include <QSyntaxHighlighter>
#include <QToolTip>
#include <QTimer>
#include "parser.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// --- ERROR HIGHLIGHTER CLASS ---
// Draws red waves under lines containing errors
class ErrorHighlighter : public QSyntaxHighlighter {
public:
    ErrorHighlighter(QTextDocument *parent = nullptr) : QSyntaxHighlighter(parent) {}

    void setErrorLine(int line) {
        m_errorLine = line;
        rehighlight();
    }

    void clearError() {
        m_errorLine = -1;
        rehighlight();
    }

protected:
    void highlightBlock(const QString &text) override {
        int currentLine = currentBlock().blockNumber() + 1;
        if (currentLine == m_errorLine) {
            QTextCharFormat fmt;
            fmt.setUnderlineColor(Qt::red);
            fmt.setUnderlineStyle(QTextCharFormat::WaveUnderline);
            setFormat(0, text.length(), fmt);
        }
    }

private:
    int m_errorLine = -1;
};

// --- CUSTOM CODE EDITOR CLASS ---
// Handles Mouse Move Events to show Error Tooltips
class CodeEditor : public QTextEdit {
    Q_OBJECT
public:
    explicit CodeEditor(QWidget *parent = nullptr) : QTextEdit(parent) {
        setMouseTracking(true); // Enable mouse tracking for hover
    }

    void setError(int line, const QString &msg) {
        m_errorLine = line;
        m_errorMessage = msg;
    }

    void clearError() {
        m_errorLine = -1;
        m_errorMessage.clear();
    }

protected:
    void mouseMoveEvent(QMouseEvent *e) override {
        QTextEdit::mouseMoveEvent(e);

        if (m_errorLine == -1) return;

        // Map mouse position to text cursor to get the line number
        QTextCursor cursor = cursorForPosition(e->pos());
        int line = cursor.blockNumber() + 1;

        if (line == m_errorLine) {
            QToolTip::showText(e->globalPos(), m_errorMessage, this);
        } else {
            QToolTip::hideText();
        }
    }

private:
    int m_errorLine = -1;
    QString m_errorMessage;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onAnalyzeClicked();
    void liveCheck(); // New Slot for Live Detection

    // Profiler Slots (Compiling and Running the C++ output)
    void onCompilationFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onExecutionFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    Ui::MainWindow *ui;

    // Visualization Scenes
    QGraphicsScene *automatonScene;
    QGraphicsScene *treeScene;

    // UI Elements (Tabs)
    CodeEditor *sourceCodeEdit; // CHANGED to custom editor
    QTextEdit *targetCodeEdit;
    QTextEdit *tokensEdit;
    QTextEdit *designEdit;      // The "Formal Design" Tab
    QTextEdit *profilerEdit;    // The "Profiler" Tab

    // Error Highlighting
    ErrorHighlighter *highlighter;
    QTimer *liveCheckTimer;

    // Process for Profiler
    QProcess *compilerProcess;
    QProcess *runnerProcess;

    // Helper Functions
    void setupUI();
    void drawParseTree(const ASTNode* node, QPointF pos, QPointF parentPos = QPointF(), int depth = 0);
    void drawTrueAutomaton(const Parser& parser);

    // Helpers
    QString getStateName(ParserState state);
    QString getTokenName(TokenType token);
};

#endif // MAINWINDOW_H
