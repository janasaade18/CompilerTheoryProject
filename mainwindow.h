#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QPointF>
#include <cmath>
#include "parser.h"

class ASTNode;

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

private:
    Ui::MainWindow *ui;
    QGraphicsScene *automatonScene;
    QGraphicsScene *treeScene;

    void setupUI();
    void drawParseTree(const ASTNode* node, QPointF pos, QPointF parentPos = QPointF(), int depth = 0);
    void drawTrueAutomaton(const Parser& parser);
    QString getStateName(ParserState state);
    QString getTokenName(TokenType token);
    void addAutomatonLegend();
    void addStateCategories();
};

#endif // MAINWINDOW_H
