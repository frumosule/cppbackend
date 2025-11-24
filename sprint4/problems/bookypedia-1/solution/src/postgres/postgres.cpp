#include "postgres.h"

#include <pqxx/pqxx>
#include <pqxx/zview.hxx>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

void AuthorRepositoryImpl::Save(const domain::Author& author) {
    // Пока каждое обращение к репозиторию выполняется внутри отдельной транзакции
    // В будущих уроках вы узнаете про паттерн Unit of Work, при помощи которого сможете несколько
    // запросов выполнить в рамках одной транзакции.
    // Вы также может самостоятельно почитать информацию про этот паттерн и применить его здесь.
    pqxx::work work{connection_};
    work.exec_params(
        R"(
INSERT INTO authors (id, name) VALUES ($1, $2)
ON CONFLICT (id) DO UPDATE SET name=$2;
)"_zv,
        author.GetId().ToString(), author.GetName());
    work.commit();
}

std::vector<domain::Author> AuthorRepositoryImpl::GetAuthors() {
    pqxx::read_transaction trans{connection_};
    std::vector<domain::Author> result;
    for (auto [id, name] : trans.query<std::string, std::string>(R"(
SELECT id, name FROM authors ORDER BY name ASC
)"_zv)) {
        result.emplace_back(domain::AuthorId::FromString(id), std::move(name));
    }
    return result;
}

std::vector<domain::Book> BookRepositoryImpl::GetBooks() {
    pqxx::read_transaction trans{ connection_ };
    std::vector<domain::Book> result;
    for (auto [id, author_id, title, year] : trans.query<std::string, std::string, std::string, int>(R"(
SELECT id, author_id, title, publication_year FROM books ORDER BY title ASC
)"_zv)) {
        result.emplace_back(
            domain::BookId::FromString(id)
            , domain::AuthorId::FromString(author_id)
            , std::move(title)
            , year
        );
    }
    return result;
}

void BookRepositoryImpl::Save(const domain::Book& book) {
    pqxx::work work{ connection_ };
    work.exec_params(
        R"(
INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4)
ON CONFLICT (id) DO UPDATE SET author_id=$2, title=$3, publication_year=$4;
)"_zv,
        book.GetId().ToString(), book.GetAuthor().ToString(), book.GetTitle(), book.GetYear());
    work.commit();
}

std::vector<domain::Book> BookRepositoryImpl::GetBooksByAuthor(const domain::AuthorId& id) {
    pqxx::read_transaction trans{ connection_ };
    std::vector<domain::Book> result;
    for (auto [book_id, author_id, title, year] : trans.query<std::string, std::string, std::string, int>(R"(
SELECT id, author_id, title, publication_year FROM books WHERE author_id=')" 
+ id.ToString() + R"(' ORDER BY publication_year ASC, title ASC
)")) {
        result.emplace_back(
            domain::BookId::FromString(book_id)
            , domain::AuthorId::FromString(author_id)
            , std::move(title)
            , year
        );
    }
    return result;
}

Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)} {
    pqxx::work work{connection_};
    work.exec(R"(
CREATE TABLE IF NOT EXISTS authors (
    id UUID CONSTRAINT author_id_constraint PRIMARY KEY,
    name varchar(100) UNIQUE NOT NULL
);
)"_zv);

    work.exec(R"(
CREATE TABLE IF NOT EXISTS books (
    id UUID CONSTRAINT book_id_constraint PRIMARY KEY,
    author_id UUID NOT NULL,
    title varchar(100) NOT NULL,
    publication_year integer
);
)"_zv);

    // коммитим изменения
    work.commit();
}

}  // namespace postgres