#pragma once
#include "../domain/author_fwd.h"
#include "../domain/book_fwd.h"
#include "use_cases.h"

namespace app {

class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(domain::AuthorRepository& authors, domain::BookRepository& books)
        : authors_{authors}, books_{books} {
    }

    void AddAuthor(const std::string& name) override;
    void AddBook(const domain::AuthorId& author, const std::string& title, int publication_year) override;
    std::vector<domain::Author> GetAuthors() override;
    std::vector<domain::Book> GetBooks() override;
    std::vector<domain::Book> GetBooksByAuthor(const domain::AuthorId& id) override;

private:
    domain::AuthorRepository& authors_;
    domain::BookRepository& books_;
};

}  // namespace app