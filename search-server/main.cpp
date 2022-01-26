///////////////////////////////
ostream& operator<< (ostream& out, tuple<vector<string>, DocumentStatus> test) {    //перегружаем оператор вывода для корректной работы теста TestMatchDocument()
    tuple<vector<string>, DocumentStatus> t;
    if (test == t) {    //empty check
        return out;
    }
    bool is_first = true;
    for (const auto& el : get<0>(test)) {
        if (is_first) {
            out << "{"s << el;
            is_first = false;
        }
        else {
            out << ", "s << el;
        }
    }
    out << "}"s;
    return out;
}

const double EXP = 1E-5;

// -------- Начало модульных тестов поисковой системы ----------

void TestAddDocument() {
    const int doc_id_1 = 23;
    const string content_1 = "pipi pupu papa popo"s;
    const vector<int> ratings_1 = { 1, 4, 8, 8 };

    const int doc_id_2 = 33;
    const string content_2 = "pipi papa"s;

    const int doc_id_3 = 43;
    const string content_3 = "pipi pupu"s;

    {
        SearchServer server;
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        const auto found_docs = server.FindTopDocuments("popo"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id_1);
    }

    {
        SearchServer server;
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_1);
        const auto found_docs1 = server.FindTopDocuments("papa"s);
        ASSERT_EQUAL(found_docs1.size(), 1u);
        const Document& doc0 = found_docs1[0];
        ASSERT_EQUAL(doc0.id, doc_id_2);

        const auto found_docs2 = server.FindTopDocuments("pipi"s);
        ASSERT_EQUAL(found_docs2.size(), 2u);
        const Document& doc_0 = found_docs2[0];
        ASSERT_EQUAL(doc_0.id, doc_id_2);
        const Document& doc_1 = found_docs2[1];
        ASSERT_EQUAL(doc_1.id, doc_id_3);
    }
}

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}

void TestMinusWords() {
    const int doc_id = 23;
    const string content = "pipi pupu papa popo"s;
    const vector<int> ratings = { 1, 4, 8, 8 };

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("popo -pupu"s);
        ASSERT(found_docs.empty());
    }
}

void TestMatchDocument() {
    const int doc_id = 23;
    const string content = "pipi pupu papa popo"s;
    const vector<int> ratings = { 1, 4, 8, 8 };

    { //without minus
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto matched_docs = server.MatchDocument("pipi papa"s, 23);
        const tuple<vector<string>, DocumentStatus> test = { {"papa"s, "pipi"s}, DocumentStatus::ACTUAL };
        ASSERT_EQUAL(matched_docs, test);   //реализовать перегрузку вывода tuple <vector<string>, DocumentStatus>
    }

    { //with minus
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto matched_docs = server.MatchDocument("pipi -papa"s, 23);
        const tuple<vector<string>, DocumentStatus> test = {};
        ASSERT_EQUAL(matched_docs, test);
    }
}

void TestSortDocumentRelevance() {
    const int doc_id_1 = 23;
    const string content_1 = "pipi pipi pupu"s;
    const vector<int> ratings_1 = {};

    const int doc_id_2 = 33;
    const string content_2 = "pipi papa pupu"s;

    const int doc_id_3 = 43;
    const string content_3 = "pipi popo"s;

    {
        SearchServer server;
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_1);
        const auto found_docs_1 = server.FindTopDocuments("pipi"s);

        ASSERT_EQUAL(found_docs_1[0].id, doc_id_1);
        ASSERT_EQUAL(found_docs_1[1].id, doc_id_2);
        ASSERT_EQUAL(found_docs_1[2].id, doc_id_3);
    }
}

