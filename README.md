# cpp-search-server
# Спринт 8, финальный проект: поисковая система

#### Проект поискового сервера, осуществляющего поиск среди текстов документов с возможностью указания стоп-слов (игнорируются сервером), минус-слов (документы с ними не учитываются в выдаче)


## Использование:

1) Установка стоп-слов с помощью конструктора. 
Например: **SearchServer search_server("and in at with"s);**
(Стоп-слово в запросе не учитывается при поиске)
2) Добавление документов в поисковую систему. 
    Например: **search_server.AddDocument(1, "funny pet and nasty -rat"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });**
    - Объявление метода: void AddDocument(int document_id, string_view document, DocumentStatus status, const vector<int> &ratings);
    - document_id - id документа
    - document - строка. Например: "funny pet and nasty -rat"s,
    - где "funny pet nasty" - слова по которым осуществляется поиcк,
    - "and" - стоп слово, 
    - "-rat" - минус слово, минус-слова исключают из результатов поиска документы, содержащие такие слова.
    - status - статус документа. Возможный DocumentStatus: ACTUAL, IRRELEVANT, BANNED, REMOVED
    - ratings - рэйтинг документа, каждый документ на входе имеет набор оценок пользователей. 
    - Первая цифра — это количество оценок, например:{4 5 -12 2 1};
3) Поиск document в поисковом сервере. Например: search_server.FindTopDocuments("curly nasty cat"s)

#### Пример использования кода: 


     SearchServer search_server("and with"s);
     for (
        int id = 0;
        const string & text : {
        "white cat and yellow hat"s, "funny pet and nasty -rat"s, "nasty dog with big eyes"s, "nasty pigeon john"s,
        }
    ) 
    {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    }
    cout << "ACTUAL by default:"s << endl;
    // последовательная версия
    for (const Document& document : search_server.FindTopDocuments("curly nasty cat"s)) {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    // последовательная версия
    for (const Document& document : search_server.FindTopDocuments(execution::seq, "curly nasty cat"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    // параллельная версия
    for (const Document& document : search_server.FindTopDocuments(execution::par, "curly nasty cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }
    
Вывод: 

    ACTUAL by default:
    { document_id = 1, relevance = 0.346574, rating = -1 }
    { document_id = 4, relevance = 0.095894, rating = -1 }
    { document_id = 2, relevance = 0.0719205, rating = -1 }
    { document_id = 3, relevance = 0.0719205, rating = -1 }
    BANNED:
    Even ids:
    { document_id = 4, relevance = 0.095894, rating = -1 }
    { document_id = 2, relevance = 0.0719205, rating = -1 }
   
## Системные требования:
С++17 (STL);

GCC (MinGW-w64) 11.2.0.

## Стек технологий:
C++ stl
