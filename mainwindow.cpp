#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "lexer.h"
#include "parser.h"
#include "translator.h"
#include <stdexcept>
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
#include <map>
#include <set>
#include <cmath>

using namespace std;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setupUI();

    const QString pythonCode = R"(def calculate_triangle_properties(width):
    if not isinstance(width, (int, float)) or width <= 0:
        print("Error: Please provide a positive number for the width.")
        return None

    height = width * 2
    area = (width * height) / 2

    properties = {
        'width': width,
        'height': height,
        'area': area
    }
    return properties

if __name__ == "__main__":
    try:
        user_input_width = int(input("Enter the width of the triangle: "))
        triangle_data = calculate_triangle_properties(user_input_width)

        if triangle_data:
            print("Triangle Properties:")
            print("Width:", triangle_data['width'])
            print("Height:", triangle_data['height'])
            print("Area:", triangle_data['area'])
    except ValueError:
        print("Invalid input"))";
    findChild<QTextEdit*>("sourceCodeEdit")->setPlainText(pythonCode);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::onAnalyzeClicked() {
    QString sourceCode = findChild<QTextEdit*>("sourceCodeEdit")->toPlainText();
    QLabel* statusLabel = findChild<QLabel*>("statusLabel");
    QTextEdit* tokensEdit = findChild<QTextEdit*>("tokensEdit");
    QTextEdit* targetCodeEdit = findChild<QTextEdit*>("targetCodeEdit");

    automatonScene->clear();
    treeScene->clear();
    tokensEdit->clear();
    targetCodeEdit->clear();

    try {
        Lexer lexer(sourceCode);
        vector<Token> tokens = lexer.tokenize();

        QString tokensString;
        for (const auto& token : tokens) {
            tokensString += QString("Line %1: Type: %2, Value: '%3'\n")
            .arg(token.line)
                .arg((int)token.type)
                .arg(token.value);
        }
        tokensEdit->setPlainText(tokensString);

        Parser parser(tokens);
        unique_ptr<ProgramNode> astRoot = parser.parse();

        if (astRoot) {
            // Draw Automaton
            drawTrueAutomaton(parser);

            // Draw AST visualization in Parse Tree tab
            drawParseTree(astRoot.get(), QPointF(treeScene->width() / 2, 50));

            // Translate to C++
            Translator translator;
            QString cppCode = translator.translate(astRoot.get());
            targetCodeEdit->setPlainText(cppCode);

            statusLabel->setText("Success: Code analyzed and translated successfully.");
        } else {
            statusLabel->setText("Error: Failed to generate AST.");
        }
    } catch (const runtime_error& e) {
        QMessageBox::critical(this, "Analysis Error", e.what());
        statusLabel->setText("Error: Analysis failed.");
    }
}

