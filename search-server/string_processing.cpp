#include "string_processing.h"

// Функция преобразует разделенный пробелами текст в вектор строк-вью
std::vector<std::string_view> SplitIntoWordsView(std::string_view str_v)
{
    using namespace std::string_literals;

    std::vector<std::string_view> result;

    str_v.remove_prefix(0);
    const int64_t pos_end = str_v.npos;
    while (true)
    {
        // Убираем лидирующие пробелы
        while ((!str_v.empty()) && (str_v.front() == ' '))
        {
            str_v.remove_prefix(1);
        }

        int64_t space = str_v.find(' ');
        result.push_back(space == pos_end ? str_v.substr() : str_v.substr(0, space));
        if (space == pos_end)
        {
            break;
        }
        else
        {
            //pos = space + 1;
            str_v.remove_prefix(space + 1);
        }
    }

    return result;
}
