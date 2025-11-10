#include "mainwindow.h"
#include "ui_mainwindow.h"

// Core compiler components
#include "lexer.h"
#include "parser.h"
#include "translator.h"

// C++ standard library for error handling
#include <stdexcept>

// Necessary Qt headers for the UI implementation
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QGraphicsView>
#include <QTextEdit>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QMessageBox>
#include <QPainter>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupUI();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete automatonScene;
    delete treeScene;
}

void MainWindow::setupUI()
{
    // --- Define the red color palette ---
    const QString COLOR_BACKGROUND_DARK = "#2c1d1d";
    const QString COLOR_BACKGROUND_MID = "#4c2a2a";
    const QString COLOR_BORDER = "#6c3a3a";
    const QString COLOR_TEXT_PRIMARY = "#f7fafc";
    const QString COLOR_TEXT_SECONDARY = "#a09393";
    const QString COLOR_ACCENT_RED = "#e53e3e";
    const QString COLOR_ACCENT_GREEN = "#48bb78";

    // Set window properties
    setWindowTitle("Compiler"); // Simplified window title
    resize(1400, 850);
    setStyleSheet(QString("background-color: %1;").arg(COLOR_BACKGROUND_DARK));

    // Create central widget and set it
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // --- HEADER WIDGET HAS BEEN REMOVED ---

    // --- Top row for code editors ---
    QWidget *topRowWidget = new QWidget();
    QHBoxLayout *topRowLayout = new QHBoxLayout(topRowWidget);
    topRowLayout->setSpacing(0);
    topRowLayout->setContentsMargins(0, 0, 0, 0);

    // SOURCE CODE PANEL
    QWidget *sourcePanel = new QWidget();
    sourcePanel->setStyleSheet(QString("border-right: 1px solid %1;").arg(COLOR_BORDER));
    QVBoxLayout *sourceLayout = new QVBoxLayout(sourcePanel);
    sourceLayout->setSpacing(0);
    sourceLayout->setContentsMargins(0, 0, 0, 0);
    QLabel *sourceLabel = new QLabel("Source Code Input");
    sourceLabel->setStyleSheet(QString("background-color: %1; color: %2; padding: 10px; font-weight: bold; border-top: 1px solid %3; border-bottom: 1px solid %3;").arg(COLOR_BACKGROUND_MID, COLOR_TEXT_PRIMARY, COLOR_BORDER));
    QTextEdit *sourceCodeEdit = new QTextEdit();
    sourceCodeEdit->setObjectName("sourceCodeEdit");
    sourceCodeEdit->setStyleSheet(QString("background-color: %1; color: %2; font-family: 'Courier New'; font-size: 13px; border: none; padding: 10px;").arg(COLOR_BACKGROUND_DARK, COLOR_TEXT_PRIMARY));
    sourceCodeEdit->setPlainText("width = 100;\nheight = width * 2;\narea = (width * height) / 2;");
    sourceLayout->addWidget(sourceLabel);
    sourceLayout->addWidget(sourceCodeEdit);

    // TARGET CODE PANEL
    QWidget *targetPanel = new QWidget();
    QVBoxLayout *targetLayout = new QVBoxLayout(targetPanel);
    targetLayout->setSpacing(0);
    targetLayout->setContentsMargins(0, 0, 0, 0);
    QLabel *targetLabel = new QLabel("Translated Output (C++)");
    targetLabel->setStyleSheet(QString("background-color: %1; color: %2; padding: 10px; font-weight: bold; border-top: 1px solid %3; border-bottom: 1px solid %3;").arg(COLOR_BACKGROUND_MID, COLOR_TEXT_PRIMARY, COLOR_BORDER));
    QTextEdit *targetCodeEdit = new QTextEdit();
    targetCodeEdit->setObjectName("targetCodeEdit");
    targetCodeEdit->setStyleSheet(QString("background-color: %1; color: %2; font-family: 'Courier New'; font-size: 13px; border: none; padding: 10px;").arg(COLOR_BACKGROUND_DARK, COLOR_TEXT_PRIMARY));
    targetCodeEdit->setReadOnly(true);
    targetCodeEdit->setPlainText("// Translated code will appear here...");
    targetLayout->addWidget(targetLabel);
    targetLayout->addWidget(targetCodeEdit);

    // Add code panels to the top row layout
    topRowLayout->addWidget(sourcePanel, 1);
    topRowLayout->addWidget(targetPanel, 1);

    // --- Bottom row for visualizations ---
    QTabWidget *tabWidget = new QTabWidget();
    tabWidget->setStyleSheet(QString(
                                 "QTabWidget::pane { background-color: %1; border: none; border-top: 1px solid %2; }"
                                 "QTabBar::tab { background-color: %3; color: %4; padding: 10px 20px; border: none; }"
                                 "QTabBar::tab:selected { background-color: %1; color: %5; border-top: 2px solid %5; }"
                                 "QTabBar::tab:hover { color: %6; }"
                                 ).arg(COLOR_BACKGROUND_DARK, COLOR_BORDER, COLOR_BACKGROUND_MID, COLOR_TEXT_SECONDARY, COLOR_ACCENT_RED, COLOR_TEXT_PRIMARY));

    // Automaton view
    automatonScene = new QGraphicsScene();
    QGraphicsView *automatonView = new QGraphicsView(automatonScene);
    automatonView->setStyleSheet(QString("background-color: %1; border: none;").arg(COLOR_BACKGROUND_DARK));
    automatonView->setRenderHint(QPainter::Antialiasing);
    drawSampleAutomaton();

    // Tokens view
    QTextEdit *tokensEdit = new QTextEdit();
    tokensEdit->setObjectName("tokensEdit");
    tokensEdit->setStyleSheet(QString("background-color: %1; color: %2; font-family: 'Courier New'; font-size: 13px; border: none; padding: 10px;").arg(COLOR_BACKGROUND_DARK, COLOR_ACCENT_GREEN));
    tokensEdit->setReadOnly(true);
    tokensEdit->setPlainText("Click 'Analyze & Translate' to see tokens...");

    // Parse tree view
    treeScene = new QGraphicsScene();
    QGraphicsView *treeView = new QGraphicsView(treeScene);
    treeView->setStyleSheet(QString("background-color: %1; border: none;").arg(COLOR_BACKGROUND_DARK));
    treeView->setRenderHint(QPainter::Antialiasing);
    QGraphicsTextItem *description = treeScene->addText("Parse Tree will be generated here after analysis.");
    description->setDefaultTextColor(QColor(COLOR_TEXT_PRIMARY));
    description->setFont(QFont("Arial", 12));
    description->setPos(150, 150);

    // Add views to the tab widget
    tabWidget->addTab(automatonView, "Automaton");
    tabWidget->addTab(tokensEdit, "Tokens");
    tabWidget->addTab(treeView, "Parse Tree");

    // --- ASSEMBLE THE MAIN LAYOUT ---
    mainLayout->addWidget(topRowWidget, 1); // Top section gets a stretch factor of 1
    mainLayout->addWidget(tabWidget, 1);    // Bottom section gets a stretch factor of 1

    // --- BOTTOM TOOLBAR (with centered button) ---
    QWidget *toolbarWidget = new QWidget();
    toolbarWidget->setStyleSheet(QString("padding: 10px; border-top: 1px solid %1;").arg(COLOR_BORDER));
    QHBoxLayout *toolbarLayout = new QHBoxLayout(toolbarWidget);
    QPushButton *analyzeBtn = new QPushButton("Analyze & Translate");
    analyzeBtn->setStyleSheet(QString(
                                  "QPushButton { background-color: %1; color: %2; padding: 10px 20px; border-radius: 5px; font-weight: bold; }"
                                  "QPushButton:hover { background-color: #c53030; }"
                                  ).arg(COLOR_ACCENT_RED, COLOR_TEXT_PRIMARY));
    analyzeBtn->setCursor(Qt::PointingHandCursor);
    connect(analyzeBtn, &QPushButton::clicked, this, &MainWindow::onAnalyzeClicked);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(analyzeBtn);
    toolbarLayout->addStretch();
    mainLayout->addWidget(toolbarWidget);

    // Status bar
    QLabel *statusLabel = new QLabel("Ready");
    statusLabel->setObjectName("statusLabel");
    statusLabel->setStyleSheet(QString("background-color: %1; color: %2; padding: 8px; font-size: 11px;").arg(COLOR_BACKGROUND_MID, COLOR_TEXT_SECONDARY));
    mainLayout->addWidget(statusLabel);
}