void MainWindow::drawParseTree(const ASTNode* node, QPointF pos, QPointF parentPos, int depth) {
    if (!node) return;

    const QColor NODE_COLOR("#48bb78"), LINE_COLOR("#a09393"), TEXT_COLOR("#f7fafc");
    const QBrush NODE_BRUSH(QColor("#1a3a2a"));
    QPen nodePen(NODE_COLOR, 2), linePen(LINE_COLOR, 1.5);

    // Create the node
    QGraphicsEllipseItem *state = treeScene->addEllipse(0, 0, 160, 60, nodePen, NODE_BRUSH);
    state->setPos(pos - QPointF(80, 30));

    QGraphicsTextItem *textItem = treeScene->addText(node->getNodeName());
    textItem->setDefaultTextColor(TEXT_COLOR);
    textItem->setFont(QFont("Arial", 9, QFont::Bold));

    // Center the text in the ellipse
    QRectF textRect = textItem->boundingRect();
    QRectF ellipseRect = state->boundingRect();
    textItem->setPos(
        state->pos().x() + (ellipseRect.width() - textRect.width()) / 2,
        state->pos().y() + (ellipseRect.height() - textRect.height()) / 2
        );

    // Draw line to parent if exists
    if (!parentPos.isNull()) {
        treeScene->addLine(QLineF(parentPos, pos), linePen);
    }

    qreal yOffset = 120;
    qreal xSpacing = 1000 / (depth + 1);

    // Recursively draw children based on node type
    if (auto p = dynamic_cast<const ProgramNode*>(node)) {
        qreal totalWidth = (p->statements.size() - 1) * xSpacing;
        qreal startX = pos.x() - totalWidth / 2;

        for (size_t i = 0; i < p->statements.size(); ++i) {
            QPointF childPos(startX + i * xSpacing, pos.y() + yOffset);
            drawParseTree(p->statements[i].get(), childPos, pos, depth + 1);
        }
    }
    else if (auto p = dynamic_cast<const AssignmentNode*>(node)) {
        drawParseTree(p->identifier.get(), QPointF(pos.x() - xSpacing/2, pos.y() + yOffset), pos, depth + 1);
        drawParseTree(p->expression.get(), QPointF(pos.x() + xSpacing/2, pos.y() + yOffset), pos, depth + 1);
    }
    else if (auto p = dynamic_cast<const BinaryOpNode*>(node)) {
        drawParseTree(p->left.get(), QPointF(pos.x() - xSpacing/2, pos.y() + yOffset), pos, depth + 1);
        drawParseTree(p->right.get(), QPointF(pos.x() + xSpacing/2, pos.y() + yOffset), pos, depth + 1);
    }
    else if (auto p = dynamic_cast<const UnaryOpNode*>(node)) {
        drawParseTree(p->right.get(), QPointF(pos.x(), pos.y() + yOffset), pos, depth + 1);
    }
    else if (auto p = dynamic_cast<const PrintNode*>(node)) {
        drawParseTree(p->expression.get(), QPointF(pos.x(), pos.y() + yOffset), pos, depth + 1);
    }
    else if (auto p = dynamic_cast<const ReturnNode*>(node)) {
        if (p->expression) {
            drawParseTree(p->expression.get(), QPointF(pos.x(), pos.y() + yOffset), pos, depth + 1);
        }
    }
    else if (auto p = dynamic_cast<const FunctionCallNode*>(node)) {
        drawParseTree(p->name.get(), QPointF(pos.x(), pos.y() + yOffset), pos, depth + 1);

        qreal totalWidth = (p->arguments.size() - 1) * xSpacing/2;
        qreal startX = pos.x() - totalWidth / 2;

        for (size_t i = 0; i < p->arguments.size(); ++i) {
            QPointF childPos(startX + i * xSpacing/2, pos.y() + yOffset * 2);
            drawParseTree(p->arguments[i].get(), childPos, pos, depth + 2);
        }
    }
    else if (auto p = dynamic_cast<const IfNode*>(node)) {
        drawParseTree(p->condition.get(), QPointF(pos.x() - xSpacing/3, pos.y() + yOffset), pos, depth + 1);
        drawParseTree(p->body.get(), QPointF(pos.x() + xSpacing/3, pos.y() + yOffset), pos, depth + 1);
    }
    else if (auto p = dynamic_cast<const BlockNode*>(node)) {
        for (size_t i = 0; i < p->statements.size(); ++i) {
            QPointF childPos(pos.x(), pos.y() + yOffset * (i + 1));
            drawParseTree(p->statements[i].get(), childPos, pos, depth + 1);
        }
    }
    else if (auto p = dynamic_cast<const FunctionDefNode*>(node)) {
        // Draw function name
        drawParseTree(p->name.get(), QPointF(pos.x() - xSpacing/3, pos.y() + yOffset), pos, depth + 1);

        // Draw parameters
        if (!p->parameters.empty()) {
            qreal paramWidth = (p->parameters.size() - 1) * xSpacing/4;
            qreal paramStartX = pos.x() - paramWidth / 2;

            for (size_t i = 0; i < p->parameters.size(); ++i) {
                QPointF paramPos(paramStartX + i * xSpacing/4, pos.y() + yOffset);
                drawParseTree(p->parameters[i].get(), paramPos, pos, depth + 1);
            }
        }

        // Draw body
        drawParseTree(p->body.get(), QPointF(pos.x() + xSpacing/3, pos.y() + yOffset), pos, depth + 1);
    }
    else if (auto p = dynamic_cast<const TryExceptNode*>(node)) {
        drawParseTree(p->try_body.get(), QPointF(pos.x(), pos.y() + yOffset), pos, depth + 1);
    }
}

