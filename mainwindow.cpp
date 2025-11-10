#include "mainwindow.h"
#include "ui_mainwindow.h" // Standard include for Qt UI files

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
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
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
    // Set window properties
    setWindowTitle("Language Compiler - Theory of Computation");
    resize(1400, 800);

    // Create central widget and set it
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Header
    QWidget *headerWidget = new QWidget();
    headerWidget->setStyleSheet("background-color: #2d3748; padding: 15px;");
    QVBoxLayout *headerLayout = new QVBoxLayout(headerWidget);
    QLabel *titleLabel = new QLabel("Language Compiler - Theory of Computation");
    titleLabel->setStyleSheet("color: #63b3ed; font-size: 20px; font-weight: bold;");
    QLabel *subtitleLabel = new QLabel("Automata-Based Language Translation System");
    subtitleLabel->setStyleSheet("color: #a0aec0; font-size: 12px;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addWidget(subtitleLabel);
    mainLayout->addWidget(headerWidget);

    // Toolbar
    QWidget *toolbarWidget = new QWidget();
    toolbarWidget->setStyleSheet("background-color: #2d3748; padding: 10px;");
    QHBoxLayout *toolbarLayout = new QHBoxLayout(toolbarWidget);
    QPushButton *openBtn = new QPushButton("📁 Open File");
    openBtn->setStyleSheet("background-color: #4299e1; color: white; padding: 8px 16px; border-radius: 4px; font-weight: bold;");
    connect(openBtn, &QPushButton::clicked, this, &MainWindow::onOpenFileClicked);
    QPushButton *saveBtn = new QPushButton("💾 Save");
    saveBtn->setStyleSheet("background-color: #4a5568; color: white; padding: 8px 16px; border-radius: 4px; font-weight: bold;");
    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::onSaveFileClicked);
    QPushButton *analyzeBtn = new QPushButton("▶️ Analyze & Translate");
    analyzeBtn->setStyleSheet("background-color: #48bb78; color: white; padding: 8px 16px; border-radius: 4px; font-weight: bold;");
    connect(analyzeBtn, &QPushButton::clicked, this, &MainWindow::onAnalyzeClicked);
    toolbarLayout->addWidget(openBtn);
    toolbarLayout->addWidget(saveBtn);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(analyzeBtn);
    mainLayout->addWidget(toolbarWidget);

    // Content area with 3 panels
    QWidget *contentWidget = new QWidget();
    QHBoxLayout *contentLayout = new QHBoxLayout(contentWidget);
    contentLayout->setSpacing(0);
    contentLayout->setContentsMargins(0, 0, 0, 0);

    // LEFT PANEL - Source Code Input
    QWidget *leftPanel = new QWidget();
    leftPanel->setStyleSheet("background-color: #1a202c; border-right: 1px solid #4a5568;");
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setSpacing(0);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    QLabel *sourceLabel = new QLabel("  📝 Source Code Input");
    sourceLabel->setStyleSheet("background-color: #2d3748; color: white; padding: 10px; font-weight: bold; border-bottom: 1px solid #4a5568;");
    QTextEdit *sourceCodeEdit = new QTextEdit();
    sourceCodeEdit->setObjectName("sourceCodeEdit"); // Important for finding it later with findChild
    sourceCodeEdit->setStyleSheet("background-color: #1a202c; color: #e2e8f0; font-family: 'Courier New'; font-size: 13px; border: none; padding: 10px;");
    sourceCodeEdit->setPlainText("width = 100;\nheight = width * 2;\narea = (width * height) / 2;");
    leftLayout->addWidget(sourceLabel);
    leftLayout->addWidget(sourceCodeEdit);

    // MIDDLE PANEL - Visualization
    QWidget *middlePanel = new QWidget();
    middlePanel->setStyleSheet("background-color: #1a202c;");
    QVBoxLayout *middleLayout = new QVBoxLayout(middlePanel);
    middleLayout->setSpacing(0);
    middleLayout->setContentsMargins(0, 0, 0, 0);
    QTabWidget *tabWidget = new QTabWidget();
    tabWidget->setStyleSheet(
        "QTabWidget::pane { background-color: #1a202c; border: none; }"
        "QTabBar::tab { background-color: #2d3748; color: #a0aec0; padding: 10px 20px; border: none; }"
        "QTabBar::tab:selected { background-color: #1a202c; color: #63b3ed; border-bottom: 2px solid #63b3ed; }"
        "QTabBar::tab:hover { color: #e2e8f0; }"
        );
    // Automaton view
    automatonScene = new QGraphicsScene();
    QGraphicsView *automatonView = new QGraphicsView(automatonScene);
    automatonView->setStyleSheet("background-color: #1a202c; border: none;");
    automatonView->setRenderHint(QPainter::Antialiasing);
    drawSampleAutomaton();
    // Tokens view
    QTextEdit *tokensEdit = new QTextEdit();
    tokensEdit->setObjectName("tokensEdit");
    tokensEdit->setStyleSheet("background-color: #1a202c; color: #48bb78; font-family: 'Courier New'; font-size: 13px; border: none; padding: 10px;");
    tokensEdit->setReadOnly(true);
    tokensEdit->setPlainText("Click 'Analyze & Translate' to see tokens...");
    // Parse tree view
    treeScene = new QGraphicsScene();
    QGraphicsView *treeView = new QGraphicsView(treeScene);
    treeView->setStyleSheet("background-color: #1a202c; border: none;");
    treeView->setRenderHint(QPainter::Antialiasing);
    QGraphicsTextItem *description = treeScene->addText("Parse Tree will be generated here after analysis.");
    description->setDefaultTextColor(QColor("#e2e8f0"));
    description->setFont(QFont("Arial", 12));
    description->setPos(150, 150);
    tabWidget->addTab(automatonView, "🔀 Automaton View");
    tabWidget->addTab(tokensEdit, "📄 Tokens");
    tabWidget->addTab(treeView, "🌲 Parse Tree");
    middleLayout->addWidget(tabWidget);

    // RIGHT PANEL - Target Code Output
    QWidget *rightPanel = new QWidget();
    rightPanel->setStyleSheet("background-color: #1a202c; border-left: 1px solid #4a5568;");
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setSpacing(0);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    QLabel *targetLabel = new QLabel("  ➡️ Translated Output (C++)");
    targetLabel->setStyleSheet("background-color: #2d3748; color: white; padding: 10px; font-weight: bold; border-bottom: 1px solid #4a5568;");
    QTextEdit *targetCodeEdit = new QTextEdit();
    targetCodeEdit->setObjectName("targetCodeEdit");
    targetCodeEdit->setStyleSheet("background-color: #1a202c; color: #e2e8f0; font-family: 'Courier New'; font-size: 13px; border: none; padding: 10px;");
    targetCodeEdit->setReadOnly(true);
    targetCodeEdit->setPlainText("// Translated code will appear here...");
    rightLayout->addWidget(targetLabel);
    rightLayout->addWidget(targetCodeEdit);

    // Add panels to content layout
    contentLayout->addWidget(leftPanel, 1);
    contentLayout->addWidget(middlePanel, 2);
    contentLayout->addWidget(rightPanel, 1);
    mainLayout->addWidget(contentWidget);

    // Status bar
    QLabel *statusLabel = new QLabel("Ready | Automaton: Loaded | Grammar: Defined");
    statusLabel->setObjectName("statusLabel");
    statusLabel->setStyleSheet("background-color: #2d3748; color: #a0aec0; padding: 8px; font-size: 11px;");
    mainLayout->addWidget(statusLabel);
}