void MainWindow::drawSampleAutomaton()
{
    // ... This function's implementation remains unchanged ...
    automatonScene->clear();
    const QColor COLOR_ACCENT_RED("#e53e3e");
    const QColor COLOR_BACKGROUND_DARK("#2c1d1d");
    const QColor COLOR_TEXT_PRIMARY("#f7fafc");
    const QColor COLOR_TEXT_SECONDARY("#a09393");

    QPen pen(COLOR_ACCENT_RED, 2);
    QBrush brush(COLOR_BACKGROUND_DARK);

    QGraphicsEllipseItem *state0 = automatonScene->addEllipse(0, 0, 60, 60, pen, brush);
    state0->setPos(100, 150);
    QGraphicsTextItem *text0 = automatonScene->addText("q0");
    text0->setDefaultTextColor(COLOR_TEXT_PRIMARY);
    text0->setPos(state0->boundingRect().center().x() - text0->boundingRect().width() / 2, state0->boundingRect().center().y() - text0->boundingRect().height() / 2);
    text0->setParentItem(state0);

    QGraphicsEllipseItem *state1 = automatonScene->addEllipse(0, 0, 60, 60, pen, brush);
    state1->setPos(300, 150);
    QGraphicsTextItem *text1 = automatonScene->addText("q1");
    text1->setDefaultTextColor(COLOR_TEXT_PRIMARY);
    text1->setPos(state1->boundingRect().center().x() - text1->boundingRect().width() / 2, state1->boundingRect().center().y() - text1->boundingRect().height() / 2);
    text1->setParentItem(state1);

    QGraphicsEllipseItem *state2outer = automatonScene->addEllipse(-5, -5, 70, 70, pen, Qt::NoBrush);
    state2outer->setPos(500, 150);
    QGraphicsEllipseItem *state2inner = automatonScene->addEllipse(0, 0, 60, 60, pen, brush);
    state2inner->setParentItem(state2outer);
    QGraphicsTextItem *text2 = automatonScene->addText("q2");
    text2->setDefaultTextColor(COLOR_TEXT_PRIMARY);
    text2->setPos(state2inner->boundingRect().center().x() - text2->boundingRect().width() / 2, state2inner->boundingRect().center().y() - text2->boundingRect().height() / 2);
    text2->setParentItem(state2inner);

    automatonScene->addLine(160, 180, 300, 180, pen);
    QGraphicsTextItem *label1 = automatonScene->addText("digit");
    label1->setDefaultTextColor(COLOR_TEXT_SECONDARY);
    label1->setPos(220, 185);

    automatonScene->addLine(360, 180, 500, 180, pen);
    QGraphicsTextItem *label2 = automatonScene->addText("letter");
    label2->setDefaultTextColor(COLOR_TEXT_SECONDARY);
    label2->setPos(420, 185);

    automatonScene->addLine(40, 180, 100, 180, pen);
    QGraphicsTextItem *startLabel = automatonScene->addText("start");
    startLabel->setDefaultTextColor(COLOR_TEXT_SECONDARY);
    startLabel->setPos(50, 155);
}