void MainWindow::drawTrueAutomaton(const Parser& parser) {
    automatonScene->clear();

    // Define all colors locally
    const QColor STATE_COLOR("#e53e3e");
    const QColor TRANSITION_COLOR("#48bb78");
    const QColor TEXT_COLOR("#f7fafc");
    const QColor HISTORY_COLOR("#ffeb3b");
    const QColor CURRENT_STATE_COLOR("#ff9800");
    const QBrush STATE_BRUSH(QColor("#2c1d1d"));

    QPen statePen(STATE_COLOR, 3);
    QPen transitionPen(TRANSITION_COLOR, 2);
    QPen historyPen(HISTORY_COLOR, 6);
    QPen currentStatePen(CURRENT_STATE_COLOR, 4);

    // Create organized state positions
    map<ParserState, QPointF> statePositions;

    // Organize states by category
    vector<ParserState> programStates = {ParserState::START, ParserState::EXPECT_STATEMENT, ParserState::END_STATEMENT};
    vector<ParserState> functionStates = {ParserState::IN_FUNCTION_DEF, ParserState::IN_FUNCTION_PARAMS, ParserState::IN_FUNCTION_BODY, ParserState::IN_FUNCTION_CALL};
    vector<ParserState> controlStates = {ParserState::IN_IF_CONDITION, ParserState::IN_IF_BODY, ParserState::IN_TRY_BLOCK, ParserState::IN_EXCEPT_BLOCK};
    vector<ParserState> expressionStates = {ParserState::IN_ASSIGNMENT, ParserState::IN_EXPRESSION, ParserState::IN_TERM, ParserState::IN_FACTOR, ParserState::EXPECT_OPERATOR, ParserState::EXPECT_OPERAND};

    // Position states in organized grid
    int startX = 150, startY = 150;
    int colSpacing = 350, rowSpacing = 120;

    // Column 1: Program Flow
    for (size_t i = 0; i < programStates.size(); ++i) {
        statePositions[programStates[i]] = QPointF(startX, startY + i * rowSpacing);
    }

    // Column 2: Functions
    for (size_t i = 0; i < functionStates.size(); ++i) {
        statePositions[functionStates[i]] = QPointF(startX + colSpacing, startY + i * rowSpacing);
    }

    // Column 3: Control Structures
    for (size_t i = 0; i < controlStates.size(); ++i) {
        statePositions[controlStates[i]] = QPointF(startX + colSpacing * 2, startY + i * rowSpacing);
    }

    // Column 4: Expressions
    for (size_t i = 0; i < expressionStates.size(); ++i) {
        statePositions[expressionStates[i]] = QPointF(startX + colSpacing * 3, startY + i * rowSpacing);
    }

    // USE REAL PARSER DATA
    vector<AutomatonTransition> transitions = parser.getTransitions();
    vector<pair<ParserState, Token>> stateHistory = parser.getStateHistory();

    // If no real transitions were recorded, show a message
    if (transitions.empty()) {
        QGraphicsTextItem* noDataText = automatonScene->addText("No automaton data available.\nParser transitions were not recorded.");
        noDataText->setDefaultTextColor(QColor("#ff6b6b"));
        noDataText->setFont(QFont("Arial", 14, QFont::Bold));
        noDataText->setPos(400, 400);
        return;
    }

    // Draw transitions
    for (const auto& transition : transitions) {
        QPointF fromPos = statePositions[transition.fromState];
        QPointF toPos = statePositions[transition.toState];

        // Draw straight line for simplicity
        QLineF line(fromPos, toPos);
        automatonScene->addLine(line, transitionPen);

        // Draw arrow head
        double angle = std::atan2(-line.dy(), line.dx());
        QPointF arrowP1 = toPos + QPointF(sin(angle + M_PI / 3) * 15, cos(angle + M_PI / 3) * 15);
        QPointF arrowP2 = toPos + QPointF(sin(angle + M_PI - M_PI / 3) * 15, cos(angle + M_PI - M_PI / 3) * 15);

        QPolygonF arrowHead;
        arrowHead << toPos << arrowP1 << arrowP2;
        automatonScene->addPolygon(arrowHead, transitionPen, QBrush(TRANSITION_COLOR));

        // Draw transition label
        QString tokenName = getTokenName(transition.triggerToken);
        QString labelText = tokenName;

        QGraphicsTextItem* label = automatonScene->addText(labelText);
        label->setDefaultTextColor(TEXT_COLOR);
        label->setFont(QFont("Arial", 8, QFont::Bold));

        QPointF labelPos = (fromPos + toPos) / 2;
        label->setPos(labelPos.x() - 10, labelPos.y() - 15);
    }

    // Draw states
    for (const auto& [state, pos] : statePositions) {
        // Draw state circle
        QGraphicsEllipseItem* stateCircle = automatonScene->addEllipse(-25, -25, 50, 50, statePen, STATE_BRUSH);
        stateCircle->setPos(pos);

        // Draw state name
        QString stateName = getStateName(state);
        QGraphicsTextItem* stateText = automatonScene->addText(stateName);
        stateText->setDefaultTextColor(TEXT_COLOR);
        stateText->setFont(QFont("Arial", 7, QFont::Bold));

        QRectF textRect = stateText->boundingRect();
        stateText->setPos(pos.x() - textRect.width()/2, pos.y() - textRect.height()/2);
    }

    // Draw execution path from REAL parser history
    if (!stateHistory.empty()) {
        for (size_t i = 1; i < stateHistory.size(); ++i) {
            QPointF fromPos = statePositions[stateHistory[i-1].first];
            QPointF toPos = statePositions[stateHistory[i].first];

            QLineF pathLine(fromPos, toPos);
            automatonScene->addLine(pathLine, historyPen);
        }

        // Highlight current state
        if (!stateHistory.empty()) {
            ParserState currentState = stateHistory.back().first;
            QPointF currentPos = statePositions[currentState];

            QGraphicsEllipseItem* currentHighlight = automatonScene->addEllipse(-30, -30, 60, 60, currentStatePen, QBrush(Qt::NoBrush));
            currentHighlight->setPos(currentPos);
        }
    }


}

