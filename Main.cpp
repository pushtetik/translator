#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <stack>
#include <unordered_map>
using namespace std;
char CH; //текущий символ
string A = ""; //строка для накопления лексемы
//таблица ключевых слов(1)
vector<pair<int, string>> keywordTable = {
    {1, "IF"}, {2, "THEN"}, {3, "ELSE"}, {4, "ELSEIF"}, {5, "END"},
    {6, "AND"}, {7, "OR"}, {8, "PRINT"}, {9, "INPUT"}, {10, "MOD"},
    {11, "DIM"}, {12, "AS"}, {13, "INTEGER"}
};
//таблица разделителей(2)
vector<pair<int, string>> separatorTable = {
    {1, "="}, {2, "/"}, {3, "+"}, {4, "-"}, {5, "*"},
    {6, "("}, {7, ")"}, {8, "^"}, {9, ","}, {10, "<"}, {11, ">"}, {12, "\""}, {13, "\n"}
};
//таблица констант(3)
vector<pair<int, string>> constantTable;
//таблица символьных имен(4)
struct SymbolEntry {
    int id;
    string name;
    int used;
};
vector<SymbolEntry> symbolTable;
//таблица ошибок(5)
struct ErrorEntry {
    int num;
    int line;
    int col;
    char sym;
};
vector<ErrorEntry> errorTable;
//токены
vector<pair<int, int>> tokenSequence;
//Функции проверки
bool isDigit(char c) { return c >= '0' && c <= '9'; }
bool isLetter(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
bool isSpace(char c) { return c == ' ' || c == '\t' || c == '\r'; }
//Сохранение токенов в файл
void saveTokenSequence(const vector<pair<int, int>>& tokenSequence, const string& filename = "tokens.txt") {
    ofstream outFile(filename);
    if (outFile.is_open()) {
        for (const auto& p : tokenSequence) {
            outFile << "[" << p.first << "|" << p.second << "] ";
        }
        outFile.close();
        cout << "\nТокены сохранены в " << filename << "" << endl;
    }
    else {
        cout << "\nError to save '" << filename << "'" << endl;
    }
}

//Получение следующего символа с преобразованием букв в верхний регистр
void nextChar(const string& input, int& pos, bool inComment = false) {
    if (pos < (int)input.length()) {
        CH = input[pos++];
        if (!inComment && isLetter(CH)) {
            CH = toupper(CH);
        }
    }
    else {
        CH = '\0';
    }
}
//Получение класса символа
int getCharClass(char c) {
    if (isSpace(c)) return 0;
    if (isDigit(c)) return -1;
    if (isLetter(c)) return -2;
    if (c == '_') return -3;
    string s(1, c);
    for (const auto& entry : separatorTable) {
        if (entry.second == s) {
            return entry.first;
        }
    }
    return -4;
}
//Найти или создать константу
int getTableId(vector<pair<int, string>>& table, const string& s) {
    for (const auto& entry : table) {
        if (entry.second == s) {
            return entry.first;
        }
    }
    int id = (int)table.size() + 1;
    table.push_back({ id, s });
    return id;
}
int getTableId(vector<SymbolEntry>& table, const string& s) {
    for (const auto& entry : table) {
        if (entry.name == s) {
            return entry.id;
        }
    }
    int id = (int)table.size() + 1;
    table.push_back({ id, s, -1 });
    return id;
}
//Проверка на ключ слово
int getKeywordId(const string& symbol) {
    for (const auto& entry : keywordTable) {
        if (entry.second == symbol) {
            return entry.first;
        }
    }
    return 0;
}

//--------------Лексический анализатор---------------------
void lex_analyze(const string& input) {
    int pos = 0;
    int currentLine = 1;
    int currentCol = 1;
    nextChar(input, pos);
    while (CH != '\0') {
        A = "";
        int charClass = getCharClass(CH);
        // если пробел или таб (но не перенос строки)
        if (charClass == 0) {
            currentCol++;
            nextChar(input, pos);
        }
        // если перенос строки - обрабатываем отдельно
        else if (charClass == 13) {
            // (не в начале программы и не после другого переноса)
            if (!tokenSequence.empty() && !(tokenSequence.back().first == 2 && tokenSequence.back().second == 13)) {
                tokenSequence.push_back({ 2, 13 });
            }
            currentLine++;
            currentCol = 1;
            nextChar(input, pos);
        }
        //если число
        else if (charClass == -1) {
            while (charClass == -1) {
                A += CH;
                currentCol++;
                nextChar(input, pos);
                charClass = getCharClass(CH);
            }
            int const_id = getTableId(constantTable, A);
            tokenSequence.push_back({ 3, const_id });
        }
        //если буква, а после нее буква, цифра, _
        else if (charClass == -2) {
            while (charClass == -2 || charClass == -1 || charClass == -3) {
                A += CH;
                currentCol++;
                nextChar(input, pos);
                charClass = getCharClass(CH);
            }
            //если то что собралось это ключевое слово
            int kw_id = getKeywordId(A);
            if (kw_id != 0) {
                tokenSequence.push_back({ 1, kw_id });
            }
            else {
                int sym_id = getTableId(symbolTable, A);
                tokenSequence.push_back({ 4, sym_id });
            }
        }
        //если "
        else if (charClass == 12) {
            nextChar(input, pos, true);
            A = "\"";
            charClass = getCharClass(CH);
            while (charClass != 12 && CH != '\0') {
                A += CH;
                currentCol++;
                nextChar(input, pos, true);
                charClass = getCharClass(CH);
            }
            if (charClass == 12) {
                A += CH;
                currentCol++;
                nextChar(input, pos, false);
            }
            int const_id = getTableId(constantTable, A);
            tokenSequence.push_back({ 3, const_id });
        }
        //если остальные разделители
        else if (charClass > 0 && charClass != 13) {
            currentCol++;
            nextChar(input, pos);
            tokenSequence.push_back({ 2, charClass });
        }
        //ERROR
        else {
            int serial = (int)errorTable.size() + 1;
            errorTable.push_back({ serial, currentLine, currentCol, CH });
            currentCol++;
            nextChar(input, pos);
        }
    }
    // Явный маркер конца входа — токен '#'
    tokenSequence.push_back({ 0, 0 });
}
//Чтение токенов из файла
string readFile(const string& filename) {
    ifstream file(filename);
    string content, line;
    if (!file.is_open()) {
        cout << "Error to open " << filename << endl;
        return "";
    }
    while (getline(file, line)) {
        content += line + "\n";
    }
    return content;
}
//--------------Синтаксический анализатор------------------------------
int findSymbol(const string& name) {
    for (size_t i = 0; i < symbolTable.size(); i++) {
        if (symbolTable[i].name == name) {
            return i;
        }
    }
    return -1;
}
class TreeNode {
public:
    string value;
    vector<TreeNode*> children;
    TreeNode(string val) {
        value = val;
    }
    // Метод для добавления нескольких детей сразу
    void addChildren(const vector<TreeNode*>& newChildren) {
        for (auto child : newChildren) {
            if (child != nullptr) {
                children.push_back(child);
            }
        }
    }
    //Открывает файл и запускает рекурсивный обход дерева
    void saveToFile(const string& filename) {
        ofstream outFile(filename);
        if (outFile.is_open()) {
            saveToFileRecursive(outFile, this, "", true);
            outFile.close();
            cout << "Синтаксическое дерево сохранено в файл " << filename << "\n " << endl;
        }
        else {
            cout << "Ошибка сохранения в файл " << filename << endl;
        }
    }
    //Рекурсивно обходит дерево и записывает каждый узел
    void saveToFileRecursive(ofstream& outFile, TreeNode* node, string prefix, bool isLast) {
        if (!node) return;
        outFile << prefix << (isLast ? "└── " : "├── ") << node->value << endl;
        for (size_t i = 0; i < node->children.size(); i++) {
            saveToFileRecursive(outFile, node->children[i], prefix + (isLast ? "    " : "│   "), i == node->children.size() - 1);
        }
    }
    // Метод для создания узла с готовыми детьми
    static TreeNode* createNode(string value, const vector<TreeNode*>& children = {}) {
        TreeNode* node = new TreeNode(value);
        node->addChildren(children);
        return node;
    }
};
class SyntaxAnalyzer {
private:
    vector<pair<int, int>> tokens;
    int currentPos;
    pair<int, int> currentToken;
    TreeNode* syntaxTree;
    // Функция перехода к следующему токену
    void nextToken() {
        if (currentPos < (int)tokens.size()) {
            currentToken = tokens[currentPos++];
        }
        else {
            currentToken = { 0, 0 }; // Конец входных данных
        }
    }
    // Функция проверки соответствия текущего токена ожидаемому
    void match(int expectedType, int expectedValue = -1) {
        if (currentToken.first == expectedType &&
            (expectedValue == -1 || currentToken.second == expectedValue)) {
            nextToken();
        }
        else {
            string expected = expectedValue == -1 ?
                "type " + to_string(expectedType) :
                getTokenValue({ expectedType, expectedValue });
            string actual = getTokenValue(currentToken);
            throw runtime_error("Синтаксическая ошибка: ожидалось '" + expected + "', но найдено '" + actual + "'");
        }
    }
    // Функция проверки типа и значения токена
    bool checkToken(int type, int value = -1) {
        if (currentToken.first == type &&
            (value == -1 || currentToken.second == value)) {
            return true;
        }
        return false;
    }
    // Функция получения строкового представления токена
    string getTokenValue(pair<int, int> token) {
        if (token.first == 1) { // ключевое слово
            for (const auto& entry : keywordTable) {
                if (entry.first == token.second) {
                    return entry.second;
                }
            }
        }
        else if (token.first == 2) { // разделитель
            for (const auto& entry : separatorTable) {
                if (entry.first == token.second) {
                    return entry.second;
                }
            }
        }
        else if (token.first == 3) { // константа
            for (const auto& entry : constantTable) {
                if (entry.first == token.second) {
                    return entry.second;
                }
            }
        }
        else if (token.first == 4) { // идентификатор
            for (const auto& entry : symbolTable) {
                if (entry.id == token.second) {
                    return entry.name;
                }
            }
        }
        else if (token.first == 0 && token.second == 0) {
            return "#"; // конец входа
        }
        return "НЕИЗВЕСТНО";
    }

public:
    SyntaxAnalyzer(const vector<pair<int, int>>& tokenSeq) : tokens(tokenSeq), currentPos(0), syntaxTree(nullptr) {
        nextToken();
    }
    TreeNode* getSyntaxTree() {
        return syntaxTree;
    }
    // Функция для красивого вывода дерева
    void printTree(TreeNode* node, string prefix = "", bool isLast = true) {
        if (node != nullptr) {
            cout << prefix;
            cout << (isLast ? "└── " : "├── ");
            cout << node->value << endl;
            for (size_t i = 0; i < node->children.size(); i++) {
                bool lastChild = (i == node->children.size() - 1);
                printTree(node->children[i], prefix + (isLast ? "    " : "│   "), lastChild);
            }
        }
    }
    // <программа> ::= <строка> { <перенос_строки> <строка> }
    void parseProgram() {
        syntaxTree = TreeNode::createNode("PROGRAM");
        // Парсим все строки до конца входных данных
        while (!checkToken(0, 0)) {
            // Пропускаем пустые строки
            while (checkToken(2, 13)) {
                match(2, 13);
            }
            // Если после пропуска пустых строк достигли конца - выходим
            if (checkToken(0, 0)) {
                break;
            }
            TreeNode* line = parseLine();
            if (line != nullptr) {
                syntaxTree->addChildren({ line });
            }
            // После каждой строки может быть перенос
            if (checkToken(2, 13)) {
                match(2, 13);
            }
        }
    }
    TreeNode* parseLine() {
        if (checkToken(0, 0)) {
            return nullptr; // Конец
        }
        // Пропускаем пустые строки
        while (checkToken(2, 13)) {
            match(2, 13);
        }
        if (checkToken(0, 0)) {
            return nullptr;
        }
        return parseOperator();
    }
private:
    // <оператор> ::= <оператор_присваивания> | <оператор_вывода> | ...
    TreeNode* parseOperator() {
        if (checkToken(4)) { // Идентификатор - оператор присваивания
            return parseAssignmentOperator();
        }
        else if (checkToken(1, 8)) { // PRINT
            return parseOutputOperator();
        }
        else if (checkToken(1, 9)) { // INPUT
            return parseInputOperator();
        }
        else if (checkToken(1, 11)) { // DIM
            return parseDeclarationOperator();
        }
        else if (checkToken(1, 1)) { // IF
            return parseConditionalOperator();
        }
        else {
            string actual = getTokenValue(currentToken);
            throw runtime_error("Синтаксическая ошибка в операторе: ожидался идентификатор ключевое слово, но найдено '" + actual + "'");
        }
    }
    // <условный_оператор> ::= IF <логическое_выражение> THEN <оператор> [ ELSE <оператор> ] 
  //                       | IF <логическое_выражение> THEN <перенос_строки> {<оператор>} 
  //                         { ELSEIF <логическое_выражение> THEN <перенос_строки> <операторы> } 
  //                         [ ELSE <перенос_строки> <операторы> ] END IF
    TreeNode* parseConditionalOperator() {
        match(1, 1); // IF
        TreeNode* condition = parseLogicalExpression();
        match(1, 2); // THEN
        // Проверяем, это однострочный или многострочный IF
        if (checkToken(2, 13)) { // Перенос строки - многострочный IF
            return parseMultilineIf(condition);
        }
        else {
            return parseSingleLineIf(condition);
        }
    }
    // Однострочный IF: IF ... THEN оператор [ ELSE оператор ]
    TreeNode* parseSingleLineIf(TreeNode* condition) {
        vector<TreeNode*> thenBranch;
        thenBranch.push_back(parseOperator()); // Один оператор после THEN
        vector<TreeNode*> elseBranch;
        // Проверяем наличие ELSE
        if (checkToken(1, 3)) { // ELSE
            match(1, 3);
            elseBranch.push_back(parseOperator()); // Один оператор после ELSE
        }
        // Создаем узел для однострочного IF
        vector<TreeNode*> ifChildren;
        ifChildren.push_back(condition);
        ifChildren.push_back(TreeNode::createNode("THEN_BLOCK", thenBranch));

        if (!elseBranch.empty()) {
            ifChildren.push_back(TreeNode::createNode("ELSE_BLOCK", elseBranch));
        }
        return TreeNode::createNode("IF_SINGLE", ifChildren);
    }

    // Многострочный IF: IF ... THEN <перенос> операторы... END IF
    TreeNode* parseMultilineIf(TreeNode* condition) {
        match(2, 13); // Первая строка после THEN
        vector<TreeNode*> thenBranch;
        // Обрабатываем строки в THEN блоке до ELSEIF, ELSE или END IF
        while (true) {
            // Пропускаем пустые строки
            while (checkToken(2, 13)) {
                match(2, 13);
            }
            // Проверяем, не начинается ли следующая строка с ELSEIF, ELSE или END IF
            if (checkToken(1, 4) || checkToken(1, 3) || checkToken(1, 5)) {
                break;
            }
            // Если достигли конца файла
            if (checkToken(0, 0)) {
                break;
            }
            // Если строка не пустая, оператор
            thenBranch.push_back(parseOperator());
            // После оператора может быть перенос строки
            if (checkToken(2, 13)) {
                match(2, 13);
            }
        }
        vector<TreeNode*> elseifBranches;
        vector<TreeNode*> elseBranch;
        // Обрабатываем блоки ELSEIF
        while (checkToken(1, 4)) { // ELSEIF
            match(1, 4); // ELSEIF

            TreeNode* elseifCondition = parseLogicalExpression();
            match(1, 2); // THEN

            // Обязательный перенос строки после THEN в многострочном IF
            if (!checkToken(2, 13)) {
                throw runtime_error("Синтаксическая ошибка: ожидался перенос строки после THEN в многострочном IF");
            }
            match(2, 13);
            vector<TreeNode*> elseifThenBranch;
            // Обрабатываем строки в ELSEIF блоке
            while (true) {
                // Пропускаем пустые строки
                while (checkToken(2, 13)) {
                    match(2, 13);
                }
                // Проверяем, не начинается ли следующая строка с ELSEIF, ELSE или END IF
                if (checkToken(1, 4) || checkToken(1, 3) || checkToken(1, 5)) {
                    break;
                }
                // Если достигли конца файла
                if (checkToken(0, 0)) {
                    break;
                }
                // Если строка не пустая, оператор
                elseifThenBranch.push_back(parseOperator());
                // После оператора может быть перенос строки
                if (checkToken(2, 13)) {
                    match(2, 13);
                }
            }
            if (!elseifThenBranch.empty()) {
                elseifBranches.push_back(TreeNode::createNode("ELSEIF_BRANCH", {
                    elseifCondition,
                    TreeNode::createNode("THEN_BLOCK", elseifThenBranch)
                    }));
            }
        }
        // Обрабатываем блок ELSE (если есть)
        if (checkToken(1, 3)) { // ELSE
            match(1, 3); // ELSE
            // Обязательный перенос строки после ELSE в многострочном IF
            if (!checkToken(2, 13)) {
                throw runtime_error("Синтаксическая ошибка: ожидался перенос строки после ELSE в многострочном IF");
            }
            match(2, 13);
            // Обрабатываем строки в ELSE блоке
            while (true) {
                // Пропускаем пустые строки
                while (checkToken(2, 13)) {
                    match(2, 13);
                }
                // Проверяем, не начинается ли следующая строка с END IF
                if (checkToken(1, 5)) {
                    break;
                }
                // Если достигли конца файла
                if (checkToken(0, 0)) {
                    break;
                }
                // Если строка не пустая, парсим оператор
                elseBranch.push_back(parseOperator());
                // После оператора может быть перенос строки
                if (checkToken(2, 13)) {
                    match(2, 13);
                }
            }
        }
        match(1, 5); // END
        match(1, 1); // IF
        // Пропускаем перенос строки после END IF
        if (checkToken(2, 13)) {
            match(2, 13);
        }
        // Создаем узел условного оператора
        vector<TreeNode*> ifChildren;
        ifChildren.push_back(condition);
        if (!thenBranch.empty()) {
            ifChildren.push_back(TreeNode::createNode("THEN_BLOCK", thenBranch));
        }
        else {
            ifChildren.push_back(TreeNode::createNode("THEN_BLOCK", {}));
        }
        for (auto elseif : elseifBranches) {
            ifChildren.push_back(elseif);
        }
        if (!elseBranch.empty()) {
            ifChildren.push_back(TreeNode::createNode("ELSE_BLOCK", elseBranch));
        }
        return TreeNode::createNode("IF_MULTILINE", ifChildren);
    }
    // <логическое_выражение> ::= <логическое_выражение_или>
    TreeNode* parseLogicalExpression() {
        return parseLogicalOr();
    }
    // <логическое_выражение_или> ::= <логическое_выражение_и> { OR <логическое_выражение_и> }
    TreeNode* parseLogicalOr() {
        TreeNode* left = parseLogicalAnd();
        while (checkToken(1, 7)) { // OR
            string op = getTokenValue(currentToken);
            match(1, 7);
            TreeNode* right = parseLogicalAnd();
            left = TreeNode::createNode(op, { left, right });
        }
        return left;
    }
    // <логическое_выражение_и> ::= <логическое_выражение_сравнение> { AND <логическое_выражение_сравнение> }
    TreeNode* parseLogicalAnd() {
        TreeNode* left = parseLogicalComparison();
        while (checkToken(1, 6)) { // AND
            string op = getTokenValue(currentToken);
            match(1, 6);
            TreeNode* right = parseLogicalComparison();
            left = TreeNode::createNode(op, { left, right });
        }
        return left;
    }
    TreeNode* parseLogicalComparison() {
        TreeNode* left = parseLogicalFactor();
        // Проверяем наличие оператора сравнения
        if (checkToken(2, 10) || checkToken(2, 11) || checkToken(2, 1)) {
            string op = getTokenValue(currentToken);
            match(currentToken.first, currentToken.second);
            // <=, >=, <>
            if ((op == "<" || op == ">") && checkToken(2, 1)) { // <=, >=
                op += getTokenValue(currentToken);
                match(2, 1);
            }
            else if (op == "<" && checkToken(2, 11)) { // <>
                op += getTokenValue(currentToken);
                match(2, 11);
            }
            TreeNode* right = parseLogicalFactor();
            // Если оператор =, создаем узел ==, иначе используем сам оператор
            if (op == "=") {
                return TreeNode::createNode("==", { left, right });
            }
            else {
                return TreeNode::createNode(op, { left, right });
            }
        }
        return left;
    }
    // <логический_фактор> ::= ( <логическое_выражение> ) | <выражение>
    TreeNode* parseLogicalFactor() {
        if (checkToken(2, 6)) { // '('
            match(2, 6);
            TreeNode* node = parseLogicalExpression(); // выражение внутри скобок
            match(2, 7); // ')'
            return node;
        }
        else {
            // Если нет скобок, обычное выражение
            return parseExpression();
        }
    }
    // <оператор_объявления> ::= DIM <идентификатор> { , <идентификатор> } AS <тип>
    TreeNode* parseDeclarationOperator() {
        match(1, 11); // DIM
        vector<TreeNode*> declarations;
        vector<string> identifiers;
        if (!checkToken(4)) {
            string actual = getTokenValue(currentToken);
            throw runtime_error("Синтаксическая ошибка: ожидался идентификатор, но найдено '" + actual + "'");
        }
        string firstId = getTokenValue(currentToken);
        int idx = findSymbol(firstId);
        // Проверяем, не объявлена ли уже
        if (symbolTable[idx].used != -1) {
            throw runtime_error("Семантическая ошибка: переменная '" + firstId + "' объявлена дважды");
        }
        // Объявляем: used = 0 (объявлена, но не использована)
        symbolTable[idx].used = 0;
        identifiers.push_back(firstId);
        match(4);

        while (checkToken(2, 9)) { // запятая
            match(2, 9);
            string nextId = getTokenValue(currentToken);
            idx = findSymbol(nextId);
            if (symbolTable[idx].used != -1) {
                throw runtime_error("Семантическая ошибка: переменная '" + nextId + "' объявлена дважды");
            }
            symbolTable[idx].used = 0;
            identifiers.push_back(nextId);
            match(4);
        }
        match(1, 12); // AS
        TreeNode* typeNode = parseType();
        for (const auto& identifier : identifiers) {
            declarations.push_back(TreeNode::createNode("VAR", { new TreeNode(identifier), typeNode }));
        }
        return TreeNode::createNode("DIM", declarations);
    }
    // <тип> ::= INTEGER 
    TreeNode* parseType() {
        if (checkToken(1, 13)) { // INTEGER
            TreeNode* node = new TreeNode("INTEGER");
            match(1, 13);
            return node;
        }
        else {
            string actual = getTokenValue(currentToken);
            throw runtime_error("Синтаксическая ошибка в типе: ожидался INTEGER, но найдено '" + actual + "'");
        }
    }
    // <оператор_присваивания> ::= <идентификатор> = <выражение>
    TreeNode* parseAssignmentOperator() {
        string identifier = getTokenValue(currentToken);
        // СЕМАНТИЧЕСКАЯ ПРОВЕРКА
        int idx = findSymbol(identifier);
        if (idx == -1) {
            throw runtime_error("Семантическая ошибка: переменная '" + identifier + "' не объявлена");
        }
        if (symbolTable[idx].used == -1) {
            throw runtime_error("Семантическая ошибка: переменная '" + identifier + "' не объявлена");
        }
        // used может быть 0 или 1, ставим 1 (использована)
        symbolTable[idx].used = 1;
        match(4); // Идентификатор
        match(2, 1); // '='
        TreeNode* expression = parseExpression();
        return TreeNode::createNode("=", { new TreeNode(identifier), expression });
    }
    // <оператор_вывода> ::= PRINT <выражение_печати> { , <выражение_печати> }
    TreeNode* parseOutputOperator() {
        match(1, 8); // PRINT
        vector<TreeNode*> expressions;
        expressions.push_back(parsePrintExpression());
        while (checkToken(2, 9)) {    // запятая
            match(2, 9);
            expressions.push_back(parsePrintExpression());
        }
        return TreeNode::createNode("PRINT", expressions);
    }
    // <выражение_печати> ::= <строка> | <выражение>
    TreeNode* parsePrintExpression() {
        if (checkToken(3)) {
            string value = getTokenValue(currentToken);
            if (value[0] == '\"') {
                //строка - обрабатываем отдельно
                TreeNode* node = new TreeNode(value);
                match(3);
                return node;
            }
        }
        // Все остальное - выражение
        return parseExpression();
    }
    // <оператор_ввода> ::= INPUT <идентификатор>
    TreeNode* parseInputOperator() {
        match(1, 9); // INPUT
        string identifier = getTokenValue(currentToken);
        int idx = findSymbol(identifier);
        if (idx == -1) {
            throw runtime_error("Семантическая ошибка: переменная '" + identifier + "' не объявлена");
        }
        if (symbolTable[idx].used == -1) {
            throw runtime_error("Семантическая ошибка: переменная '" + identifier + "' не объявлена");
        }
        symbolTable[idx].used = 1;
        match(4); // Идентификатор
        return TreeNode::createNode("INPUT", { new TreeNode(identifier) });
    }
    // <выражение> ::= <терм> { (+ | -) <терм> }
    TreeNode* parseExpression() {
        vector<TreeNode*> nodes;
        vector<string> operators;
        // Первый терм
        nodes.push_back(parseTerm());
        // Собираем все операторы и термы
        while (checkToken(2, 3) || checkToken(2, 4)) {
            operators.push_back(getTokenValue(currentToken));
            match(currentToken.first, currentToken.second);
            nodes.push_back(parseTerm());
        }
        // Строим лево-ассоциативное дерево
        TreeNode* result = nodes[0];
        for (size_t i = 0; i < operators.size(); i++) {
            result = TreeNode::createNode(operators[i], { result, nodes[i + 1] });
        }
        return result;
    }
    // <терм> ::= <фактор> { (* | / | MOD) <фактор> }
    TreeNode* parseTerm() {
        vector<TreeNode*> nodes;
        vector<string> operators;
        // Первый фактор
        nodes.push_back(parseFactor());
        // Собираем все операторы и факторы
        while (checkToken(2, 5) || checkToken(2, 2) || checkToken(1, 10)) {
            operators.push_back(getTokenValue(currentToken));
            match(currentToken.first, currentToken.second);
            nodes.push_back(parseFactor());
        }
        // Строим лево-ассоциативное дерево
        TreeNode* result = nodes[0];
        for (size_t i = 0; i < operators.size(); i++) {
            result = TreeNode::createNode(operators[i], { result, nodes[i + 1] });
        }
        return result;
    }
    // <фактор> ::= <степень> { ^ <степень> }
    TreeNode* parseFactor() {
        return parseExponent();
    }
    // <степень> ::= <первичное_выражение> { ^ <первичное_выражение> }
    TreeNode* parseExponent() {
        vector<TreeNode*> nodes;
        vector<string> operators;
        // Первое первичное выражение
        nodes.push_back(parsePrimary());
        // Собираем все операторы ^ и первичные выражения
        while (checkToken(2, 8)) {
            operators.push_back(getTokenValue(currentToken));
            match(2, 8);
            nodes.push_back(parsePrimary());
        }
        // Строим право-ассоциативное дерево для степени
        TreeNode* result = nodes.back();
        for (int i = (int)operators.size() - 1; i >= 0; i--) {
            result = TreeNode::createNode(operators[i], { nodes[i], result });
        }

        return result;
    }
    // <первичное_выражение> ::= [ - ] ( <число> | <идентификатор> | ( <выражение> ) )
    TreeNode* parsePrimary() {
        // Проверяем наличие унарного минуса
        int isNegative = 0;
        if (checkToken(2, 4)) { // только унарный минус
            match(2, 4);
            isNegative = 1;
        }
        if (checkToken(2, 3)) { // если встретили унарный плюс
            match(2, 3);
            return parsePrimary();
        }
        TreeNode* node = nullptr;
        if (checkToken(3)) {  // число
            string value = getTokenValue(currentToken);
            node = new TreeNode(value);
            match(3);
            //после числа не должен идти идентификатор без оператора
            if (checkToken(4)) {
                string nextValue = getTokenValue(currentToken);
                string actual = getTokenValue(currentToken);
                throw runtime_error("Синтаксическая ошибка: ожидался оператор (+, -, *, /, ^, MOD, AND, OR, =, <, >, <=, >=, <>), но найдено '" + actual + "'");
            }
        }
        else if (checkToken(4)) {  // идентификатор
            string value = getTokenValue(currentToken);
            int idx = findSymbol(value);
            if (idx == -1) {
                throw runtime_error("Семантическая ошибка: переменная '" + value + "' не объявлена");
            }
            if (symbolTable[idx].used == -1) {
                throw runtime_error("Семантическая ошибка: переменная '" + value + "' не объявлена");
            }
            symbolTable[idx].used = 1;
            node = new TreeNode(value);
            match(4);
            // после идентификатора не должен идти другой идентификатор или число без оператора
            if (checkToken(4) || checkToken(3)) {
                string nextValue = getTokenValue(currentToken);
                string actual = getTokenValue(currentToken);
                throw runtime_error("Синтаксическая ошибка: ожидался оператор (+, -, *, /, ^, MOD, AND, OR, =, <, >, <=, >=, <>), но найдено '" + actual + "'");
            }
        }
        else if (checkToken(2, 6)) {
            match(2, 6);
            node = parseExpression();
            match(2, 7);
        }
        else {
            string actual = getTokenValue(currentToken);
            throw runtime_error("Синтаксическая ошибка в первичном выражении: ожидалось число, идентификатор или '(', но найдено '" + actual + "'");
        }
        // Применяем унарный минус если был
        if (isNegative != 0) {
            node = TreeNode::createNode("-", { node });
        }
        return node;
    }
};

//--------------Интерпретатор------------------------------
class Interpreter {
private:
    vector<pair<string, int>> variables;
    TreeNode* syntaxTree;
    // функция для поиска переменной в векторе
    int findVariable(const string& name) {
        for (size_t i = 0; i < variables.size(); i++) {
            if (variables[i].first == name) {
                return i;
            }
        }
        return -1;
    }
    // функция для получения значения переменной
    int getVariable(const string& name) {
        int idx = findVariable(name);
        if (idx != -1) {
            return variables[idx].second;
        }
        return 0;
    }
    //функция для установки значения переменной
    void setVariable(const string& name, int value) {
        int idx = findVariable(name);
        if (idx != -1) {
            variables[idx].second = value;
        }
        else {
            variables.push_back({ name, value });
        }
    }
    // функция для вычисления значения узла
    int evaluateNode(TreeNode* node) {
        if (node == nullptr) return 0;
        string value = node->value;
        // Узел-константа (число)
        if (value.find_first_not_of("0123456789") == string::npos) {
            return stoi(value);
        }
        if (value == "+") {
            if (node->children.size() == 1) { // унарный плюс
                return evaluateNode(node->children[0]);
            }
            return evaluateNode(node->children[0]) + evaluateNode(node->children[1]);
        }
        else if (value == "-") {
            if (node->children.size() == 1) { // унарный минус
                return -evaluateNode(node->children[0]);
            }
            return evaluateNode(node->children[0]) - evaluateNode(node->children[1]);
        }
        else if (value == "*") {
            return evaluateNode(node->children[0]) * evaluateNode(node->children[1]);
        }
        else if (value == "/") {
            int right = evaluateNode(node->children[1]);
            if (right == 0) {
                throw runtime_error("деление на ноль");
            }
            return evaluateNode(node->children[0]) / right;
        }
        else if (value == "^") {
            int left = evaluateNode(node->children[0]);
            int right = evaluateNode(node->children[1]);
            int result = 1;
            for (int i = 0; i < right; i++) {
                result *= left;
            }
            return result;
        }
        else if (value == "MOD") {
            int right = evaluateNode(node->children[1]);
            if (right == 0) {
                throw runtime_error("деление на ноль в MOD");
            }
            return evaluateNode(node->children[0]) % right;
        }
        // Логические операции (возвращают 1 - истина, 0 - ложь)
        else if (value == "AND") {
            return (evaluateNode(node->children[0]) != 0 && evaluateNode(node->children[1]) != 0) ? 1 : 0;
        }
        else if (value == "OR") {
            return (evaluateNode(node->children[0]) != 0 || evaluateNode(node->children[1]) != 0) ? 1 : 0;
        }
        else if (value == "==") {
            return (evaluateNode(node->children[0]) == evaluateNode(node->children[1])) ? 1 : 0;
        }
        else if (value == "=") {
            if (node->children.size() == 2 &&
                node->children[0]->children.size() == 0) {
                string varName = node->children[0]->value;
                int val = evaluateNode(node->children[1]);
                setVariable(varName, val);
                return val;
            }
        }
        else if (value == "<") {
            return (evaluateNode(node->children[0]) < evaluateNode(node->children[1])) ? 1 : 0;
        }
        else if (value == ">") {
            return (evaluateNode(node->children[0]) > evaluateNode(node->children[1])) ? 1 : 0;
        }
        else if (value == "<=") {
            return (evaluateNode(node->children[0]) <= evaluateNode(node->children[1])) ? 1 : 0;
        }
        else if (value == ">=") {
            return (evaluateNode(node->children[0]) >= evaluateNode(node->children[1])) ? 1 : 0;
        }
        else if (value == "<>") {
            return (evaluateNode(node->children[0]) != evaluateNode(node->children[1])) ? 1 : 0;
        }
        if (value.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ") == string::npos) {
            return getVariable(value);
        }
        throw runtime_error("Неизвестная операция: " + value);
    }
    //PRINT
    void executePrint(TreeNode* node) {
        for (size_t i = 0; i < node->children.size(); i++) {
            auto child = node->children[i];
            if (child->value[0] == '\"') {
                // Строковая константа - выводим без кавычек
                string str = child->value;
                cout << str.substr(1, str.length() - 2);
            }
            else {
                // Выражение
                cout << evaluateNode(child);
            }
            // Если не последний элемент, выводим таб
            if (i < node->children.size() - 1) {
                cout << "\t";
            }
        }
        cout << endl;
    }
    // INPUT
    void executeInput(TreeNode* node) {
        string varName = node->children[0]->value;
        int value;
        cout << "Введите значение для " << varName << ": ";
        while (true) {
            cin >> value;
            if (cin.fail()) {
                cin.clear();
                cin.ignore(1000, '\n');
                cout << "Ошибка ввода, введите целое число " << varName << ": ";
                continue;
            }
            break;
        }
        setVariable(varName, value);
    }
    // DIM
    void executeDim(TreeNode* node) {
        for (auto decl : node->children) {
            string varName = decl->children[0]->value;
            if (findVariable(varName) == -1) {
                setVariable(varName, 0);
            }
        }
    }
    // Выполнение условного оператора (однострочного)
    void executeIfSingle(TreeNode* node) {
        TreeNode* condition = node->children[0];
        TreeNode* thenBlock = node->children[1];
        if (evaluateNode(condition) != 0) {
            // Выполняем THEN блок
            for (auto op : thenBlock->children) {
                executeNode(op);
            }
        }
        else if (node->children.size() > 2) {
            // Есть ELSE блок
            TreeNode* elseBlock = node->children[2];
            for (auto op : elseBlock->children) {
                executeNode(op);
            }
        }
    }
    // Выполнение многострочного условного оператора
    void executeIfMultiline(TreeNode* node) {
        bool conditionMet = false;
        // Проверяем основное условие
        TreeNode* condition = node->children[0];
        if (evaluateNode(condition) != 0) {
            conditionMet = true;
            TreeNode* thenBlock = node->children[1];
            for (auto op : thenBlock->children) {
                executeNode(op);
            }
        }
        // Проверяем ELSEIF
        if (!conditionMet) {
            for (size_t i = 2; i < node->children.size(); i++) {
                TreeNode* child = node->children[i];
                if (child->value == "ELSEIF_BRANCH") {
                    TreeNode* elseifCondition = child->children[0];
                    if (evaluateNode(elseifCondition) != 0) {
                        conditionMet = true;
                        TreeNode* elseifThenBlock = child->children[1];
                        for (auto op : elseifThenBlock->children) {
                            executeNode(op);
                        }
                        break;
                    }
                }
                else if (child->value == "ELSE_BLOCK") {
                    // ELSE блок - выполняется, если ни одно условие не сработало
                    if (!conditionMet) {
                        for (auto op : child->children) {
                            executeNode(op);
                        }
                    }
                }
            }
        }
    }
    // Выполнение узла в зависимости от его типа
    void executeNode(TreeNode* node) {
        if (node == nullptr) return;
        string value = node->value;
        if (value == "=") {
            evaluateNode(node);
        }
        else if (value == "PRINT") {
            executePrint(node);
        }
        else if (value == "INPUT") {
            executeInput(node);
        }
        else if (value == "DIM") {
            executeDim(node);
        }
        else if (value == "IF_SINGLE") {
            executeIfSingle(node);
        }
        else if (value == "IF_MULTILINE") {
            executeIfMultiline(node);
        }
        else if (value == "PROGRAM") {
            for (auto child : node->children) {
                executeNode(child);
            }
        }
        else if (value == "THEN_BLOCK" || value == "ELSE_BLOCK" || value == "ELSEIF_BRANCH") {
            for (auto child : node->children) {
                executeNode(child);
            }
        }
        else {
            evaluateNode(node);
        }

    }

public:
    Interpreter(TreeNode* tree) : syntaxTree(tree) {}
    void run() {
        if (syntaxTree == nullptr) {
            cout << "Ошибка: синтаксическое дерево пустое" << endl;
            return;
        }
        try {
            cout << "\n=== ВЫПОЛНЕНИЕ ПРОГРАММЫ ===" << endl;
            executeNode(syntaxTree);
        }
        catch (const exception& e) {
            cout << "Ошибка выполнения: " << e.what() << endl;
        }
    }
    void printVariables() {
        cout << "\nТекущие значения переменных:" << endl;
        for (const auto& var : variables) {
            cout << "  " << var.first << " = " << var.second << endl;
        }
    }
};
void syntax_analyze() {
    try {
        SyntaxAnalyzer analyzer(tokenSequence);
        analyzer.parseProgram();
        //cout << "\n=== СИНТАКСИЧЕСКОЕ ДЕРЕВО ===" << endl;
        //analyzer.printTree(analyzer.getSyntaxTree());
        analyzer.getSyntaxTree()->saveToFile("syntax_tree.txt");
        for (const auto& entry : symbolTable) {
            if (entry.used == 0) {
                cout << "Предупреждение: переменная '" << entry.name
                    << "' объявлена, но не используется" << endl;
            }
        }
        cout << "Программа синтаксически и семантически корректна!" << endl;
        Interpreter interpreter(analyzer.getSyntaxTree());
        interpreter.run();
    }
    catch (const exception& e) {
        cout << "Ошибка: " << e.what() << endl;
    }
}
int main() {
    //setlocale(LC_ALL, "Russian");
    string filename = "program.txt";
    string code = readFile(filename);
    if (!code.empty()) {
        cout << "Source code:\n" << code << endl;
        lex_analyze(code);
        cout << "\nKeywordTable(1):" << endl;
        for (const auto& entry : keywordTable) {
            cout << entry.first << ": " << entry.second << endl;
        }
        cout << "\nSeparatorTable(2):" << endl;
        for (const auto& entry : separatorTable) {
            cout << entry.first << ": " << entry.second << endl;
        }
        cout << "\nConstant Table(3):" << endl;
        for (const auto& entry : constantTable) {
            cout << entry.first << ": " << entry.second << endl;
        }
        cout << "\nSymbol Table(4):" << endl;
        for (const auto& entry : symbolTable) {
            cout << entry.id << ": " << entry.name << endl;
        }
        cout << "\nError Table(5):" << endl;
        for (const auto& entry : errorTable) {
            cout << entry.num << ": " << entry.line << ", " << entry.col << ": '";
            cout << entry.sym << "'" << endl;
        }
        //cout << "\nToken Sequence:" << endl;
        //for (const auto& p : tokenSequence) {
        //    cout << "[" << p.first << "|" << p.second << "] ";
        //}
        //cout << endl;
        saveTokenSequence(tokenSequence);
        if (!tokenSequence.empty()) {
            syntax_analyze();
        }
    }
    return 0;
}