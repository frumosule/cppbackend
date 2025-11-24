#include "use_cases_impl.h"

#include "../domain/author.h"
#include "../domain/book.h"

namespace app {
using namespace domain;

void UnitOfWorkImpl::Commit() {
    work_.commit();
}

void UnitOfWorkImpl::Abort() {
    work_.abort();
}

void UnitOfWorkImpl::AddAuthor(const std::string& name) {
    authors_.Save({ AuthorId::New(), name });
}

domain::Book UnitOfWorkImpl::AddBook(const domain::AuthorId& author, const std::string& title, int publication_year) {
    domain::Book book { BookId::New(), author, title, publication_year };
    books_.Save(book);
    return book;
}

std::vector<domain::Author> UnitOfWorkImpl::GetAuthors() {
    return authors_.GetAuthors();
}

std::vector<domain::Book> UnitOfWorkImpl::GetBooks() {
    auto books = books_.GetBooks();
    for (auto& book : books) {
        book.SetAuthorName(authors_.GetAuthorById(book.GetAuthor()).GetName());
    }
    return books;
}

std::vector<domain::Book> UnitOfWorkImpl::GetBooksByAuthor(const domain::AuthorId& id) {
    return books_.GetBooksByAuthor(id);
}

std::vector<domain::Book> UnitOfWorkImpl::GetBooksByTitle(const std::string& title) {
    return books_.GetBooksByTitle(title);
}

std::optional<domain::Author> UnitOfWorkImpl::GetAuthorIfExists(const std::string& name) {
    return authors_.GetAuthorIfExists(name);
}

void UnitOfWorkImpl::AddTags(const domain::BookId& id, const std::vector<std::string>& tags) {
    books_.AddTags(id, tags);
}

void UnitOfWorkImpl::DeleteAuthor(const domain::Author& author) {
    books_.DeleteBooksOfAuthor(author.GetId());
    authors_.DeleteAuthor(author.GetName());
}

void UnitOfWorkImpl::EditAuthor(const domain::Author& author) {
    authors_.Save(author);
}

//std::vector<std::string> UnitOfWorkImpl::GetTags(const domain::BookId& id) {
//    return books_.GetTags(id);
//}

std::string UnitOfWorkImpl::GetTags(const domain::BookId& id) {
    return books_.GetTags(id);
}

void UnitOfWorkImpl::DeleteBook(const domain::BookId& id) {
    books_.DeleteBook(id);
}

void UnitOfWorkImpl::DeleteTags(const domain::BookId& id) {
    books_.DeleteTags(id);
}

void UnitOfWorkImpl::EditBook(const domain::Book& book, const std::vector<std::string>& tags) {
    books_.EditBook(book, tags);
}

std::unique_ptr<UnitOfWork> UnitOfWorkFactoryImpl::CreateUnit() {
    return std::make_unique<UnitOfWorkImpl>(conn_);
}

std::unique_ptr<UnitOfWork> UseCasesImpl::GetUnit() {
    return factory_.CreateUnit();
}

}  // namespace app