#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "lexer.h"
#include "parser.h"
#include "translator.h"
#include "semantic_analyzer.h"
#include "types.h"

#include <stdexcept>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QGraphicsView>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QSplitter>
#include <QWheelEvent>
#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <QScrollBar>
#include <QRegularExpression>
#include <map>
#include <set>
#include <cmath>

QElapsedTimer executionTimer;
using namespace std;


class ZoomableView : public QGraphicsView {
public:
    explicit ZoomableView(QGraphicsScene *scene, QWidget *parent = nullptr)
        : QGraphicsView(scene, parent) {

        // Better Scrolling / Panning Behavior
        setDragMode(QGraphicsView::ScrollHandDrag);
        setRenderHint(QPainter::Antialiasing);
        setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

        // Ensure scrollbars appear when needed
        setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        // Anchors for smooth zooming
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    }

protected:
    void wheelEvent(QWheelEvent *event) override {
        if (event->modifiers() & Qt::ControlModifier) {
            // Smooth Zoom Logic
            const double zoomFactor = 1.15;
            if (event->angleDelta().y() > 0) {
                scale(zoomFactor, zoomFactor);
            } else {
                scale(1.0 / zoomFactor, 1.0 / zoomFactor);
            }
            event->accept();
        } else {
            // Standard scrolling if CTRL is not pressed
            QGraphicsView::wheelEvent(event);
        }
    }
};

// Global helper for Design Text
QString getDesignDocumentText() {
    return R"(
==============================================================================
FULL ATTRIBUTE GRAMMAR SPECIFICATION
==============================================================================
This document defines the static semantics enforced by the compiler.
S = Statement, E = Expression, T = Type

[ 1. Literals & Base Types ]
   E -> integer_literal    => E.type = INTEGER
   E -> float_literal      => E.type = FLOAT
   E -> string_literal     => E.type = STRING
   E -> 'True' | 'False'   => E.type = BOOLEAN
   E -> 'None'             => E.type = NONE

[ 2. Variable Declarations & Assignments ]
   Production: ID = E
   Action:
       1. lookup(ID) in SymbolTable
       2. If exists:
            if (ID.type == FLOAT && E.type == INTEGER): ALLOW (Promotion)
            else if (ID.type != E.type): ERROR("Type Mismatch")
       3. Else:
            SymbolTable.define(ID, E.type)
       4. ID.type = E.type

[ 3. Binary Operations (Arithmetic) ]
   Production: E -> E1 op E2   where op in { +, -, *, / }
   Rules:
       1. If (E1.type == STRING || E2.type == STRING):
            if (op == +): E.type = STRING  (Concatenation)
            else: ERROR("Cannot perform -, *, / on Strings")
       2. Else if (E1.type == FLOAT || E2.type == FLOAT):
            E.type = FLOAT
       3. Else:
            E.type = INTEGER

[ 4. Binary Operations (Logic & Comparison) ]
   Production: E -> E1 op E2
   Ops: { >, <, >=, <=, ==, != } OR { and, or }
   Rule:
       E.type = BOOLEAN

[ 5. Unary Operations ]
   Production: E -> op E1
   Rules:
       1. If op == 'not': E.type = BOOLEAN
       2. If op == '-':   E.type = E1.type

[ 6. For Loop Semantics ]
   Case A: Range Loop
       S -> for ID in range(start, stop, step)
       Check: start.type == INTEGER
       Check: stop.type == INTEGER
       Action:
           Scope.enter()
           SymbolTable.define(ID, INTEGER)
           Visit(Body)
           Scope.exit()

   Case B: Generic Loop
       S -> for ID in Iterable
       Action:
           If (Iterable.type == STRING): SymbolTable.define(ID, STRING)
           Else: SymbolTable.define(ID, UNDEFINED)

[ 7. Function Definitions ]
   Production: def ID(Params...): Block
   Action:
       1. Check if ID defined globally. If yes -> ERROR.
       2. SymbolTable.define(ID, FUNCTION)
       3. Scope.enter()
       4. For p in Params: SymbolTable.define(p, INTEGER) (Default)
       5. Visit(Block)
       6. Scope.exit()

[ 8. Return Statements ]
   Production: return E
   Action:
       1. Check if inside Function. If no -> ERROR.
       2. current_func.return_type = E.type
       3. If (current_func has previous returns):
            Check (previous_return_type == E.type)
            If mismatch -> ERROR("Inconsistent return types")

[ 9. Function Calls ]
   Production: ID(Args...)
   Action:
       1. func = lookup(ID)
       2. If !func -> ERROR("Function not defined")
       3. Visit all Args (to resolve their types)
       4. E.type = func.return_type
)";
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    // Initialize Profiler Processes
    compilerProcess = new QProcess(this);
    runnerProcess = new QProcess(this);

    // Connect Process Signals for the Profiler Tab
    connect(compilerProcess, &QProcess::finished, this, &MainWindow::onCompilationFinished);
    connect(runnerProcess, &QProcess::finished, this, &MainWindow::onExecutionFinished);

    setupUI();

    // Attach Highlighter to the Editor
    highlighter = new ErrorHighlighter(sourceCodeEdit->document());

    // Setup Live Check Timer (Debouncing)
    liveCheckTimer = new QTimer(this);
    liveCheckTimer->setSingleShot(true);
    connect(liveCheckTimer, &QTimer::timeout, this, &MainWindow::liveCheck);

    // Trigger check when text changes
    connect(sourceCodeEdit, &CodeEditor::textChanged, [this]() {
        // Wait 600ms after user stops typing to avoid lag
        liveCheckTimer->start(600);
    });

    // Default Code Example
    const QString pythonCode = R"(def calculate_sum(limit):
    total = 0.0
    # Loop from 1 to limit
    for i in range(1, limit, 1):
        total = total + i
    return total

