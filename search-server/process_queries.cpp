#include "process_queries.h"

#include <algorithm>
#include <execution>
#include <functional>
#include <numeric>
#include <vector>
#include <string>


std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries)
{

    std::vector<std::vector<Document>> results(queries.size());

    std::transform(std::execution::par, // от версии С++17
                queries.begin(), queries.end(),  // входной диапазон 1
                results.begin(),             // входной диапазон 2
                [&search_server](const std::string& param)    // при захвате &search_server в C++20 алгоритм медленнее в 1,5 раза дефолтного!
                {
                    return search_server.FindTopDocuments(param);
                }); 

    return results;
}

std::vector<std::vector<Document>> DefaultProcess(const SearchServer& search_server, const std::vector<std::string>& queries)
{
    std::vector<std::vector<Document>> results(queries.size());

    for (const std::string& query : queries)
    {
        results.push_back(search_server.FindTopDocuments(query));
    }

    return results;
}


std::vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries)
{
    std::vector<Document> results_v;

    for (const auto& vector : ProcessQueries(search_server, queries))
    {
        for (const auto& document : vector)
        {
            results_v.push_back(document);
        }
    }

    return results_v;
}