void MainWindow::drawSampleAutomaton()
{
    automatonScene->clear();
    QPen pen(QColor("#63b3ed"), 2);
    QBrush brush(QColor("#1a202c"));
    QColor textColor("#63b3ed");
    QColor labelColor("#a0aec0");

    // State q0
    QGraphicsEllipseItem *state0 = automatonScene->addEllipse(0, 0, 60, 60, pen, brush);
    state0->setPos(100, 150);
    QGraphicsTextItem *text0 = automatonScene->addText("q0");
    text0->setDefaultTextColor(textColor);
    text0->setPos(state0->boundingRect().center().x() - text0->boundingRect().width() / 2,
                  state0->boundingRect().center().y() - text0->boundingRect().height() / 2);
    text0->setParentItem(state0);

    // State q1
    QGraphicsEllipseItem *state1 = automatonScene->addEllipse(0, 0, 60, 60, pen, brush);
    state1->setPos(300, 150);
    QGraphicsTextItem *text1 = automatonScene->addText("q1");
    text1->setDefaultTextColor(textColor);
    text1->setPos(state1->boundingRect().center().x() - text1->boundingRect().width() / 2,
                  state1->boundingRect().center().y() - text1->boundingRect().height() / 2);
    text1->setParentItem(state1);

    // State q2 (accepting)
    QGraphicsEllipseItem *state2outer = automatonScene->addEllipse(-5, -5, 70, 70, pen, Qt::NoBrush);
    state2outer->setPos(500, 150);
    QGraphicsEllipseItem *state2inner = automatonScene->addEllipse(0, 0, 60, 60, pen, brush);
    state2inner->setParentItem(state2outer);
    QGraphicsTextItem *text2 = automatonScene->addText("q2");
    text2->setDefaultTextColor(textColor);
    text2->setPos(state2inner->boundingRect().center().x() - text2->boundingRect().width() / 2,
                  state2inner->boundingRect().center().y() - text2->boundingRect().height() / 2);
    text2->setParentItem(state2inner);

    // Transitions
    automatonScene->addLine(160, 180, 300, 180, pen);
    QGraphicsTextItem *label1 = automatonScene->addText("digit");
    label1->setDefaultTextColor(labelColor);
    label1->setPos(220, 185);

    automatonScene->addLine(360, 180, 500, 180, pen);
    QGraphicsTextItem *label2 = automatonScene->addText("letter");
    label2->setDefaultTextColor(labelColor);
    label2->setPos(420, 185);

    // Start arrow
    automatonScene->addLine(40, 180, 100, 180, pen);
    QGraphicsTextItem *startLabel = automatonScene->addText("start");
    startLabel->setDefaultTextColor(labelColor);
    startLabel->setPos(50, 155);

    // Add description
    QGraphicsTextItem *description = automatonScene->addText("Finite Automaton - Token Recognition");
    description->setDefaultTextColor(labelColor);
    description->setPos(200, 300);
}

