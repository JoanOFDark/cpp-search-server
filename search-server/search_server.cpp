#include <cmath>
#include <numeric>
#include <algorithm>

#include "string_processing.h"
#include "search_server.h"
#include "read_input_functions.h"


SearchServer::SearchServer(const std::string& stop_words_text)  // Invoke delegating constructor
    : SearchServer(SplitIntoWordsView(stop_words_text))             // from string container
{}


SearchServer::SearchServer(const std::string_view stop_words_view)  // Invoke delegating constructor
    : SearchServer(SplitIntoWordsView(stop_words_view))           // from string container
{}


void SearchServer::AddDocument(int document_id, std::string_view document, DocumentStatus status,
    const std::vector<int>& ratings)
{
    using namespace std::string_literals;

    if ((document_id < 0) || (documents_.count(document_id) > 0))
    {
        throw std::invalid_argument("Invalid document_id"s);
    }

    const auto [it, inserted] = documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status, std::string(document) });
    const auto words = SplitIntoWordsNoStop(it->second.doc_text);

    const double inv_word_count = 1.0 / words.size();
    for (std::string_view word : words)
    {
        word_to_document_freqs_[word][document_id] += inv_word_count;

        // Заполняем дополнительный словарь для подсистемы поиска дубликатов (кэш подсистемы)
        document_to_words_[document_id][word] += inv_word_count;
    }
    document_ids_.emplace(document_id);
}


std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const
{
    return FindTopDocuments(std::execution::seq,
        raw_query, [status](int document_id, DocumentStatus document_status, int rating)
        {
            return document_status == status;
        });
}


std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const
{
    return FindTopDocuments(std::execution::seq, raw_query, DocumentStatus::ACTUAL);
}


int SearchServer::GetDocumentCount() const
{
    return documents_.size();
}

std::set<int>::const_iterator SearchServer::begin() const
{
    return document_ids_.begin();
}


std::set<int>::const_iterator SearchServer::end() const
{
    return document_ids_.end();
}

SearchServer::Query SearchServer::ParseQuery(std::string_view text, bool sort_check = true) const
{
    // Разбиваем запрос на слова
    const std::vector<std::string_view> query_words = SplitIntoWordsView(text);

    // Резервируем память только для плюс-слов
    SearchServer::Query result;
    result.plus_words.reserve(query_words.size());

    for (std::string_view word : query_words)
    {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop)
        {
            if (query_word.is_minus)
            {
                result.minus_words.push_back(query_word.data);
            }
            else
            {
                result.plus_words.push_back(query_word.data);
            }
        }
    }

    if (sort_check)
    {
        result.SortUniq();
    }

    return result;
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query,
    int document_id) const
{
    using namespace std::string_literals;
    if (documents_.count(document_id) == 0)
    {
        throw std::out_of_range("Invalid document_id"s);
    }

    // MatchDocument() без политики - вызываем последовательную версию
    const auto query = ParseQuery(raw_query);

    // Сначала проверим минус-слова.
    for (std::string_view word : query.minus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id) != 0)
        {
            // Минус-слово из запроса есть в документе. Выходим с пустым результатом.
            return { {}, documents_.at(document_id).status };
        }
    }

    std::vector<std::string_view> matched_words;

    for (std::string_view word : query.plus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id) != 0)
        {
            matched_words.push_back(word);
        }
    }

    return { std::vector<std::string_view>(matched_words.begin(), matched_words.end()), documents_.at(document_id).status };
}


std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument([[maybe_unused]] std::execution::sequenced_policy policy,
    std::string_view raw_query,
    int document_id)
{
    // Тело метода дублирует код метода без политик выполнения
    return SearchServer::MatchDocument(raw_query, document_id);
}


