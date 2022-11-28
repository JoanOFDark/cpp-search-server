#pragma once

// #include ��� type resolution � ����������� �������:
#include <string>
#include <string_view>
#include <algorithm>
#include <stdexcept>
#include <set>
#include <vector>
#include <tuple>
#include <map>
#include <iterator>
#include <execution>    // ��� std::execution::parallel_policy
#include <mutex>
#include <type_traits>
#include <future>

#include <ostream>      // ��� ������
#include <iostream>     // ��� ������
#include <ios>          // ��� ������

#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;

// ��������� �������� ��������� ������������ ����� (�������� �������������)
const double EPSILON = 1e-6;

// ����� ������ ��� ��������� ������������� ��������
const size_t BUCKETS_NUM = 8;

class SearchServer
{
public:
    // ��������� ����������� �� ������ ���������� �� ����-�������
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);

    // ����������� �� ������ ������ �� ����-������� (�������� ��������� �����������)
    explicit SearchServer(const std::string&);

    // ����������� �� ������ string_view �� ����-������� (�������� ��������� �����������)
    explicit SearchServer(const std::string_view);

    // ����� ��������� ����� �������� � ���� ������ ���������� �������
    void AddDocument(int, std::string_view, DocumentStatus, const std::vector<int>&);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view, DocumentPredicate) const;
    template <class ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&&, std::string_view, DocumentPredicate) const;

    template <class ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&&, std::string_view, DocumentStatus) const;
    std::vector<Document> FindTopDocuments(std::string_view, DocumentStatus) const;

    template <class ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&&, std::string_view) const;
    std::vector<Document> FindTopDocuments(std::string_view) const;

    int GetDocumentCount() const;

    int GetDocumentId(int) const;

    std::set<int>::const_iterator begin() const;

    std::set<int>::const_iterator end() const;

    using Match = std::tuple<std::vector<std::string_view>, DocumentStatus>;
    Match MatchDocument(std::string_view, int) const;
    // ������ MatchDocument() ��� �������� ����������������� ����������
    Match MatchDocument(std::execution::sequenced_policy, std::string_view, int);
    // ������ MatchDocument() ��� �������� ������������� ����������
    Match MatchDocument(std::execution::parallel_policy, std::string_view, int);

    // ����� ������� �������� ��� ��������� id ��� ���� �����������
    void RemoveDocument(int);

    // ������ RemoveDocument() � ���������� ������������� ����������
    template <class ExecutionPolicy>
    void RemoveDocument(ExecutionPolicy&&, int);

    // ����� ���������� ������� ������� ���� ��� ��������� � ��������� id
    const std::map<std::string_view, double>& GetWordFrequencies(int) const;

private:
    struct DocumentData
    {
        int rating;
        DocumentStatus status;
        std::string doc_text;   // �������� ������ ���������. �� �� ������ �������������� string_view
    };

    struct QueryWord
    {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    // ��������� ������� ��� ������� � ���������������� ����������
    struct Query
    {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;

        Query() = default;

        explicit Query(size_t, size_t);

        explicit Query(size_t);

        enum class PlusMinusWords {
            PLUS_WORDS,
            MINUS_WORDS
        };

        // ��������� ���������� � ���������� ���������� �������� � ������� (�������� set).
        // PlusMinusWords::PLUS_WORDS ��� ���������� ����-����, PlusMinusWords::MINUS_WORDS ��� ���������� �����-����
        void SortUniq(PlusMinusWords pmw); //�� ������ ����� ��� ������� ���� �����))
                                           //������� ���� ���������� � ��� ����������?
                                           //��� ��������� ������?

        //��������� ���������� � ���������� ���������� �������� � ������� (�������� set).
        void SortUniq();
    };

    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;

    //NEW
    // ������� "����� ��������� - ������� ������� ��� ����"
    std::map<int, std::map<std::string_view, double>> document_to_words_;

    bool IsStopWord(std::string_view) const;

    static bool IsValidWord(std::string_view);

    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view) const;

    static int ComputeAverageRating(const std::vector<int>&);

    QueryWord ParseQueryWord(std::string_view) const;

    Query ParseQuery(std::string_view, bool sort_check) const;

    double ComputeWordInverseDocumentFreq(std::string_view word) const;

    // ������������������ ������ ��� ����������������� ����������
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(std::execution::sequenced_policy,
        const Query&,
        DocumentPredicate) const;
    // ������������������ ������ ��� ������������� ����������
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(std::execution::parallel_policy,
        const Query&,
        DocumentPredicate) const;
    // ������ ������� ��� ������ ��� �������� �������� ���������� (�������� seq-������)
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query&,
        DocumentPredicate) const;
};