x = 10
result = calculate_sum(x)

if result > 40.5:
    print("High Sum")
else:
    print("Low Sum")
)";
    sourceCodeEdit->setPlainText(pythonCode);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::liveCheck() {
    QString sourceCode = sourceCodeEdit->toPlainText();
    if (sourceCode.trimmed().isEmpty()) return;

    QLabel* statusLabel = findChild<QLabel*>("statusLabel");

    try {
        // 1. Lexer
        Lexer lexer(sourceCode);
        vector<Token> tokens = lexer.tokenize();

        // 2. Parser
        Parser parser(tokens);
        unique_ptr<ProgramNode> astRoot = parser.parse();

        if (astRoot) {
            // 3. Semantic Analysis
            SemanticAnalyzer analyzer;
            analyzer.analyze(astRoot.get());

            // If we get here, no errors found
            highlighter->clearError();
            sourceCodeEdit->clearError();
            statusLabel->setStyleSheet("background-color: #276749; color: white; padding: 8px;");
            statusLabel->setText("Status: No errors detected.");
        }
    } catch (const exception& e) {
        // Parse error message for line number
        QString errorMsg = QString(e.what());
        int line = -1;

        // Attempt to find "line X" in the error string
        QRegularExpression re("line\\s+(\\d+)");
        QRegularExpressionMatch match = re.match(errorMsg);
        if (match.hasMatch()) {
            line = match.captured(1).toInt();
        }

        if (line != -1) {
            highlighter->setErrorLine(line);
            sourceCodeEdit->setError(line, errorMsg);
        }

        statusLabel->setStyleSheet("background-color: #9b2c2c; color: white; padding: 8px; font-weight: bold;");
        statusLabel->setText(QString("Live Error: ") + errorMsg);
    }
}