QString MainWindow::getStateName(ParserState state) {
    switch(state) {
    case ParserState::START: return "START";
    case ParserState::EXPECT_STATEMENT: return "EXPECT_STMT";
    case ParserState::IN_FUNCTION_DEF: return "FUNC_DEF";
    case ParserState::IN_FUNCTION_PARAMS: return "FUNC_PARAMS";
    case ParserState::IN_FUNCTION_BODY: return "FUNC_BODY";
    case ParserState::IN_IF_CONDITION: return "IF_COND";
    case ParserState::IN_IF_BODY: return "IF_BODY";
    case ParserState::IN_ASSIGNMENT: return "ASSIGN";
    case ParserState::IN_EXPRESSION: return "EXPR";
    case ParserState::IN_TERM: return "TERM";
    case ParserState::IN_FACTOR: return "FACTOR";
    case ParserState::IN_FUNCTION_CALL: return "FUNC_CALL";
    case ParserState::IN_TRY_BLOCK: return "TRY_BLOCK";
    case ParserState::IN_EXCEPT_BLOCK: return "EXCEPT_BLOCK";
    case ParserState::EXPECT_OPERATOR: return "EXPECT_OP";
    case ParserState::EXPECT_OPERAND: return "EXPECT_OPERAND";
    case ParserState::END_STATEMENT: return "END_STMT";
    default: return "UNKNOWN";
    }
}

QString MainWindow::getTokenName(TokenType token) {
    switch(token) {
    case TokenType::DEF: return "DEF";
    case TokenType::IF: return "IF";
    case TokenType::IDENTIFIER: return "ID";
    case TokenType::EQUAL: return "=";
    case TokenType::LPAREN: return "(";
    case TokenType::RPAREN: return ")";
    case TokenType::COLON: return ":";
    case TokenType::RETURN: return "RETURN";
    case TokenType::PRINT: return "PRINT";
    case TokenType::TRY: return "TRY";
    case TokenType::EXCEPT: return "EXCEPT";
    case TokenType::OR: return "OR";
    case TokenType::NOT: return "NOT";
    case TokenType::NUMBER: return "NUM";
    case TokenType::STRING: return "STR";
    default: return "TOK";
    }
}