void TestComputeRating() {
    const int doc_id_1 = 23;
    const string content_1 = "pipi pupu papa popo"s;
    const vector<int> ratings_1 = { 1, 4, 8, 8 };

    const int doc_id_2 = 33;
    const string content_2 = "pipi pupu papa popo"s;
    const vector<int> ratings_2 = { -1, -4, -8, -8 };

    const int doc_id_3 = 43;
    const string content_3 = "pipi pupu papa popo"s;
    const vector<int> ratings_3 = { 1, 4, -8, -8 };

    {
        SearchServer server;
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        const auto found_docs = server.FindTopDocuments("popo pupu"s);
        ASSERT_EQUAL(found_docs[0].rating, 5);
    }

    {
        SearchServer server;
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        const auto found_docs = server.FindTopDocuments("popo pupu"s);
        ASSERT_EQUAL(found_docs[0].rating, -5);
    }

    {
        SearchServer server;
        server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_3);
        const auto found_docs = server.FindTopDocuments("popo pupu"s);
        ASSERT_EQUAL(found_docs[0].rating, -2);
    }
}

void TestFilter() {
    const int doc_id_1 = 23;
    const string content_1 = "pipi pupu papa popo"s;
    const vector<int> ratings_1 = { 1, 4, 8, 8 };

    const int doc_id_2 = 33;
    const string content_2 = "pipi pupu"s;
    const vector<int> ratings_2 = { 1, 4, 8, 3 };

    {
        SearchServer server;
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        const auto found_docs = server.FindTopDocuments("pipi pupu"s,
            [](int document_id, DocumentStatus status, int rating)
            { return rating % 2 == 0; });
        int size = found_docs.size();
        ASSERT_EQUAL(size, 1);
        ASSERT_EQUAL(found_docs[0].id, doc_id_2);
    }
}

void TestFindDocumentStatus() {
    const int doc_id_1 = 23;
    const string content_1 = "pipi pupu papa popo"s;
    const vector<int> ratings_1 = { 1, 4, 8, 8 };

    const int doc_id_2 = 33;
    const string content_2 = "pipi pupu"s;
    const vector<int> ratings_2 = { 1, 4, 8, 3 };

    {
        SearchServer server;
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        const auto found_docs = server.FindTopDocuments("pipi pupu"s, DocumentStatus::BANNED);
        int size = found_docs.size();
        ASSERT_EQUAL(size, 0);
    }

    {
        SearchServer server;
        server.AddDocument(doc_id_1, content_1, DocumentStatus::BANNED, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::BANNED, ratings_2);
        const auto found_docs = server.FindTopDocuments("pipi pupu"s, DocumentStatus::BANNED);
        int size = found_docs.size();
        ASSERT_EQUAL(size, 2);
    }

    {
        SearchServer server;
        server.AddDocument(doc_id_1, content_1, DocumentStatus::BANNED, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::IRRELEVANT, ratings_2);
        const auto found_docs = server.FindTopDocuments("pipi pupu"s, DocumentStatus::IRRELEVANT);
        int size = found_docs.size();
        ASSERT_EQUAL(size, 1);
    }
}

void TestComputeDocumentRelevance() {
    const int doc_id_1 = 23;
    const string content_1 = "pipi pipi pupu"s;
    const vector<int> ratings_1 = {};

    const int doc_id_2 = 33;
    const string content_2 = "pipi papa pupu"s;

    const int doc_id_3 = 43;
    const string content_3 = "popo"s;

    {
        SearchServer server;
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_1);
        server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_1);
        const auto found_docs_1 = server.FindTopDocuments("pipi"s);
        double t1 = 0.27031;
        double t2 = 0.135155;
        ASSERT(abs(found_docs_1[0].relevance - t1) < EXP);
        ASSERT(abs(found_docs_1[1].relevance - t2) < EXP);
    }
}


// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestMatchDocument);
    RUN_TEST(TestSortDocumentRelevance);
    RUN_TEST(TestComputeRating);
    RUN_TEST(TestFilter);
    RUN_TEST(TestFindDocumentStatus);
    RUN_TEST(TestComputeDocumentRelevance);
}

// --------- Окончание модульных тестов поисковой системы -----------
