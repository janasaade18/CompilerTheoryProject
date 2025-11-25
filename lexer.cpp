#include "lexer.h"
#include <QChar>
#include <stdexcept>
#include <string>

using namespace std;

Lexer::Lexer(const QString& source) : m_source(source) {
    m_indent_stack.push(0);
}

vector<Token> Lexer::tokenize() {
    vector<Token> tokens;
    bool at_start_of_line = true; // Flag to track if we need to check indentation

    while (m_pos < m_source.length()) {
        // --- 1. Indentation Handling  ---
        // We only check indentation at the very beginning of a line.
        if (at_start_of_line) {
            at_start_of_line = false; // We are now inside the line
            int current_indent = getCurrentIndent();

            // Ignore blank lines (pure whitespace or just newlines)
            if (m_pos >= m_source.length() || currentChar() == '\n') {
                if (currentChar() == '\n') {
                    m_line++;
                    advance();
                    at_start_of_line = true; // Next char starts a new line
                }
                continue;
            }

            // Case A: Indentation Increased (Opening a Block)
            // Example: 'if x:' -> next line has more spaces
            if (current_indent > m_indent_stack.top()) {
                m_indent_stack.push(current_indent);
                tokens.push_back({TokenType::INDENT, "INDENT", m_line});
            }
            // Case B: Indentation Decreased (Closing Block(s))
            // Example: End of 'if' block, going back to main scope
            else if (current_indent < m_indent_stack.top()) {
                // We might be closing multiple nested blocks at once, so we loop/pop
                while (current_indent < m_indent_stack.top() && m_indent_stack.size() > 1) {
                    m_indent_stack.pop();
                    tokens.push_back({TokenType::DEDENT, "DEDENT", m_line});
                }

                // Validation: The new indent level MUST match a previous level in the stack
                if (current_indent != m_indent_stack.top()) {
                    throw runtime_error("Indentation error at line " + to_string(m_line));
                }
            }
        }

        // --- 2. Standard Tokenization ---
        skipWhitespace(); // Ignore spaces between tokens (e.g., x = 5)
        if (m_pos >= m_source.length()) break;

        // Handle Newlines (Reset the line flag)
        if (currentChar() == '\n') {
            at_start_of_line = true;
            m_line++;
            advance();
            continue;
        }

        // Handle Comments (Ignore everything until the end of the line)
        if (currentChar() == '#') {
            skipComment();
            continue;
        }

        // Identify the next token (Identifier, Number, Symbol, etc.)
        tokens.push_back(getNextTokenFromSource());
    }


    // Implicitly close any blocks that are still open at EOF
    while (m_indent_stack.top() > 0) {
        m_indent_stack.pop();
        tokens.push_back({TokenType::DEDENT, "DEDENT", m_line});
    }

    tokens.push_back({TokenType::END_OF_FILE, "EOF", m_line});
    return tokens;
}

Token Lexer::getNextTokenFromSource() {
    if (m_pos >= m_source.length()) return {TokenType::END_OF_FILE, "", m_line};

    QChar current = currentChar();
    if (current.isLetter() || current == '_') return identifier();
    if (current.isDigit()) return number();
    if (current == '"' || current == '\'') return string();

    switch (current.toLatin1()) {
    case '=':
        if (peek() == '=') {
            advance(); advance();
            return {TokenType::DOUBLE_EQUAL, "==", m_line};
        }
        advance();
        return {TokenType::EQUAL, "=", m_line};

    case '>':
        if (peek() == '=') {
            advance(); advance();
            return {TokenType::GREATER, ">=", m_line};
        }
        advance();
        return {TokenType::GREATER, ">", m_line};

    case '<':
        if (peek() == '=') {
            advance(); advance();
            return {TokenType::LESS_EQUAL, "<=", m_line};
        }
        advance();
        return {TokenType::LESS_EQUAL, "<", m_line};

    case '+':
        advance();
        return {TokenType::PLUS, "+", m_line};

    case '-':
        advance();
        return {TokenType::MINUS, "-", m_line};

    case '*':
        advance();
        return {TokenType::STAR, "*", m_line};

    case '/':
        advance();
        return {TokenType::SLASH, "/", m_line};

    case '(':
        advance();
        return {TokenType::LPAREN, "(", m_line};

    case ')':
        advance();
        return {TokenType::RPAREN, ")", m_line};

    case '{':
        advance();
        return {TokenType::LBRACE, "{", m_line};

    case '}':
        advance();
        return {TokenType::RBRACE, "}", m_line};

    case ':':
        advance();
        return {TokenType::COLON, ":", m_line};

    case ',':
        advance();
        return {TokenType::COMMA, ",", m_line};

    case ';':
        advance();
        return {TokenType::SEMICOLON, ";", m_line};

    case '.':
        advance();
        return {TokenType::DOT, ".", m_line};

    case '#':
        skipComment();
        return getNextTokenFromSource();

    default:
        advance();
        return {TokenType::ILLEGAL, QString(current), m_line};
    }
}