void MainWindow::setupUI() {
    const QString COLOR_BACKGROUND_DARK = "#2c1d1d", COLOR_BACKGROUND_MID = "#4c2a2a", COLOR_BORDER = "#6c3a3a",
        COLOR_TEXT_PRIMARY = "#f7fafc", COLOR_TEXT_SECONDARY = "#a09393", COLOR_ACCENT_RED = "#e53e3e",
        COLOR_ACCENT_GREEN = "#48bb78";

    setWindowTitle("Compiler");
    resize(1400, 850);
    setStyleSheet(QString("background-color: %1;").arg(COLOR_BACKGROUND_DARK));

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QWidget *topRowWidget = new QWidget();
    QHBoxLayout *topRowLayout = new QHBoxLayout(topRowWidget);
    topRowLayout->setSpacing(0);
    topRowLayout->setContentsMargins(0, 0, 0, 0);

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
    sourceLayout->addWidget(sourceLabel);
    sourceLayout->addWidget(sourceCodeEdit);

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
    targetCodeEdit->setPlainText("// Results will appear here...");
    targetLayout->addWidget(targetLabel);
    targetLayout->addWidget(targetCodeEdit);

    topRowLayout->addWidget(sourcePanel, 1);
    topRowLayout->addWidget(targetPanel, 1);

    QTabWidget *tabWidget = new QTabWidget();
    tabWidget->setStyleSheet(QString("QTabWidget::pane { background-color: %1; border: none; border-top: 1px solid %2; } QTabBar::tab { background-color: %3; color: %4; padding: 10px 20px; border: none; } QTabBar::tab:selected { background-color: %1; color: %5; border-top: 2px solid %5; } QTabBar::tab:hover { color: %6; }").arg(COLOR_BACKGROUND_DARK, COLOR_BORDER, COLOR_BACKGROUND_MID, COLOR_TEXT_SECONDARY, COLOR_ACCENT_RED, COLOR_TEXT_PRIMARY));

    automatonScene = new QGraphicsScene(this);
    automatonScene->setSceneRect(0, 0, 2000, 2000);
    QGraphicsView *automatonView = new QGraphicsView(automatonScene);
    automatonView->setStyleSheet(QString("background-color: %1; border: none;").arg(COLOR_BACKGROUND_DARK));
    automatonView->setRenderHint(QPainter::Antialiasing);

    QTextEdit *tokensEdit = new QTextEdit();
    tokensEdit->setObjectName("tokensEdit");
    tokensEdit->setStyleSheet(QString("background-color: %1; color: %2; font-family: 'Courier New'; font-size: 13px; border: none; padding: 10px;").arg(COLOR_BACKGROUND_DARK, COLOR_ACCENT_GREEN));
    tokensEdit->setReadOnly(true);

    treeScene = new QGraphicsScene(this);
    treeScene->setSceneRect(0, 0, 3000, 3000);
    QGraphicsView *treeView = new QGraphicsView(treeScene);
    treeView->setStyleSheet(QString("background-color: %1; border: none;").arg(COLOR_BACKGROUND_DARK));
    treeView->setRenderHint(QPainter::Antialiasing);

    tabWidget->addTab(automatonView, "Automaton");
    tabWidget->addTab(tokensEdit, "Tokens");
    tabWidget->addTab(treeView, "Parse Tree");

    mainLayout->addWidget(topRowWidget, 1);
    mainLayout->addWidget(tabWidget, 1);

    QWidget *toolbarWidget = new QWidget();
    toolbarWidget->setStyleSheet(QString("padding: 10px; border-top: 1px solid %1;").arg(COLOR_BORDER));
    QHBoxLayout *toolbarLayout = new QHBoxLayout(toolbarWidget);
    QPushButton *analyzeBtn = new QPushButton("Analyze & Translate");
    analyzeBtn->setStyleSheet(QString("QPushButton { background-color: %1; color: %2; padding: 10px 20px; border-radius: 5px; font-weight: bold; } QPushButton:hover { background-color: #c53030; }").arg(COLOR_ACCENT_RED, COLOR_TEXT_PRIMARY));
    analyzeBtn->setCursor(Qt::PointingHandCursor);
    connect(analyzeBtn, &QPushButton::clicked, this, &MainWindow::onAnalyzeClicked);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(analyzeBtn);
    toolbarLayout->addStretch();
    mainLayout->addWidget(toolbarWidget);

    QLabel *statusLabel = new QLabel("Ready");
    statusLabel->setObjectName("statusLabel");
    statusLabel->setStyleSheet(QString("background-color: %1; color: %2; padding: 8px; font-size: 11px;").arg(COLOR_BACKGROUND_MID, COLOR_TEXT_SECONDARY));
    mainLayout->addWidget(statusLabel);
}