void MainWindow::onAnalyzeClicked()
{
    // ... This function's implementation remains unchanged ...
    QTextEdit *sourceEditor = findChild<QTextEdit*>("sourceCodeEdit");
    QString sourceCode = sourceEditor->toPlainText();
    QLabel* statusLabel = findChild<QLabel*>("statusLabel");

    Lexer lexer(sourceCode);
    std::vector<Token> tokens = lexer.tokenize();
    QString tokensString;
    for (const auto& token : tokens) {
        if (token.type == TokenType::END_OF_FILE) continue;
        QString typeStr = QString::number(static_cast<int>(token.type));
        tokensString += QString("Type: %1, Value: '%2'\n").arg(typeStr, token.value);
    }
    findChild<QTextEdit*>("tokensEdit")->setPlainText(tokensString);

    treeScene->clear();
    Parser parser(tokens);
    std::unique_ptr<ProgramNode> astRoot;
    try {
        astRoot = parser.parse();
        drawAst(astRoot.get(), QPointF(treeScene->width() / 2, 30));
    } catch (const std::runtime_error& e) {
        QMessageBox::critical(this, "Parsing Error", e.what());
        statusLabel->setText("Error: Failed to parse code.");
        return;
    }

    Translator translator;
    QString translatedCode;
    try {
        translatedCode = translator.translate(astRoot.get());
    } catch (const std::runtime_error& e) {
        QMessageBox::critical(this, "Semantic Error", e.what());
        statusLabel->setText("Error: Semantic analysis failed.");
        return;
    }

    findChild<QTextEdit*>("targetCodeEdit")->setPlainText(translatedCode);
    statusLabel->setText("Success: Code translated successfully.");
}

