#pragma once
#include <string>

#include "author.h"
#include "../util/tagged_uuid.h"

namespace domain {

    namespace detail {
        struct BookTag {};
    }  // namespace detail

    using BookId = util::TaggedUUID<detail::BookTag>;

    class Book {
    public:
        Book(BookId id, AuthorId author, std::string title, int publication_year)
            : id_(std::move(id))
            , author_(std::move(author))
            , title_(std::move(title))
            , publication_year_(publication_year){
        }

        const BookId& GetId() const noexcept {
            return id_;
        }

        const std::string& GetTitle() const noexcept {
            return title_;
        }

        const AuthorId& GetAuthor() const noexcept {
            return author_;
        }

        int GetYear() const noexcept {
            return publication_year_;
        }

    private:
        BookId id_;
        AuthorId author_;
        std::string title_;
        int publication_year_;
    };

    class BookRepository {
    public:
        virtual void Save(const Book& book) = 0;
        virtual std::vector<Book> GetBooks() = 0;
        virtual std::vector<Book> GetBooksByAuthor(const AuthorId& id) = 0;

    protected:
        ~BookRepository() = default;
    };

}  // namespace domain