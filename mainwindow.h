#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QPointF> // Required for QPointF in function declaration

// Forward declaration of the base ASTNode class
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
    // This is the only slot we need now
    void onAnalyzeClicked();

private:
    Ui::MainWindow *ui;
    QGraphicsScene *automatonScene;
    QGraphicsScene *treeScene;

    // UI and drawing helper functions
    void setupUI();
    void drawSampleAutomaton();
    void drawAst(const ASTNode* node, QPointF pos, QPointF parentPos = QPointF());
};

#endif // MAINWINDOW_H