// ����������� ��������� ������� (�������� ��� ������)

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
{
    using namespace std::string_literals;

    if (!std::all_of(stop_words_.begin(), stop_words_.end(), IsValidWord))
    {
        throw std::invalid_argument("Some of stop words are invalid"s);
    }

}


template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query,
    DocumentPredicate document_predicate) const
{
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}


template <class ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy,
    std::string_view raw_query,
    DocumentPredicate document_predicate) const
{
    const auto query = ParseQuery(raw_query, std::is_same_v<ExecutionPolicy, std::execution::parallel_policy>);

    auto matched_documents = FindAllDocuments(policy, query, document_predicate);

    std::sort(policy, matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs)
        {
            return lhs.relevance > rhs.relevance
                || (std::abs(lhs.relevance - rhs.relevance) < EPSILON && lhs.rating > rhs.rating);
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
    {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}


template <class ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query, DocumentStatus status) const
{
    return FindTopDocuments(policy,
        raw_query, [status](int document_id, DocumentStatus document_status, int rating)
        {
            return document_status == status;
        });
}


template <class ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query) const
{
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}


template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(std::execution::sequenced_policy policy,
    const SearchServer::Query& query,
    DocumentPredicate document_predicate) const
{
    // ������ �����������
    std::vector<Document> matched_documents;

    // ����������� ������������ �������
    std::map<int, double> document_to_relevance;

    // ������������ ����-�����
    for (std::string_view word : query.plus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word))
        {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating))
            {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    // ������������ �����-�����, ������� �� ��������� ��������� � �����-�������
    for (std::string_view word : query.minus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word))
        {
            document_to_relevance.erase(document_id);
        }
    }

    // ��������� ������ � ���������� �����������
    for (const auto [document_id, relevance] : document_to_relevance)
    {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }

    return matched_documents;
}


template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(std::execution::parallel_policy policy,
    const SearchServer::Query& query,
    DocumentPredicate document_predicate) const
{
    // ������ �����������
    std::vector<Document> matched_documents;

    // ������� � ���������� ������������ ����������
    ConcurrentMap<int, double> document_to_relevance(BUCKETS_NUM);

    // ��������� ����-����
    // ��������� �������� � ���������� ���������������
    std::for_each(policy,
        query.plus_words.begin(), query.plus_words.end(),
        [this, &document_to_relevance, &document_predicate](std::string_view word)
        {
            // ���� ����-����� ���� � ������� ������ ����
            if (!word_to_document_freqs_.count(word) == 0)
            {
                const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word))
                {
                    const auto& document_data = documents_.at(document_id);
                    if (document_predicate(document_id, document_data.status, document_data.rating))
                    {
                        document_to_relevance[document_id] += term_freq * inverse_document_freq;
                    }
                }
            }
        }
    );

    // ��������� �����-����. ����������� ������� document_to_relevance, ����������� �� ����-������
    std::for_each(policy,
        query.minus_words.begin(), query.minus_words.end(),
        [this, &document_to_relevance](std::string_view word)
        {
            if (!word_to_document_freqs_.count(word) == 0)
            {
                for (const auto [document_id, _] : word_to_document_freqs_.at(word))
                {
                    // Erase � ConcurrentMap ����������������
                    document_to_relevance.Erase(document_id);
                }
            }
        }
    );

    for (const auto& [document_id, relevance] : document_to_relevance.BuildOrdinaryMap())
    {
        matched_documents.emplace_back(
            Document(document_id, relevance, documents_.at(document_id).rating)
        );
    }

    return matched_documents;
}


template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const SearchServer::Query& query,
    DocumentPredicate document_predicate) const
{
    return SearchServer::FindAllDocuments(std::execution::seq, query, document_predicate);
}


template <class ExecutionPolicy>
void SearchServer::RemoveDocument(ExecutionPolicy&& policy, int document_id)
{
    // ����� ������ ������ �� ����� ���������� ���������
    const auto& word_freqs = document_to_words_.at(document_id);  // map<string, double>

    // ������ ������ ���������� �� ����� ������������ �������
    std::vector<const std::string*> words(word_freqs.size());
    std::transform(
        policy,
        word_freqs.begin(), word_freqs.end(),
        words.begin(),
        [](const auto& word)
        {
            // word - map<string, double>
            // ������ ��������� �� �����
            return &word.first;
        });

    std::mutex mutex_;
    // ������� �����, ��������� ��������� �� ���
    std::for_each(policy, words.begin(), words.end(),
        [this, document_id, &mutex_](const std::string* word)
        {
            const std::lock_guard<std::mutex> lock(mutex_);
            word_to_document_freqs_.at(*word).erase(document_id);
        });

    documents_.erase(document_id);
    document_to_words_.erase(document_id);

}