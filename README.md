# cpp-search-server
Спринт 8, финальный проект: поисковая система

Проект поискового сервера, осуществляющего поиск среди текстов документов с возможностью указания стоп-слов (игнорируются сервером), минус-слов (документы с ними не учитываются в выдаче)

Использование:
Установка стоп-слов с помощью конструктора. Например: SearchServer search_server("and in at with"s); 
Стоп-слово в запросе не учитывается при поиске.

Добавление документов в поисковую систему. Например: search_server.AddDocument(1, "funny pet and nasty -rat"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
Объявление метода: void AddDocument(int document_id, string_view document, DocumentStatus status, const vector<int> &ratings);
document_id - id документа
document - строка. Например: "funny pet and nasty -rat"s,
где "funny pet nasty" - слова по которым осуществляется поиcк,
"and" - стоп слово, 
"-rat" - минус слово, минус-слова исключают из результатов поиска документы, содержащие такие слова.
status - статус документа. Возможный DocumentStatus: ACTUAL, IRRELEVANT, BANNED, REMOVED
ratings - рэйтинг документа, каждый документ на входе имеет набор оценок пользователей.
Первая цифра — это количество оценок, например:{4 5 -12 2 1};

Поиск document в поисковом сервере. Например: search_server.FindTopDocuments("curly nasty cat"s)

Системные требования:
С++17 (STL);
GCC (MinGW-w64) 11.2.0.

Стек технологий:
C++ stl
