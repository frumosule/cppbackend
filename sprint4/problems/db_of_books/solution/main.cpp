#include <cctype>
#include <iostream>
#include <optional>
#include <pqxx/pqxx>
#include <string_view>
#include <string>

using namespace std::literals;
using pqxx::operator"" _zv;

constexpr auto tag_add_book = "add_book"_zv;
constexpr auto get_books_query = "SELECT * FROM books ORDER BY year DESC, title ASC, author ASC, ISBN ASC;"_zv;

void InitializeDB(pqxx::connection& conn) {
    pqxx::work w(conn);
    w.exec("CREATE TABLE IF NOT EXISTS books (id SERIAL PRIMARY KEY, title varchar(100) NOT NULL, author varchar(100) NOT NULL, year integer NOT NULL, ISBN char(13) UNIQUE);"_zv);
    w.commit();
    conn.prepare(tag_add_book, "INSERT INTO books (title, author, year, ISBN) VALUES ($1, $2, $3, $4);"_zv); 
}

enum Action {
    ADD_BOOK,
    ALL_BOOKS,
    EXIT
};

struct PayloadData {
    std::string title;
    std::string author;
    int year;
    std::optional<std::string> isbn;
};

std::string_view FindAndParseNullable(std::string_view key, std::string_view query) {
    size_t key_pos = query.find(key);
    size_t key_end_pos = query.find("\"", key_pos + 1);
    size_t value_pos = query.find("\"", key_end_pos + 1);
    if (value_pos == query.npos) {
        return ""sv;
    }
    size_t value_end_pos = query.find("\"", value_pos + 1);
    return query.substr(value_pos + 1, value_end_pos - value_pos - 1);
}

std::string_view FindAndParse(std::string_view key, std::string_view query) {
    size_t key_pos = query.find(key);
    size_t key_end_pos = query.find("\"", key_pos + 1);
    size_t value_pos = query.find("\"", key_end_pos + 1);
    if (value_pos == query.npos) {
        return ""sv;
    }
    size_t value_end_pos = query.find("\"", value_pos + 1);
    return query.substr(value_pos + 1, value_end_pos - value_pos - 1);
}

int FindAndParseInt(std::string_view key, std::string_view query) {
    size_t key_pos = query.find(key);
    size_t key_end_pos = query.find("\"", key_pos + 1);
    size_t value_pos = query.find(":", key_end_pos);
    std::string parsed = "";
    bool number_ended = false;
    bool number_started = false;
    while (!number_ended) {
        if (std::isdigit(query.at(value_pos)) != 0) {
            if (!number_started) {
                number_started = true;
            }
            parsed += query.at(value_pos);
        }
        else {
            if (number_started) {
                number_ended = true;
            }
        }
        ++value_pos;
    }
    return std::stoi(parsed);
}

std::pair<Action, PayloadData> ParseQuery(const std::string& query) {
    auto action = FindAndParse("\"action\"", query);
    if (action == "add_book"sv) {
        auto title = FindAndParse("\"title\"", query);
        auto author = FindAndParse("\"author\"", query);
        auto year = FindAndParseInt("\"year\"", query);
        std::optional<std::string> isbn;
        auto isbn_parsed = FindAndParse("\"ISBN\"", query);
        if (isbn_parsed.size() != 0) {
            isbn.emplace(isbn_parsed.data(), isbn_parsed.size());
        }
        return { Action::ADD_BOOK, PayloadData{ std::string(title.data(), title.size()), std::string(author.data(), author.size()), static_cast<int>(year), isbn } };
    }
    if (action == "all_books") {
        return { Action::ALL_BOOKS, PayloadData{} };
    }
    if (action == "exit") {
        return { Action::EXIT, PayloadData{} };
    }
    throw std::exception();
}

int main(int argc, const char* argv[]) {
    try {
        if (argc == 1) {
            std::cout << "Usage: connect_db <conn-string>\n"sv;
            return EXIT_SUCCESS;
        } else if (argc != 2) {
            std::cerr << "Invalid command line\n"sv;
            return EXIT_FAILURE;
        }
        pqxx::connection conn{argv[1]};
        InitializeDB(conn);
        
        std::string query;
        while (std::getline(std::cin, query)) {
            auto [action, data] = ParseQuery(query);
            switch (action) {
                case Action::ADD_BOOK: 
                {
                    if (data.isbn->size() > 13) {
                        continue;
                    }
                    pqxx::work w(conn);
                    //throw std::runtime_error(data.title + " + " + data.author + " + " + *data.isbn + " :: " + query);
                    auto result = w.exec_prepared(tag_add_book, data.title, data.author, data.year, data.isbn);
                    w.commit();
                    std::cout << "{\"result\":";
                    if (result.affected_rows() > 0) {
                        std::cout << "true";
                    }
                    else {
                        std::cout << "false";
                    }
                    std::cout << "}" << std::endl;
                    break;
                }
                case Action::ALL_BOOKS:
                {
                    pqxx::read_transaction r(conn);
                    std::cout << "[";
                    bool first = true;
                    for (auto [id, title, author, year, isbn] 
                    : r.query<int, std::string, std::string, int, std::optional<std::string>>(get_books_query)) {
                        if (!first) {
                            std::cout << ",";
                        }
                        else {
                            first = false;
                        }
                        std::cout << "{\"id\":" << id << ",\"title\":\"" << title;
                        std::cout << "\",\"author\":\"" << author << "\",\"year\":" << std::to_string(year);
                        std::cout << ",\"ISBN\":";
                        if (isbn) {
                            std::cout << "\"" << *isbn << "\"";
                        }
                        else {
                            std::cout << "null";
                        }
                        std::cout << "}";
                    }
                    std::cout << "]" << std::endl;
                    break;
                }
                case Action::EXIT:
                    return 0;
            }
        }
    } 
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}