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
#include <QPainterPath>
#include <QSplitter>
#include <QWheelEvent>
#include <map>
#include <set>
#include <cmath>

using namespace std;

class ZoomableView : public QGraphicsView {
public:
    explicit ZoomableView(QGraphicsScene *scene, QWidget *parent = nullptr)
        : QGraphicsView(scene, parent) {
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    }

protected:
    void wheelEvent(QWheelEvent *event) override {
        if (event->modifiers() & Qt::ControlModifier) {
            const double zoomFactor = 1.15;
            if (event->angleDelta().y() > 0) {
                scale(zoomFactor, zoomFactor);
            } else {
                scale(1.0 / zoomFactor, 1.0 / zoomFactor);
            }
            event->accept();
        } else {
            QGraphicsView::wheelEvent(event);
        }
    }
};

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setupUI();

    const QString pythonCode = R"(def factorial(n):
    result = 1
    i = 1
    while i <= n:
        result = result * i
        i = i + 1
    return result

num = 5
fact = factorial(num)

if fact == 120:
    print("Test Passed!")
else:
    print("Test Failed!")
)";
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
            drawTrueAutomaton(parser);
            drawParseTree(astRoot.get(), QPointF(treeScene->width() / 2, 50));

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

    QGraphicsEllipseItem *ellipse = treeScene->addEllipse(0, 0, 160, 50, nodePen, NODE_BRUSH);
    ellipse->setPos(pos - QPointF(80, 25));

    QGraphicsTextItem *textItem = treeScene->addText(node->getNodeName());
    textItem->setDefaultTextColor(TEXT_COLOR);
    textItem->setFont(QFont("Arial", 9, QFont::Bold));
    QRectF textRect = textItem->boundingRect();
    textItem->setPos(
        pos.x() - textRect.width() / 2,
        pos.y() - textRect.height() / 2
        );

    if (!parentPos.isNull()) {
        QPainterPath path;
        path.moveTo(parentPos);
        QPointF controlPoint1(parentPos.x(), parentPos.y() + 60);
        QPointF controlPoint2(pos.x(), pos.y() - 60);
        path.cubicTo(controlPoint1, controlPoint2, pos);
        treeScene->addPath(path, linePen);
    }

    vector<const ASTNode*> children;
    if (auto p = dynamic_cast<const ProgramNode*>(node)) {
        for(const auto& stmt : p->statements) children.push_back(stmt.get());
    } else if (auto p = dynamic_cast<const AssignmentNode*>(node)) {
        children.push_back(p->identifier.get());
        children.push_back(p->expression.get());
    } else if (auto p = dynamic_cast<const BinaryOpNode*>(node)) {
        children.push_back(p->left.get());
        children.push_back(p->right.get());
    } else if (auto p = dynamic_cast<const UnaryOpNode*>(node)) {
        children.push_back(p->right.get());
    } else if (auto p = dynamic_cast<const PrintNode*>(node)) {
        children.push_back(p->expression.get());
    } else if (auto p = dynamic_cast<const ReturnNode*>(node)) {
        if(p->expression) children.push_back(p->expression.get());
    } else if (auto p = dynamic_cast<const FunctionCallNode*>(node)) {
        children.push_back(p->name.get());
        for(const auto& arg : p->arguments) children.push_back(arg.get());
    } else if (auto p = dynamic_cast<const IfNode*>(node)) {
        children.push_back(p->condition.get());
        children.push_back(p->body.get());
        if(p->else_branch) children.push_back(p->else_branch.get());
    } else if (auto p = dynamic_cast<const WhileNode*>(node)) {
        children.push_back(p->condition.get());
        children.push_back(p->body.get());
    } else if (auto p = dynamic_cast<const BlockNode*>(node)) {
        for(const auto& stmt : p->statements) children.push_back(stmt.get());
    } else if (auto p = dynamic_cast<const FunctionDefNode*>(node)) {
        children.push_back(p->name.get());
        for(const auto& param : p->parameters) children.push_back(param.get());
        children.push_back(p->body.get());
    } else if (auto p = dynamic_cast<const TryExceptNode*>(node)) {
        children.push_back(p->try_body.get());
    }

    if (!children.empty()) {
        qreal yOffset = 150;
        qreal totalWidth = children.size() * 250;
        if (depth < 2) totalWidth = children.size() * 400;

        qreal startX = pos.x() - totalWidth / 2;
        qreal xSpacing = totalWidth / children.size();

        for (size_t i = 0; i < children.size(); ++i) {
            QPointF childPos(startX + i * xSpacing + xSpacing / 2, pos.y() + yOffset);
            drawParseTree(children[i], childPos, pos, depth + 1);
        }
    }
}

