#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QGraphicsScene>
#include <QPointF>
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
    void drawAutomatonFromAst(const ASTNode* node, QPointF pos, QPointF parentPos = QPointF());
    void drawParseTree(const ASTNode* node, QPointF pos, QPointF parentPos = QPointF(), int depth = 0);  // ADD THIS LINE
};
#endif
