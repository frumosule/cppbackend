#include "use_cases_impl.h"

#include "../domain/author.h"
#include "../domain/book.h"

namespace app {
using namespace domain;

void UseCasesImpl::AddAuthor(const std::string& name) {
    authors_.Save({AuthorId::New(), name});
}

void UseCasesImpl::AddBook(const domain::AuthorId& author, const std::string& title, int publication_year) {
    books_.Save({BookId::New(), author, title, publication_year});
}

std::vector<domain::Author> UseCasesImpl::GetAuthors() {
    return authors_.GetAuthors();
}

std::vector<domain::Book> UseCasesImpl::GetBooks() {
    return books_.GetBooks();
}

std::vector<domain::Book> UseCasesImpl::GetBooksByAuthor(const domain::AuthorId& id) {
    return books_.GetBooksByAuthor(id);
}

}  // namespace app