std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::parallel_policy policy,
    std::string_view raw_query,
    int document_id)
{
    using namespace std::string_literals;
    if (!documents_.count(document_id))
    {
        throw std::out_of_range("Invalid document_id"s);
    }

    // Получаем векторы плюс- и минус-слов параллельным алгоритмом
    const auto query = ParseQuery(raw_query, false);

    // Проверяем, есть ли среди минус-слов хотя бы 1, входящее в текущий документ
    if (std::any_of(std::execution::par, query.minus_words.cbegin(), query.minus_words.cend(),
        [this, document_id](std::string_view word)
        {
            const auto it = word_to_document_freqs_.find(word);

            // Если минус слово есть среди слов сервера И в заданном документе это слово встечается (== 1)  => true
            return ((it != word_to_document_freqs_.end()) && (it->second.count(document_id)));
        })
        )
    {
        // В запросе есть хотя бы 1 минус-слово, встречающееся в текущем документе.
        // Возвращаем пустой ответ
        return { {}, documents_.at(document_id).status };
    }

        ///////////////////////////////////////////////
        // Если мы здесь, то минус-слов в документе нет

        // Пустой вектор совпавших слов
        std::vector<std::string_view> matched_words{};

        // Резервируем память (не более чем количество плюс-слов в запросе)
        matched_words.reserve(query.plus_words.size());

        // Матчинг плюс-слов, версия 2 для последовательной реализации
        const auto& this_doc_words = document_to_words_.at(document_id);
        for (std::string_view word : query.plus_words)
        {
            if (this_doc_words.count(word))
            {
                matched_words.push_back(word);
            }
        }

        // Удаляем дубликаты из результатов матчинга
        std::sort(std::execution::par, matched_words.begin(), matched_words.end());
        auto last = std::unique(std::execution::par, matched_words.begin(), matched_words.end());
        last = matched_words.erase(last, matched_words.end());

        return { matched_words, documents_.at(document_id).status };
}


void SearchServer::RemoveDocument(int document_id)
{
    if (document_ids_.count(document_id) == 1) {
        for (auto [word, freq] : GetWordFrequencies(document_id)) {
            word_to_document_freqs_[word].erase(document_id);
            if (word_to_document_freqs_.count(word) == 1 && word_to_document_freqs_.at(word).size() == 0) {
                std::string s_word{ word };
                word_to_document_freqs_.erase(s_word);
            }
        }
        document_ids_.erase(document_id);
        documents_.erase(document_id);
        document_to_words_.erase(document_id);
    }
    return;
}


const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const
{
    // Возвращаем пустой map, если не находим документов
    const std::map<std::string_view, double> empty_word_freqs_;

    if (documents_.count(document_id) > 0)
    {
        return document_to_words_.at(document_id);
    }

    return empty_word_freqs_;
}

SearchServer::Query::Query(size_t size_plus, size_t size_minus) : plus_words(size_plus), minus_words(size_minus)
{}

SearchServer::Query::Query(size_t size) : plus_words(size), minus_words(size)
{}

void SearchServer::Query::SortUniq(PlusMinusWords words)
{
    if (words == PlusMinusWords::PLUS_WORDS)
    {
        // Сортировка словаря плюс-слов
        std::sort(std::execution::par, plus_words.begin(), plus_words.end());
        auto last = std::unique(std::execution::par, plus_words.begin(), plus_words.end());
        last = plus_words.erase(last, plus_words.end());
    }
    else
    {
        // Сортировка словаря минус-слов
        std::sort(std::execution::par, minus_words.begin(), minus_words.end());
        auto last = std::unique(std::execution::par, minus_words.begin(), minus_words.end());
        last = minus_words.erase(last, minus_words.end());
    }
}

// Сортировка
void SearchServer::Query::SortUniq()
{
    SortUniq(PlusMinusWords::PLUS_WORDS);
    SortUniq(PlusMinusWords::MINUS_WORDS);
}

bool SearchServer::IsStopWord(std::string_view word) const
{
    return stop_words_.count(word) > 0;
}


bool SearchServer::IsValidWord(std::string_view word)
{
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c)
        {
            return c >= '\0' && c < ' ';
        });
}


std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(std::string_view text) const
{
    using namespace std::string_literals;

    std::vector<std::string_view> words;
    for (std::string_view word : SplitIntoWordsView(text))
    {
        if (!IsValidWord(word))
        {
            throw std::invalid_argument("Word "s + word.data() + " is invalid"s);
        }
        if (!IsStopWord(word))
        {
            words.push_back(word);
        }
    }
    return words;
}


int SearchServer::ComputeAverageRating(const std::vector<int>& ratings)
{
    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);

    return rating_sum / static_cast<int>(ratings.size());
}


SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const
{
    using namespace std::string_literals;

    if (text.empty())
    {
        throw std::invalid_argument("Query word is empty"s);
    }
    std::string_view word = text;
    bool is_minus = false;
    if (word[0] == '-')
    {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word))
    {
        throw std::invalid_argument("Query word "s + text.data() + " is invalid"s);
    }

    return { word, is_minus, IsStopWord(word) };
}


// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(std::string_view word) const
{
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}