void MainWindow::onAnalyzeClicked() {
    QString sourceCode = sourceCodeEdit->toPlainText();
    QLabel* statusLabel = findChild<QLabel*>("statusLabel");

    // Clear previous results
    automatonScene->clear();
    treeScene->clear();
    tokensEdit->clear();
    targetCodeEdit->clear();
    profilerEdit->clear();

    try {
        // 1. Lexer
        Lexer lexer(sourceCode);
        vector<Token> tokens = lexer.tokenize();
        QString tokensString;
        for (const auto& token : tokens) {
            tokensString += QString("Line %1: Type: %2, Value: '%3'\n")
            .arg(token.line)
                .arg(getTokenName(token.type))
                .arg(token.value);
        }
        tokensEdit->setPlainText(tokensString);

        // 2. Parser
        Parser parser(tokens);
        unique_ptr<ProgramNode> astRoot = parser.parse();

        if (astRoot) {
            // 3. Semantic Analysis
            SemanticAnalyzer analyzer;
            analyzer.analyze(astRoot.get());

            // 4. Visualizations
            drawTrueAutomaton(parser);
            drawParseTree(astRoot.get(), QPointF(treeScene->width() / 2, 50));

            // 5. Translation
            Translator translator(analyzer.getSymbolTable());
            QString cppCode = translator.translate(astRoot.get());
            targetCodeEdit->setPlainText(cppCode);

            statusLabel->setText("Success: Code analyzed and translated successfully.");
            highlighter->clearError(); // Clear highlights if button clicked and succeeds

            // 6. Profiler Execution
            // Save generated code to temp file
            QFile tempFile("temp_profiler.cpp");
            if (tempFile.open(QIODevice::WriteOnly)) {
                QTextStream out(&tempFile);
                out << cppCode;
                tempFile.close();

                profilerEdit->setPlainText("Compiling C++ Output...");
                // Requires g++ in system PATH
                compilerProcess->start("g++", QStringList() << "temp_profiler.cpp" << "-o" << "temp_profiler_app");
            } else {
                profilerEdit->setPlainText("Error: Could not save temp file for profiling.");
            }

        } else {
            statusLabel->setText("Error: Failed to generate AST.");
        }
    } catch (const SemanticError& e) {
        QMessageBox::critical(this, "Semantic Error", e.what());
        statusLabel->setText("Error: Semantic analysis failed.");
        liveCheck(); // Trigger highlights
    } catch (const runtime_error& e) {
        QMessageBox::critical(this, "Analysis Error", e.what());
        statusLabel->setText("Error: Analysis failed.");
        liveCheck(); // Trigger highlights
    }
}

// ============================================================================
// Profiler Slots
// ============================================================================

void MainWindow::onCompilationFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (exitStatus == QProcess::CrashExit || exitCode != 0) {
        profilerEdit->setPlainText("Compilation Failed.\n" + compilerProcess->readAllStandardError());
    } else {
        profilerEdit->setPlainText("Compilation Successful. Running program...\n");
        executionTimer.start();
        runnerProcess->start("./temp_profiler_app");
    }
}

void MainWindow::onExecutionFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    // --- STOP TIMER ---
    qint64 duration = executionTimer.nsecsElapsed(); // Nanoseconds for high precision
    double seconds = duration / 1000000000.0;
    if (exitCode != 0) {
        profilerEdit->append("Runtime Error.\n" + runnerProcess->readAllStandardError());
    } else {
        QString output = runnerProcess->readAllStandardOutput();
        profilerEdit->append("--------------------------------\n");
        profilerEdit->append("PROGRAM OUTPUT:\n");
        profilerEdit->append("--------------------------------\n");
        profilerEdit->append(output);
        profilerEdit->append("--------------------------------\n");
        profilerEdit->append("Process exited with code 0.");
        QString stats = QString("Execution Finished.\nExit Code: 0\nTime Taken: %1 seconds").arg(seconds, 0, 'f', 6);
        profilerEdit->append(stats);
    }
}

// ============================================================================
// Visualization Logic
// ============================================================================