int Lexer::getCurrentIndent() {
    int indent = 0;
    while (m_pos < m_source.length() && (currentChar() == ' ' || currentChar() == '\t')) {
        if (currentChar() == ' ') {
            indent++;
        } else if (currentChar() == '\t') {
            indent += 4;
        }
        advance();
    }
    return indent;
}

void Lexer::skipComment() {
    while (m_pos < m_source.length() && currentChar() != '\n') {
        advance();
    }
}

QChar Lexer::currentChar() {
    if (m_pos >= m_source.length()) return '\0';
    return m_source[m_pos];
}

QChar Lexer::peek() {
    if (m_pos + 1 >= m_source.length()) return '\0';
    return m_source[m_pos + 1];
}

void Lexer::advance() {
    if (m_pos < m_source.length()) m_pos++;
}

void Lexer::skipWhitespace() {
    while (m_pos < m_source.length() && (currentChar() == ' ' || currentChar() == '\t' || currentChar() == '\r')) {
        advance();
    }
}


Token Lexer::number() {
    QString result;
    bool dot_seen = false; // Track if we've seen a decimal point

    while (m_pos < m_source.length()) {
        QChar c = currentChar();

        if (c.isDigit()) {
            result += c;
            advance();
        } else if (c == '.') {
            if (dot_seen) break; // We already saw a dot, so 1.2.3 -> stop at second dot
            dot_seen = true;
            result += c;
            advance();
        } else {
            break;
        }
    }

    // Note: We still return TokenType::NUMBER.
    // We will distinguish Int vs Float in the Semantic Analyzer based on the presence of '.'
    return {TokenType::NUMBER, result, m_line};
}

Token Lexer::string() {
    QString result;
    QChar quote = currentChar();
    advance();

    while (m_pos < m_source.length() && currentChar() != quote) {
        if (currentChar() == '\\') {
            advance();
            if (m_pos < m_source.length()) {
                result += currentChar();
                advance();
            }
        } else {
            result += currentChar();
            advance();
        }
    }

    if (currentChar() == quote) {
        advance();
    }

    return {TokenType::STRING, result, m_line};
}

Token Lexer::identifier() {
    QString result;
    while (m_pos < m_source.length() && (currentChar().isLetterOrNumber() || currentChar() == '_')) {
        result += currentChar();
        advance();
    }

    if (result == "def") return {TokenType::DEF, result, m_line};
    if (result == "if") return {TokenType::IF, result, m_line};
    if (result == "while") return {TokenType::WHILE, result, m_line};
    if (result == "else") return {TokenType::ELSE, result, m_line};
    if (result == "elif") return {TokenType::ELIF, result, m_line};
    if (result == "return") return {TokenType::RETURN, result, m_line};
    if (result == "print") return {TokenType::PRINT, result, m_line};
    if (result == "not") return {TokenType::NOT, result, m_line};
    if (result == "or") return {TokenType::OR, result, m_line};
    if (result == "None") return {TokenType::NONE, result, m_line};
    if (result == "True") return {TokenType::TRUE, result, m_line};
    if (result == "False") return {TokenType::FALSE, result, m_line};
    if (result == "try") return {TokenType::TRY, result, m_line};
    if (result == "except") return {TokenType::EXCEPT, result, m_line};
    if (result == "for")   return {TokenType::FOR, result, m_line};
    if (result == "in")    return {TokenType::IN, result, m_line};

    return {TokenType::IDENTIFIER, result, m_line};
}
