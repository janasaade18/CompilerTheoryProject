#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QPointF> // Required for QPointF in function declaration

// Forward declare ASTNode to avoid circular dependencies if needed,
// but including the header is better here.
#include "ast.h"

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
    void onOpenFileClicked();
    void onSaveFileClicked();

private:
    Ui::MainWindow *ui;
    QGraphicsScene *automatonScene;
    QGraphicsScene *treeScene;

    void setupUI();
    void drawSampleAutomaton();

    // Replaced the old sample function with the one that draws a real AST
    void drawAst(const ASTNode* node, QPointF pos, QPointF parentPos = QPointF());
};

#endif // MAINWINDOW_H
