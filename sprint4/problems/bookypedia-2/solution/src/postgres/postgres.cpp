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
    work_.exec_params(R"(
INSERT INTO authors (id, name) VALUES ($1, $2)
ON CONFLICT (id) DO UPDATE SET name=$2;
)"_zv,
        author.GetId().ToString(), author.GetName());
}

std::vector<domain::Author> AuthorRepositoryImpl::GetAuthors() {
    std::vector<domain::Author> result;
    pqxx::result data = work_.exec(R"(
SELECT id, name FROM authors ORDER BY name ASC
)"_zv);
    for (const auto& row : data) {
        auto [id, name] = row.as<std::string, std::string>();
        result.emplace_back(domain::AuthorId::FromString(id), std::move(name));
    }
    return result;
}

std::optional<domain::Author> AuthorRepositoryImpl::GetAuthorIfExists(const std::string& name) {
    try {
        auto row = work_.exec_params1(R"(
SELECT id, name FROM authors WHERE name=$1
)"_zv, name);
        auto [id, name] = row.as<std::string, std::string>();
        return domain::Author{domain::AuthorId::FromString(id), std::move(name)};
    }
    catch (pqxx::unexpected_rows) {
        return {};
    }
}

void AuthorRepositoryImpl::DeleteAuthor(const std::string& name) {
    work_.exec_params(R"(
DELETE FROM authors WHERE name=$1
)", name);
}

domain::Author AuthorRepositoryImpl::GetAuthorById(const domain::AuthorId& id) {
    auto row = work_.exec_params1(R"(
SELECT name FROM authors WHERE id=$1
)"_zv, id.ToString());
    auto name = std::get<0>(row.as<std::string>());
    return domain::Author{ id, std::move(name)};
}

std::vector<domain::Book> BookRepositoryImpl::GetBooks() {
    std::vector<domain::Book> result;
    pqxx::result data = work_.exec(R"(
SELECT books.id, author_id, title, publication_year, authors.name FROM books 
JOIN authors ON authors.id = author_id
ORDER BY title ASC, authors.name ASC
)"_zv);
    for (const auto& row : data) {
        auto [id, author_id, title, year, author_name] = row.as<std::string, std::string, std::string, int, std::string>();
        result.emplace_back(
            domain::BookId::FromString(id)
            , domain::AuthorId::FromString(author_id)
            , std::move(title)
            , year
        );
        result.back().SetAuthorName(author_name);
    }
    return result;
}

void BookRepositoryImpl::Save(const domain::Book& book) {
    work_.exec_params(R"(
INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4)
ON CONFLICT (id) DO UPDATE SET author_id=$2, title=$3, publication_year=$4;
)"_zv,
        book.GetId().ToString(), book.GetAuthor().ToString(), book.GetTitle(), book.GetYear());
}

std::vector<domain::Book> BookRepositoryImpl::GetBooksByAuthor(const domain::AuthorId& id) {
    std::vector<domain::Book> result;
    pqxx::result data = work_.exec_params(R"(
SELECT books.id, author_id, title, publication_year, authors.name FROM books 
JOIN authors ON authors.id = author_id
WHERE author_id=$1
ORDER BY publication_year ASC, title ASC
)", id.ToString());
    for (const auto& row : data) {
        auto [id, author_id, title, year, author_name] = row.as<std::string, std::string, std::string, int, std::string>();
        result.emplace_back(
            domain::BookId::FromString(id)
            , domain::AuthorId::FromString(author_id)
            , std::move(title)
            , year
        );
        result.back().SetAuthorName(author_name);
    }
    return result;
}

std::vector<domain::Book> BookRepositoryImpl::GetBooksByTitle(const std::string& title) {
    std::vector<domain::Book> result;
    pqxx::result data = work_.exec_params(R"(
SELECT books.id, author_id, title, publication_year, authors.name FROM books 
JOIN authors ON authors.id = author_id
WHERE title=$1
ORDER BY authors.name ASC, publication_year ASC
)", title);
    for (const auto& row : data) {
        auto [id, author_id, title, year, author_name] = row.as<std::string, std::string, std::string, int, std::string>();
        result.emplace_back(
            domain::BookId::FromString(id)
            , domain::AuthorId::FromString(author_id)
            , std::move(title)
            , year
        );
        result.back().SetAuthorName(author_name);
    }
    return result;
}

void BookRepositoryImpl::AddTags(const domain::BookId& id, const std::vector<std::string>& tags) {
    auto id_str = id.ToString();
    for (const auto& tag : tags) {
        work_.exec_params(R"(
INSERT INTO book_tags (book_id, tag) VALUES ($1, $2)
)"_zv, id_str, tag);
    }
}

void BookRepositoryImpl::DeleteBooksOfAuthor(const domain::AuthorId& id) {
    for (const auto& book : GetBooksByAuthor(id)) {
        work_.exec_params(R"(
DELETE FROM book_tags WHERE book_id=$1
)", book.GetId().ToString());
    }
    work_.exec_params(R"(
DELETE FROM books WHERE author_id=$1
)", id.ToString());
}

//std::vector<std::string> BookRepositoryImpl::GetTags(const domain::BookId& id) {
//    std::vector<std::string> result;
//    auto tags = work_.exec_params(R"(
//SELECT tag FROM book_tags WHERE book_id=$1
//)", id.ToString());
//    for (const auto& row : tags) {
//        result.emplace_back(std::get<0>(row.as<std::string>()));
//    }
//    return result;
//}

std::string BookRepositoryImpl::GetTags(const domain::BookId& id) {
    std::string result = "";
    auto tags = work_.exec_params(R"(
SELECT tag FROM book_tags WHERE book_id=$1
)", id.ToString());
    bool first = true;
    for (const auto& row : tags) {
        if (first) {
            first = false;
        }
        else {
            result += ", ";
        }
        result += std::get<0>(row.as<std::string>());
    }
    return result;
}

void BookRepositoryImpl::DeleteBook(const domain::BookId& id) {
    DeleteTags(id);
    work_.exec_params(R"(
DELETE FROM books WHERE id=$1
)", id.ToString());
}

void BookRepositoryImpl::DeleteTags(const domain::BookId& id) {
    work_.exec_params(R"(
DELETE FROM book_tags WHERE book_id=$1
)", id.ToString());
}

void BookRepositoryImpl::EditBook(const domain::Book& book, const std::vector<std::string>& tags) {
    DeleteTags(book.GetId());
    Save(book);
    if (!tags.empty()) {
        AddTags(book.GetId(), tags);
    }
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
    id UUID PRIMARY KEY,
    author_id UUID NOT NULL,
    title varchar(100) NOT NULL,
    publication_year integer,
    CONSTRAINT fk_authors
        FOREIGN KEY(author_id)
        REFERENCES authors(id)
);
)"_zv);
    //UNIQUE (author_id, title, publication_year),
    work.exec(R"(
CREATE TABLE IF NOT EXISTS book_tags (
    book_id UUID NOT NULL,
    tag varchar(30) NOT NULL,
    CONSTRAINT fk_books
        FOREIGN KEY(book_id)
        REFERENCES books(id)
);
)"_zv);

    // коммитим изменения
    work.commit();
}

}  // namespace postgres