#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QTextEdit>
#include <QProcess>
#include <QMap>
#include "parser.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onAnalyzeClicked();

    // Profiler Slots (Compiling and Running the C++ output)
    void onCompilationFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onExecutionFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    Ui::MainWindow *ui;

    // Visualization Scenes
    QGraphicsScene *automatonScene;
    QGraphicsScene *treeScene;

    // UI Elements (Tabs)
    QTextEdit *sourceCodeEdit;
    QTextEdit *targetCodeEdit;
    QTextEdit *tokensEdit;
    QTextEdit *designEdit;      // The "Formal Design" Tab
    QTextEdit *profilerEdit;    // The "Profiler" Tab

    // Process for Profiler
    QProcess *compilerProcess;
    QProcess *runnerProcess;

    // Helper Functions
    void setupUI();
    // Removed the function declaration causing ambiguity, implemented as static/global in cpp
    void drawParseTree(const ASTNode* node, QPointF pos, QPointF parentPos = QPointF(), int depth = 0);
    void drawTrueAutomaton(const Parser& parser);

    // Helpers
    QString getStateName(ParserState state);
    QString getTokenName(TokenType token);
};

#endif // MAINWINDOW_H