void MainWindow::drawAst(const ASTNode* node, QPointF pos, QPointF parentPos)
{
    // ... This function's implementation remains unchanged ...
    if (!node) return;

    const QColor COLOR_ACCENT_GREEN("#48bb78");
    const QColor COLOR_BACKGROUND_MID("#4c2a2a");
    const QColor COLOR_TEXT_PRIMARY("#f7fafc");

    QPen pen(COLOR_ACCENT_GREEN, 2);
    QBrush brush(COLOR_BACKGROUND_MID);

    QString nodeText;
    if (dynamic_cast<const ProgramNode*>(node)) { nodeText = "Program"; }
    else if (dynamic_cast<const AssignmentNode*>(node)) { nodeText = "="; }
    else if (auto p = dynamic_cast<const BinaryOpNode*>(node)) { nodeText = "Op: " + p->op.value; }
    else if (auto p = dynamic_cast<const IdentifierNode*>(node)) { nodeText = "ID: " + p->token.value; }
    else if (auto p = dynamic_cast<const NumberNode*>(node)) { nodeText = "Num: " + p->token.value; }

    QGraphicsTextItem* textItem = treeScene->addText(nodeText, QFont("Courier", 10, QFont::Bold));
    textItem->setDefaultTextColor(COLOR_TEXT_PRIMARY);
    QRectF textRect = textItem->boundingRect();
    textRect.moveCenter(pos);
    textItem->setPos(textRect.topLeft());

    QGraphicsRectItem* rectItem = treeScene->addRect(textRect.adjusted(-10, -5, 10, 5), pen, brush);
    rectItem->setZValue(-1);

    if (!parentPos.isNull()) {
        treeScene->addLine(QLineF(pos, parentPos), pen);
    }

    qreal yOffset = 90;
    qreal xOffset = 130;
    if (auto p = dynamic_cast<const ProgramNode*>(node)) {
        for (size_t i = 0; i < p->statements.size(); ++i) {
            drawAst(p->statements[i].get(), pos + QPointF(i * xOffset * 1.5, yOffset), pos);
        }
    } else if (auto p = dynamic_cast<const AssignmentNode*>(node)) {
        drawAst(p->identifier.get(), pos + QPointF(-xOffset / 1.8, yOffset), pos);
        drawAst(p->expression.get(), pos + QPointF(xOffset / 1.8, yOffset), pos);
    } else if (auto p = dynamic_cast<const BinaryOpNode*>(node)) {
        drawAst(p->left.get(), pos + QPointF(-xOffset, yOffset), pos);
        drawAst(p->right.get(), pos + QPointF(xOffset, yOffset), pos);
    }
}