void MainWindow::drawParseTree(const ASTNode* node, QPointF pos, QPointF parentPos, int depth) {
    if (!node) return;

    const QColor NODE_COLOR(38, 115, 83), LINE_COLOR(160, 147, 147), TEXT_COLOR(247, 250, 252);
    const QBrush NODE_BRUSH(QColor(26, 58, 42));
    QPen nodePen(NODE_COLOR, 2), linePen(LINE_COLOR, 1.5);

    QGraphicsEllipseItem *ellipse = treeScene->addEllipse(0, 0, 160, 50, nodePen, NODE_BRUSH);
    ellipse->setPos(pos - QPointF(80, 25));

    // Label Logic: Show Name AND Type if determined
    QString labelText = node->getNodeName();
    if (node->determined_type != DataType::UNDEFINED &&
        node->determined_type != DataType::NONE &&
        !dynamic_cast<const ProgramNode*>(node) &&
        !dynamic_cast<const BlockNode*>(node)) {
        labelText += "\n[" + DataTypeToString(node->determined_type) + "]";
    }

    QGraphicsTextItem *textItem = treeScene->addText(labelText);
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
    // --- FOR LOOP VISUALIZATION ---
    else if (auto p = dynamic_cast<const ForNode*>(node)) {
        children.push_back(p->iterator.get());
        children.push_back(p->start.get());
        children.push_back(p->stop.get());
        children.push_back(p->step.get());
        children.push_back(p->body.get());
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
    automatonScene->setBackgroundBrush(QBrush(QColor(40, 40, 40)));

    // --- COLORS ---
    const QColor STATE_COLOR(229, 62, 62);
    const QColor TRANSITION_COLOR(255, 255, 0);
    const QColor TEXT_COLOR(255, 255, 255);
    const QColor HUB_COLOR(0, 120, 215);
    const QColor ACTIVE_HIGHLIGHT(50, 205, 50);

    QPen statePen(STATE_COLOR, 3);
    QPen hubPen(HUB_COLOR, 4);
    QPen transitionPen(TRANSITION_COLOR, 2);
    transitionPen.setCosmetic(true);

    // --- 1. DYNAMIC FILTERING (Identify used states) ---
    vector<pair<ParserState, Token>> history = parser.getStateHistory();
    set<ParserState> activeStates;
    for (const auto& item : history) {
        activeStates.insert(item.first);
    }
    // Always keep structural anchors active
    activeStates.insert(ParserState::START);
    activeStates.insert(ParserState::EXPECT_STATEMENT);
    activeStates.insert(ParserState::END_STATEMENT);

    // --- 2. LAYOUT DEFINITION (With "Bring Down" Offset) ---
    map<ParserState, QPointF> layoutMap;

    int w = 350; // Width between columns
    int h = 180; // Height between rows

    // OFFSETS: Adjust these to move the whole graph
    int startX = 100;
    int startY = 250; // <--- MOVED DOWN (Was 0)

    int x = startX;

    // COL 1: Main Flow (Far Left)
    layoutMap[ParserState::START]            = QPointF(0, startY);
    layoutMap[ParserState::EXPECT_STATEMENT] = QPointF(w/2, startY + h * 1.5); // The Hub
    layoutMap[ParserState::END_STATEMENT]    = QPointF(0, startY + h * 3);

    // COL 2: Logic (If/Try)
    x += w;
    layoutMap[ParserState::IN_IF_CONDITION]  = QPointF(x, startY);
    layoutMap[ParserState::IN_IF_BODY]       = QPointF(x, startY + h);
    layoutMap[ParserState::IN_TRY_BLOCK]     = QPointF(x, startY + h * 2);
    layoutMap[ParserState::IN_EXCEPT_BLOCK]  = QPointF(x, startY + h * 3);

    // COL 3: Functions
    x += w;
    layoutMap[ParserState::IN_FUNCTION_DEF]    = QPointF(x, startY);
    layoutMap[ParserState::IN_FUNCTION_PARAMS] = QPointF(x, startY + h);
    layoutMap[ParserState::IN_FUNCTION_BODY]   = QPointF(x, startY + h * 2);
    layoutMap[ParserState::IN_FUNCTION_CALL]   = QPointF(x, startY + h * 3);

    // COL 4: Expressions (Far Right)
    x += w;
    layoutMap[ParserState::IN_ASSIGNMENT]    = QPointF(x, startY);
    layoutMap[ParserState::IN_EXPRESSION]    = QPointF(x, startY + h);
    layoutMap[ParserState::IN_TERM]          = QPointF(x, startY + h * 2);
    layoutMap[ParserState::IN_FACTOR]        = QPointF(x, startY + h * 3);

    // --- 3. DRAW NODES ---
    for (auto const& [state, pos] : layoutMap) {
        // Skip unused states
        if (activeStates.find(state) == activeStates.end()) continue;

        bool isHub = (state == ParserState::EXPECT_STATEMENT);
        int size = isHub ? 80 : 60;

        QGraphicsEllipseItem* node = automatonScene->addEllipse(-size/2, -size/2, size, size,
                                                                isHub ? hubPen : statePen,
                                                                QBrush(QColor(30,30,30)));
        node->setPos(pos);

        // Label processing
        QString name = getStateName(state);
        name.replace("IN_", "").replace("EXPECT_", "").replace("STATEMENT", "STMT");

        QGraphicsTextItem* label = automatonScene->addText(name);
        label->setDefaultTextColor(TEXT_COLOR);
        label->setFont(QFont("Arial", 8, QFont::Bold));

        // Center text on node
        QRectF r = label->boundingRect();
        label->setPos(pos.x() - r.width()/2, pos.y() - r.height()/2);
    }

    // --- 4. DRAW DYNAMIC TRANSITIONS ---
    struct TransKey { ParserState a, b; TokenType t;
        bool operator<(const TransKey& o) const { return std::tie(a, b, t) < std::tie(o.a, o.b, o.t); }
    };
    std::set<TransKey> drawn;

    for (size_t i = 1; i < history.size(); ++i) {
        ParserState from = history[i-1].first;
        ParserState to = history[i].first;
        Token tok = history[i].second;

        // Skip transitions involving filtered states
        if (layoutMap.find(from) == layoutMap.end() || layoutMap.find(to) == layoutMap.end()) continue;
        if (activeStates.find(from) == activeStates.end() || activeStates.find(to) == activeStates.end()) continue;

        // Deduplication
        if (drawn.count({from, to, tok.type})) continue;
        drawn.insert({from, to, tok.type});

        QPointF p1 = layoutMap[from];
        QPointF p2 = layoutMap[to];

        QPainterPath path;
        path.moveTo(p1);

        if (from == to) {
            // Self Loop (Arc above)
            path.cubicTo(p1 + QPointF(-40, -60), p1 + QPointF(40, -60), p1);
        } else {
            // Bezier Curve Logic
            QPointF mid = (p1 + p2) / 2;
            double dx = p2.x() - p1.x();
            double dy = p2.y() - p1.y();

            // Curve Factor
            double offsetFactor = 0.2;
            // Flip curve based on direction to minimize crossing
            if (p1.x() > p2.x()) offsetFactor = -0.2;

            QPointF control = mid + QPointF(-dy * offsetFactor, dx * offsetFactor);
            path.quadTo(control, p2);

            // Arrowhead
            double angle = std::atan2(p2.y() - control.y(), p2.x() - control.x());
            QPointF arrowP1 = p2 - QPointF(cos(angle + M_PI/6) * 15, sin(angle + M_PI/6) * 15);
            QPointF arrowP2 = p2 - QPointF(cos(angle - M_PI/6) * 15, sin(angle - M_PI/6) * 15);
            QPolygonF arrowHead;
            arrowHead << p2 << arrowP1 << arrowP2;
            automatonScene->addPolygon(arrowHead, QPen(Qt::NoPen), QBrush(TRANSITION_COLOR));

            // Token Label on Line
            QPointF labelPos = (mid + control) / 2;
            QGraphicsTextItem* tag = automatonScene->addText(getTokenName(tok.type));
            tag->setDefaultTextColor(TRANSITION_COLOR);
            tag->setFont(QFont("Arial", 7));
            QRectF tr = tag->boundingRect();
            tag->setPos(labelPos.x() - tr.width()/2, labelPos.y() - tr.height()/2);
        }
        automatonScene->addPath(path, transitionPen);
    }

    // --- 5. HIGHLIGHT CURRENT STATE ---
    if (!history.empty()) {
        ParserState last = history.back().first;
        if (layoutMap.count(last) && activeStates.count(last)) {
            QGraphicsEllipseItem* cur = automatonScene->addEllipse(-40, -40, 80, 80, QPen(ACTIVE_HIGHLIGHT, 4), Qt::NoBrush);
            cur->setPos(layoutMap[last]);
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
    case TokenType::DEF: return "DEF"; case TokenType::IF: return "IF";
    case TokenType::WHILE: return "WHILE"; case TokenType::ELSE: return "ELSE";
    case TokenType::ELIF: return "ELIF"; case TokenType::FOR: return "FOR";
    case TokenType::IN: return "IN";
    case TokenType::IDENTIFIER: return "ID"; case TokenType::EQUAL: return "=";
    case TokenType::LPAREN: return "("; case TokenType::RPAREN: return ")";
    case TokenType::COLON: return ":"; case TokenType::RETURN: return "RETURN";
    case TokenType::PRINT: return "PRINT"; case TokenType::TRY: return "TRY";
    case TokenType::EXCEPT: return "EXCEPT"; case TokenType::OR: return "OR";
    case TokenType::NOT: return "NOT"; case TokenType::NUMBER: return "NUM";
    case TokenType::STRING: return "STR"; default: return "TOK";
    }
}

// ============================================================================
// UI Setup (Layouts and Tab Order)
// ============================================================================

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

    // CHANGED: Use custom CodeEditor
    sourceCodeEdit = new CodeEditor();
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
    targetCodeEdit = new QTextEdit();
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

    // --- TAB 1: Automaton ---
    automatonScene = new QGraphicsScene(this);
    automatonScene->setSceneRect(-2500, -2500, 5000, 5000);
    ZoomableView *automatonView = new ZoomableView(automatonScene, this);
    automatonView->setStyleSheet(QString("background-color: %1; border: none;").arg(COLOR_BACKGROUND_DARK));
    automatonView->setRenderHint(QPainter::Antialiasing);
    automatonView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    automatonView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    tabWidget->addTab(automatonView, "Automaton");

    // --- TAB 2: Tokens ---
    tokensEdit = new QTextEdit();
    tokensEdit->setObjectName("tokensEdit");
    tokensEdit->setStyleSheet(QString("background-color: %1; color: %2; font-family: 'Courier New'; font-size: 13px; border: none; padding: 10px;").arg(COLOR_BACKGROUND_DARK, COLOR_ACCENT_GREEN));
    tokensEdit->setReadOnly(true);
    tabWidget->addTab(tokensEdit, "Tokens");

    // --- TAB 3: Grammar (Detailed) ---
    QTextEdit *grammarEdit = new QTextEdit();
    grammarEdit->setObjectName("grammarEdit");
    grammarEdit->setReadOnly(true);
    grammarEdit->setStyleSheet(QString("background-color: %1; color: %2; font-family: 'Courier New'; font-size: 13px; border: none; padding: 10px;").arg(COLOR_BACKGROUND_DARK, "#87CEEB"));
    grammarEdit->setPlainText(R"(
Program      -> Statement Program | ε
Statement    -> FunctionDef | IfStmt | WhileStmt | ForStmt | ReturnStmt | PrintStmt | Assignment | Expression

FunctionDef  -> 'def' ID '(' Params ')' ':' Block
Params       -> ID ParamTail | ε
ParamTail    -> ',' ID ParamTail | ε

ForStmt      -> 'for' ID 'in' LoopSource
LoopSource   -> 'range' '(' Expression, Expression, Expression ')' ':' Block
LoopSource   -> Expression ':' Block  (Generic Iterable)

IfStmt       -> 'if' Expression ':' Block ElseClause
ElseClause   -> 'elif' Expression ':' Block ElseClause
ElseClause   -> 'else' ':' Block
ElseClause   -> ε

WhileStmt    -> 'while' Expression ':' Block
ReturnStmt   -> 'return' Expression | 'return'
PrintStmt    -> 'print' Expression
Assignment   -> ID '=' Expression
Assignment   -> ID ('+=' | '-=' | '*=' | '/=') Expression

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

Unary        -> 'not' Unary | '-' Unary | Primary
Primary      -> NUMBER | STRING | ID | FuncCall | 'True' | 'False' | 'None' | '(' Expression ')'

FuncCall     -> ID '(' Arguments ')'
Arguments    -> Expression ArgTail | ε
ArgTail      -> ',' Expression ArgTail | ε
)");
    tabWidget->addTab(grammarEdit, "Grammar");

    // --- TAB 4: Parse Tree ---
    treeScene = new QGraphicsScene(this);
    treeScene->setSceneRect(0, 0, 3000, 3000);
    ZoomableView *treeView = new ZoomableView(treeScene, this);
    treeView->setStyleSheet(QString("background-color: %1; border: none;").arg(COLOR_BACKGROUND_DARK));
    treeView->setRenderHint(QPainter::Antialiasing);
    treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    treeView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    tabWidget->addTab(treeView, "Parse Tree");

    // --- TAB 5: Formal Design (ONLY Semantic Logic) ---
    // Uses getDesignDocumentText() which returns Plain Text, no HTML.
    designEdit = new QTextEdit();
    designEdit->setReadOnly(true);
    designEdit->setStyleSheet(QString("background-color: %1; color: %2; font-family: 'Courier New'; font-size: 13px; border: none; padding: 15px;").arg(COLOR_BACKGROUND_DARK, COLOR_TEXT_PRIMARY));
    designEdit->setPlainText(getDesignDocumentText());
    tabWidget->addTab(designEdit, "Formal Design");

    // --- TAB 6: Profiler ---
    profilerEdit = new QTextEdit();
    profilerEdit->setReadOnly(true);
    profilerEdit->setStyleSheet(QString("background-color: %1; color: %2; font-family: 'Courier New'; font-size: 13px; border: none; padding: 10px;").arg(COLOR_BACKGROUND_DARK, "#63b3ed"));
    tabWidget->addTab(profilerEdit, "Profiler");

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
