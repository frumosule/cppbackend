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

        void SetAuthorName(const std::string& name) noexcept {
            author_name_ = name;
        }

        const std::string& GetAuthorName() const noexcept {
            return author_name_;
        }

        int GetYear() const noexcept {
            return publication_year_;
        }

    private:
        BookId id_;
        AuthorId author_;
        std::string author_name_ = "";
        std::string title_;
        int publication_year_;
    };

    class BookRepository {
    public:
        virtual void Save(const Book& book) = 0;
        virtual std::vector<Book> GetBooks() = 0;
        virtual std::vector<Book> GetBooksByAuthor(const AuthorId& id) = 0;
        virtual std::vector<domain::Book> GetBooksByTitle(const std::string& title) = 0;
        //virtual std::optional<Book> GetBookIfExists(const std::string& title) = 0;
        virtual void AddTags(const BookId& id, const std::vector<std::string>& tags) = 0;
        virtual void DeleteBooksOfAuthor(const AuthorId& id) = 0;
        //virtual std::vector<std::string> GetTags(const BookId& id) = 0;
        virtual std::string GetTags(const BookId& id) = 0;
        virtual void DeleteBook(const domain::BookId& id) = 0;
        virtual void DeleteTags(const domain::BookId& id) = 0;
        virtual void EditBook(const domain::Book& book, const std::vector<std::string>& tags) = 0;

    protected:
        ~BookRepository() = default;
    };

}  // namespace domain