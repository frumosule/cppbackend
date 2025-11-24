#pragma once

#include "../domain/author.h"
#include "../domain/book.h"

#include <string>
#include <memory>

namespace app {

class UnitOfWork {
public:
    virtual void Commit() = 0;
    virtual void Abort() = 0;
    virtual void AddAuthor(const std::string& name) = 0;
    virtual domain::Book AddBook(const domain::AuthorId& author, const std::string& title, int publication_year) = 0;
    virtual std::vector<domain::Author> GetAuthors() = 0;
    virtual std::vector<domain::Book> GetBooks() = 0;
    virtual std::vector<domain::Book> GetBooksByAuthor(const domain::AuthorId& id) = 0;
    virtual std::vector<domain::Book> GetBooksByTitle(const std::string& title) = 0;
    virtual std::optional<domain::Author> GetAuthorIfExists(const std::string& name) = 0;
    //virtual std::optional<domain::Book> GetBookIfExists(const std::string& title) = 0;
    virtual void AddTags(const domain::BookId& id, const std::vector<std::string>& tags) = 0;
    virtual void DeleteAuthor(const domain::Author& author) = 0;
    virtual void EditAuthor(const domain::Author& author) = 0;
    //virtual std::vector<std::string> GetTags(const domain::BookId& id) = 0;
    virtual std::string GetTags(const domain::BookId& id) = 0;
    virtual void DeleteBook(const domain::BookId& id) = 0;
    virtual void DeleteTags(const domain::BookId& id) = 0;
    virtual void EditBook(const domain::Book& book, const std::vector<std::string>& tags) = 0;
};

class UnitOfWorkFactory {
public:
    virtual std::unique_ptr<UnitOfWork> CreateUnit() = 0;
};

class UseCases {
public:
    virtual std::unique_ptr<UnitOfWork> GetUnit() = 0;

protected:
    
};

}  // namespace app