void MainWindow::drawTrueAutomaton(const Parser& parser) {
    automatonScene->clear();

    const QColor STATE_COLOR("#e53e3e"), TRANSITION_COLOR("#48bb78"), TEXT_COLOR("#f7fafc"),
        HISTORY_COLOR("#ffeb3b"), CURRENT_STATE_COLOR("#ff9800");
    const QBrush STATE_BRUSH(QColor("#2c1d1d")), HISTORY_BRUSH(QColor("#3f3b0f"));

    QPen statePen(STATE_COLOR, 3), transitionPen(TRANSITION_COLOR, 2), historyPen(HISTORY_COLOR, 3),
        currentStatePen(CURRENT_STATE_COLOR, 4);

    map<ParserState, QPointF> statePositions;
    vector<ParserState> programStates = {ParserState::START, ParserState::EXPECT_STATEMENT, ParserState::END_STATEMENT};
    vector<ParserState> functionStates = {ParserState::IN_FUNCTION_DEF, ParserState::IN_FUNCTION_PARAMS, ParserState::IN_FUNCTION_BODY, ParserState::IN_FUNCTION_CALL};
    vector<ParserState> controlStates = {ParserState::IN_IF_CONDITION, ParserState::IN_IF_BODY, ParserState::IN_TRY_BLOCK, ParserState::IN_EXCEPT_BLOCK};
    vector<ParserState> expressionStates = {ParserState::IN_ASSIGNMENT, ParserState::IN_EXPRESSION, ParserState::IN_TERM, ParserState::IN_FACTOR, ParserState::EXPECT_OPERATOR, ParserState::EXPECT_OPERAND};

    int startX = 150, startY = 150, colSpacing = 350, rowSpacing = 120;
    for (size_t i = 0; i < programStates.size(); ++i) statePositions[programStates[i]] = QPointF(startX, startY + i * rowSpacing);
    for (size_t i = 0; i < functionStates.size(); ++i) statePositions[functionStates[i]] = QPointF(startX + colSpacing, startY + i * rowSpacing);
    for (size_t i = 0; i < controlStates.size(); ++i) statePositions[controlStates[i]] = QPointF(startX + colSpacing * 2, startY + i * rowSpacing);
    for (size_t i = 0; i < expressionStates.size(); ++i) statePositions[expressionStates[i]] = QPointF(startX + colSpacing * 3, startY + i * rowSpacing);

    vector<pair<ParserState, Token>> stateHistory = parser.getStateHistory();
    if (stateHistory.empty()) return;

    set<ParserState> visitedStates;
    for(const auto& historyItem : stateHistory) {
        visitedStates.insert(historyItem.first);
    }

    for (ParserState state : visitedStates) {
        QPointF pos = statePositions[state];
        QGraphicsEllipseItem* stateCircle = automatonScene->addEllipse(-25, -25, 50, 50, statePen, STATE_BRUSH);
        stateCircle->setPos(pos);
        QString stateName = getStateName(state);
        QGraphicsTextItem* stateText = automatonScene->addText(stateName);
        stateText->setDefaultTextColor(TEXT_COLOR);
        stateText->setFont(QFont("Arial", 7, QFont::Bold));
        QRectF textRect = stateText->boundingRect();
        stateText->setPos(pos.x() - textRect.width()/2, pos.y() - textRect.height()/2);
    }

    for (size_t i = 1; i < stateHistory.size(); ++i) {
        ParserState from = stateHistory[i-1].first;
        ParserState to = stateHistory[i].first;
        Token trigger = stateHistory[i].second;

        QPointF fromPos = statePositions[from];
        QPointF toPos = statePositions[to];

        QPainterPath path;
        path.moveTo(fromPos);
        QPointF c1 = (fromPos + toPos) / 2 + QPointF(30, 30);
        path.quadTo(c1, toPos);
        automatonScene->addPath(path, historyPen);

        double angle = std::atan2(-path.slopeAtPercent(1), 1);
        QPointF arrowP1 = toPos + QPointF(sin(angle - M_PI / 3) * 15, cos(angle - M_PI / 3) * 15);
        QPointF arrowP2 = toPos + QPointF(sin(angle - M_PI + M_PI / 3) * 15, cos(angle - M_PI + M_PI / 3) * 15);
        QPolygonF arrowHead;
        arrowHead << toPos << arrowP1 << arrowP2;
        automatonScene->addPolygon(arrowHead, historyPen, QBrush(HISTORY_COLOR));

        QString tokenName = getTokenName(trigger.type);
        QGraphicsTextItem* label = automatonScene->addText(tokenName);
        label->setDefaultTextColor(HISTORY_COLOR);
        label->setFont(QFont("Arial", 8, QFont::Bold));
        label->setPos((fromPos + toPos) / 2 + QPointF(10, -20));
    }

    ParserState currentState = stateHistory.back().first;
    if(statePositions.count(currentState) > 0) {
        QPointF currentPos = statePositions[currentState];
        QGraphicsEllipseItem* currentHighlight = automatonScene->addEllipse(-30, -30, 60, 60, currentStatePen, QBrush(Qt::NoBrush));
        currentHighlight->setPos(currentPos);
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
    case TokenType::DEF: return "DEF"; case TokenType::IF: return "IF";
    case TokenType::WHILE: return "WHILE"; case TokenType::ELSE: return "ELSE";
    case TokenType::ELIF: return "ELIF";
    case TokenType::IDENTIFIER: return "ID"; case TokenType::EQUAL: return "=";
    case TokenType::LPAREN: return "("; case TokenType::RPAREN: return ")";
    case TokenType::COLON: return ":"; case TokenType::RETURN: return "RETURN";
    case TokenType::PRINT: return "PRINT"; case TokenType::TRY: return "TRY";
    case TokenType::EXCEPT: return "EXCEPT"; case TokenType::OR: return "OR";
    case TokenType::NOT: return "NOT"; case TokenType::NUMBER: return "NUM";
    case TokenType::STRING: return "STR"; default: return "TOK";
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

    QSplitter *mainSplitter = new QSplitter(Qt::Vertical, this);
    mainSplitter->setStyleSheet("QSplitter::handle { background-color: " + COLOR_BORDER + "; }");
    mainSplitter->setHandleWidth(2);

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

    mainSplitter->addWidget(topRowWidget);

    QTabWidget *tabWidget = new QTabWidget();
    tabWidget->setStyleSheet(QString("QTabWidget::pane { background-color: %1; border: none; border-top: 1px solid %2; } QTabBar::tab { background-color: %3; color: %4; padding: 10px 20px; border: none; } QTabBar::tab:selected { background-color: %1; color: %5; border-top: 2px solid %5; } QTabBar::tab:hover { color: %6; }").arg(COLOR_BACKGROUND_DARK, COLOR_BORDER, COLOR_BACKGROUND_MID, COLOR_TEXT_SECONDARY, COLOR_ACCENT_RED, COLOR_TEXT_PRIMARY));

    automatonScene = new QGraphicsScene(this);
    automatonScene->setSceneRect(0, 0, 2000, 2000);
    ZoomableView *automatonView = new ZoomableView(automatonScene, this);
    automatonView->setStyleSheet(QString("background-color: %1; border: none;").arg(COLOR_BACKGROUND_DARK));
    automatonView->setRenderHint(QPainter::Antialiasing);
    automatonView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    automatonView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);


    QTextEdit *tokensEdit = new QTextEdit();
    tokensEdit->setObjectName("tokensEdit");
    tokensEdit->setStyleSheet(QString("background-color: %1; color: %2; font-family: 'Courier New'; font-size: 13px; border: none; padding: 10px;").arg(COLOR_BACKGROUND_DARK, COLOR_ACCENT_GREEN));
    tokensEdit->setReadOnly(true);

    QTextEdit *grammarEdit = new QTextEdit();
    grammarEdit->setObjectName("grammarEdit");
    grammarEdit->setReadOnly(true);
    grammarEdit->setStyleSheet(QString("background-color: %1; color: %2; font-family: 'Courier New'; font-size: 13px; border: none; padding: 10px;").arg(COLOR_BACKGROUND_DARK, "#87CEEB"));
    grammarEdit->setPlainText(R"(
Program      -> Statement Program | ε

Statement    -> FunctionDef | IfStmt | WhileStmt | ReturnStmt | PrintStmt | Assignment | Expression

FunctionDef  -> 'def' ID '(' Params ')' ':' Block
Params       -> ID ParamTail | ε
ParamTail    -> ',' ID ParamTail | ε

IfStmt       -> 'if' Expression ':' Block ElseClause
ElseClause   -> 'elif' Expression ':' Block ElseClause
ElseClause   -> 'else' ':' Block
ElseClause   -> ε

WhileStmt    -> 'while' Expression ':' Block

ReturnStmt   -> 'return' Expression | 'return'

PrintStmt    -> 'print' Expression

Assignment   -> ID '=' Expression

Expression   -> LogicalOr
LogicalOr    -> Comparison LogicalOr'
LogicalOr'   -> 'or' Comparison LogicalOr' | ε

Comparison   -> Term Comparison'
Comparison'  -> CompOp Term Comparison' | ε
CompOp       -> '==' | '>' | '<='

Term         -> Factor Term'
Term'        -> AddOp Factor Term' | ε
AddOp        -> '+' | '-'

Factor       -> Unary Factor'
Factor'      -> MulOp Unary Factor' | ε
MulOp        -> '*' | '/'

Unary        -> 'not' Unary | Primary

Primary      -> NUMBER | STRING | ID | FuncCall | 'True' | 'False' | 'None' | '(' Expression ')'

FuncCall     -> ID '(' Arguments ')'
Arguments    -> Expression ArgTail | ε
ArgTail      -> ',' Expression ArgTail | ε
)");

    treeScene = new QGraphicsScene(this);
    treeScene->setSceneRect(0, 0, 3000, 3000);
    ZoomableView *treeView = new ZoomableView(treeScene, this);
    treeView->setStyleSheet(QString("background-color: %1; border: none;").arg(COLOR_BACKGROUND_DARK));
    treeView->setRenderHint(QPainter::Antialiasing);
    treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    treeView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    tabWidget->addTab(automatonView, "Automaton");
    tabWidget->addTab(tokensEdit, "Tokens");
    tabWidget->addTab(grammarEdit, "Grammar");
    tabWidget->addTab(treeView, "Parse Tree");

    mainSplitter->addWidget(tabWidget);
    mainSplitter->setSizes({400, 400});

    mainLayout->addWidget(mainSplitter);

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
