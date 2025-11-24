#pragma once
#include <pqxx/connection>
#include <pqxx/transaction>

#include "../domain/author.h"
#include "../domain/book.h"

namespace postgres {

class AuthorRepositoryImpl : public domain::AuthorRepository {
public:
    AuthorRepositoryImpl(pqxx::work& work) 
        :work_{ work } {
    };

    void Save(const domain::Author& author) override;
    std::vector<domain::Author> GetAuthors() override;
    std::optional<domain::Author> GetAuthorIfExists(const std::string& name) override;
    void DeleteAuthor(const std::string& name) override;
    domain::Author GetAuthorById(const domain::AuthorId& id) override;

private:
    pqxx::work& work_;
};

class BookRepositoryImpl : public domain::BookRepository {
public:
    BookRepositoryImpl(pqxx::work& work)
        :work_{ work } {
    };

    void Save(const domain::Book& book) override;
    std::vector<domain::Book> GetBooks() override; 
    std::vector<domain::Book> GetBooksByAuthor(const domain::AuthorId& id) override;
    std::vector<domain::Book> GetBooksByTitle(const std::string& title) override;
    void AddTags(const domain::BookId& id, const std::vector<std::string>& tags) override;
    void DeleteBooksOfAuthor(const domain::AuthorId& id) override;
    std::string GetTags(const domain::BookId& id) override;
    void DeleteBook(const domain::BookId& id) override;
    void DeleteTags(const domain::BookId& id) override;
    void EditBook(const domain::Book& book, const std::vector<std::string>& tags) override;


private:
    pqxx::work& work_;
};

class Database {
public:
    explicit Database(pqxx::connection connection);

    pqxx::connection& GetConnection() {
        return connection_;
    }

private:
    pqxx::connection connection_;
};

}  // namespace postgres