// --- SLOTS IMPLEMENTATION ---

void MainWindow::onAnalyzeClicked()
{
    // 1. Get source code from UI
    QTextEdit *sourceEditor = findChild<QTextEdit*>("sourceCodeEdit");
    QString sourceCode = sourceEditor->toPlainText();
    QLabel* statusLabel = findChild<QLabel*>("statusLabel");

    // --- STAGE 1: LEXICAL ANALYSIS ---
    Lexer lexer(sourceCode);
    std::vector<Token> tokens = lexer.tokenize();
    QString tokensString;
    for (const auto& token : tokens) {
        if (token.type == TokenType::END_OF_FILE) continue;
        // A simple way to get a string representation of the enum
        QString typeStr = QString::number(static_cast<int>(token.type));
        tokensString += QString("Type: %1, Value: '%2'\n").arg(typeStr, token.value);
    }
    findChild<QTextEdit*>("tokensEdit")->setPlainText(tokensString);

    // --- STAGE 2: PARSING (SYNTAX ANALYSIS) ---
    treeScene->clear();
    Parser parser(tokens);
    std::unique_ptr<ProgramNode> astRoot;
    try {
        astRoot = parser.parse();
        // The scene's coordinate system might need adjustment depending on view size.
        // We pass a starting point for the root of the tree.
        drawAst(astRoot.get(), QPointF(treeScene->width() / 2 + 150, 30));
    } catch (const std::runtime_error& e) {
        QMessageBox::critical(this, "Parsing Error", e.what());
        statusLabel->setText("Error: Failed to parse code.");
        return;
    }

    // --- STAGE 3: SEMANTIC ANALYSIS & TRANSLATION ---
    Translator translator;
    QString translatedCode;
    try {
        translatedCode = translator.translate(astRoot.get());
    } catch (const std::runtime_error& e) {
        QMessageBox::critical(this, "Semantic Error", e.what());
        statusLabel->setText("Error: Semantic analysis failed.");
        return;
    }

    // --- STAGE 4: DISPLAY RESULTS ---
    findChild<QTextEdit*>("targetCodeEdit")->setPlainText(translatedCode);
    statusLabel->setText("Success: Code translated successfully.");
}

void MainWindow::onOpenFileClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open Source File", "", "All Files (*);;Text Files (*.txt)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            findChild<QTextEdit*>("sourceCodeEdit")->setPlainText(in.readAll());
            file.close();
        } else {
            QMessageBox::warning(this, "Error", "Could not open the file.");
        }
    }
}

void MainWindow::onSaveFileClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save Translated File", "", "C++ Files (*.cpp);;All Files (*)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << findChild<QTextEdit*>("targetCodeEdit")->toPlainText();
            file.close();
        } else {
            QMessageBox::warning(this, "Error", "Could not save the file.");
        }
    }
}

// --- AST VISUALIZATION ---

void MainWindow::drawAst(const ASTNode* node, QPointF pos, QPointF parentPos)
{
    if (!node) return;

    QPen pen(QColor("#48bb78"), 2);
    QBrush brush(QColor("#2d3748"));
    QColor textColor("#e2e8f0");
    QString nodeText;

    // Determine node text based on its dynamic type
    if (dynamic_cast<const ProgramNode*>(node)) { nodeText = "Program"; }
    else if (dynamic_cast<const AssignmentNode*>(node)) { nodeText = "="; }
    else if (auto p = dynamic_cast<const BinaryOpNode*>(node)) { nodeText = "Op: " + p->op.value; }
    else if (auto p = dynamic_cast<const IdentifierNode*>(node)) { nodeText = "ID: " + p->token.value; }
    else if (auto p = dynamic_cast<const NumberNode*>(node)) { nodeText = "Num: " + p->token.value; }

    // Draw the node's text and a surrounding rectangle
    QGraphicsTextItem* textItem = treeScene->addText(nodeText, QFont("Courier", 10, QFont::Bold));
    textItem->setDefaultTextColor(textColor);
    QRectF textRect = textItem->boundingRect();
    textRect.moveCenter(pos);
    textItem->setPos(textRect.topLeft());

    QGraphicsRectItem* rectItem = treeScene->addRect(textRect.adjusted(-10, -5, 10, 5), pen, brush);
    rectItem->setZValue(-1); // Place rectangle behind text

    // Draw a line connecting to the parent node
    if (!parentPos.isNull()) {
        treeScene->addLine(QLineF(pos, parentPos), pen);
    }

    // --- Recursive calls to draw children ---
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
