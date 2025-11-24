#include "view.h"

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <sstream>
#include <string>

#include "../menu/menu.h"

using namespace std::literals;
namespace ph = std::placeholders;

std::ostream& operator<<(std::ostream& out, const domain::Book& book) {
    out << book.GetTitle() << " by " << book.GetAuthorName() << ", " << book.GetYear();
    return out;
}

namespace ui {
namespace detail {

std::ostream& operator<<(std::ostream& out, const AuthorInfo& author) {
    out << author.name;
    return out;
}

std::ostream& operator<<(std::ostream& out, const BookInfo& book) {
    out << book.title << ", " << book.publication_year;
    return out;
}

std::ostream& operator<<(std::ostream& out, const BookInfoWithAuthor& book) {
    out << book.title << " by " << book.author << ", " << book.publication_year;
    return out;
}

}  // namespace detail

template <typename T>
void PrintVector(std::ostream& out, const std::vector<T>& vector) {
    int i = 1;
    for (auto& value : vector) {
        out << i++ << " " << value << std::endl;
    }
}

std::string RemoveExtraSpaces(const std::string& input) {
    std::istringstream iss(input);
    std::string word;
    std::string result;

    while (iss >> word) {
        if (!result.empty()) {
            result += " ";
        }
        result += word;
    }

    return result;
}

std::vector<std::string> ParseTags(std::istream& input) {
    std::string str;
    std::getline(input, str);
    if (str.empty()) {
        return {};
    }
    std::vector<std::string> result;
    boost::split(result, str, boost::is_any_of(","));

    for (auto& tag : result) {
        tag = RemoveExtraSpaces(tag);
    }

    result.erase(std::remove_if(result.begin(), result.end(),
        [](const std::string& s) { return s.empty(); }),
        result.end());

    std::sort(result.begin(), result.end());
    auto last = std::unique(result.begin(), result.end());
    result.erase(last, result.end());
    return result;
}

View::View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output)
    : menu_{menu}
    , use_cases_{use_cases}
    , input_{input}
    , output_{output} {
    menu_.AddAction(  //
        "AddAuthor"s, "name"s, "Adds author"s, std::bind(&View::AddAuthor, this, ph::_1)
        // либо
        // [this](auto& cmd_input) { return AddAuthor(cmd_input); }
    );
    menu_.AddAction("AddBook"s, "<pub year> <title>"s, "Adds book"s,
                    std::bind(&View::AddBook, this, ph::_1));
    menu_.AddAction("ShowAuthors"s, {}, "Show authors"s, std::bind(&View::ShowAuthors, this));
    menu_.AddAction("ShowBooks"s, {}, "Show books"s, std::bind(&View::ShowBooks, this));
    menu_.AddAction("ShowAuthorBooks"s, {}, "Show author books"s,
                    std::bind(&View::ShowAuthorBooks, this));
    menu_.AddAction("DeleteAuthor"s, "<name>", "Delete author and all his books, type empty name to select from list"s,
        std::bind(&View::DeleteAuthor, this, ph::_1));
    menu_.AddAction("EditAuthor"s, "<name>", "Edit author name, type empty name to select from list"s,
        std::bind(&View::EditAuthor, this, ph::_1));
    menu_.AddAction("ShowBook"s, "<title>", "Show all book info, type empty title to select from list"s,
        std::bind(&View::ShowBook, this, ph::_1));
    menu_.AddAction("DeleteBook"s, "<title>", "Delete all book info, type empty title to select from list"s,
        std::bind(&View::DeleteBook, this, ph::_1));
    menu_.AddAction("EditBook"s, "<title>", "Edit all book info but author, type empty title to select from list"s,
        std::bind(&View::EditBook, this, ph::_1));
}

bool View::AddAuthor(std::istream& cmd_input) const {
    auto unit = use_cases_.GetUnit();
    try {
        std::string name;
        if (!getline(cmd_input, name) || name.empty()) {
            throw std::exception();
        }
        boost::algorithm::trim(name);
        unit->AddAuthor(std::move(name));
    } catch (const std::exception&) {
        unit->Abort();
        output_ << "Failed to add author"sv << std::endl;
        return true;
    }
    unit->Commit();
    return true;
}

bool View::AddBook(std::istream& cmd_input) const {
    auto unit = use_cases_.GetUnit();
    try {
        if (auto params = GetBookParams(unit.get(), cmd_input)) {
            auto book = unit->AddBook(domain::AuthorId::FromString(params->author_id), params->title, params->publication_year);
            AddTags(unit.get(), book.GetId());
        }
    } catch (const std::exception&) {
        unit->Abort();
        output_ << "Failed to add book"sv << std::endl;
        return true;
    }
    unit->Commit();
    return true;
}

bool View::ShowAuthors() const {
    auto unit = use_cases_.GetUnit();
    PrintVector(output_, GetAuthors(unit.get()));
    unit->Commit();
    return true;
}

bool View::ShowBooks() const {
    auto unit = use_cases_.GetUnit();
    PrintVector(output_, GetBooks(unit.get()));
    unit->Commit();
    return true;
}

bool View::ShowAuthorBooks() const {
    auto unit = use_cases_.GetUnit();
    try {
        if (auto author = SelectAuthorFromList(unit.get())) {
            PrintVector(output_, GetAuthorBooks(unit.get(), author->id));
        }

    } catch (const std::exception&) {
        unit->Abort();
        output_ << "Failed to Show Books"sv << std::endl;
        return true;
    }
    unit->Commit();
    return true;
}

bool View::DeleteAuthor(std::istream& cmd_input) const {
    auto unit = use_cases_.GetUnit();
    try {
        std::string name;
        std::getline(cmd_input, name);
        if (name.empty()) {
            if (auto selected_author = SelectAuthorFromList(unit.get())) {
                unit->DeleteAuthor({domain::AuthorId::FromString(selected_author->id), selected_author->name});
            }
        }
        else {
            boost::algorithm::trim(name);
            if (auto author = unit->GetAuthorIfExists(name)) {
                unit->DeleteAuthor(author.value());
            }
            else {
                throw std::exception();
            }
        }
    }
    catch (const std::exception&) {
        unit->Abort();
        output_ << "Failed to delete author"sv << std::endl;
        return true;
    }
    unit->Commit();
    return true;
}

bool View::EditAuthor(std::istream& cmd_input) const {
    auto unit = use_cases_.GetUnit();
    try {
        std::string name;
        if (!std::getline(cmd_input, name) || name.empty()) {
            if (auto selected_author = SelectAuthorFromList(unit.get())) {
                output_ << "Enter new name:" << std::endl;
                std::string new_name;
                if (std::getline(input_, new_name) && !new_name.empty()) {
                    boost::algorithm::trim(new_name);
                    unit->EditAuthor({ domain::AuthorId::FromString(selected_author->id), new_name });
                }
                else {
                    throw std::exception();
                }
            }
        }
        else {
            boost::algorithm::trim(name);
            if (auto author = unit->GetAuthorIfExists(name)) {
                output_ << "Enter new name:" << std::endl;
                std::string new_name;
                if (std::getline(input_, new_name) && !new_name.empty()) {
                    boost::algorithm::trim(new_name);
                    unit->EditAuthor({ author->GetId(), new_name });
                }
                else {
                    throw std::exception();
                }
            }
            else {
                throw std::exception();
            }
        }
    }
    catch (const std::exception&) {
        unit->Abort();
        output_ << "Failed to edit author"sv << std::endl;
        return true;
    }
    unit->Commit();
    return true;
}

bool View::ShowBook(std::istream& cmd_input) const {
    auto unit = use_cases_.GetUnit();
    try {
        auto book = SelectBookFromCommand(unit.get(), cmd_input);
        if (book) {
            auto tags = unit->GetTags(book->GetId());
            output_ << "Title: " << book->GetTitle() << std::endl;
            output_ << "Author: " << book->GetAuthorName() << std::endl;
            output_ << "Publication year: " << book->GetYear() << std::endl;
            if (!tags.empty()) {
                output_ << "Tags: " << tags << std::endl;
            }
        }
    }
    catch (std::exception&) {
        unit->Abort();
        return true;
    }
    unit->Commit();
    return true;
}

bool View::DeleteBook(std::istream& cmd_input) const {
    auto unit = use_cases_.GetUnit();
    try {
        auto book = SelectBookFromCommand(unit.get(), cmd_input);
        if (book) {
            unit->DeleteBook(book->GetId());
        }
    }
    catch (std::exception&) {
        output_ << "Failed to delete book" << std::endl;
        unit->Abort();
        return true;
    }
    unit->Commit();
    return true;
}

bool View::EditBook(std::istream& cmd_input) const {
    auto unit = use_cases_.GetUnit();
    try {
        auto book = SelectBookFromCommand(unit.get(), cmd_input);
        if (book) {
            output_ << "Enter new title or empty line to use the current one (" + book->GetTitle() + "):" << std::endl;
            std::string new_name;
            std::getline(input_, new_name);
            if (new_name.empty()) {
                new_name = book->GetTitle();
            }
            output_ << "Enter publication year or empty line to use the current one (" + std::to_string(book->GetYear()) + "):" << std::endl;
            std::string new_year_str;
            std::getline(input_, new_year_str);
            int new_year;
            if (new_year_str.empty()) {
                new_year = book->GetYear();
            }
            else {
                new_year = std::stoi(new_year_str);
            }
            auto tags = unit->GetTags(book->GetId());
            output_ << "Enter tags (current tags: " << tags << "):" << std::endl;
            std::vector<std::string> new_tags = ParseTags(input_);
            auto new_book = domain::Book(book->GetId(), book->GetAuthor(), new_name, new_year);
            unit->EditBook(new_book, new_tags);
        }
        else {
            throw std::exception();
        }
    }
    catch (std::exception&) {
        unit->Abort();
        output_ << "Book not found" << std::endl;
        return true;
    }
    unit->Commit();
    return true;
}

std::optional<detail::AddBookParams> View::GetBookParams(app::UnitOfWork* unit, std::istream& cmd_input) const {
    detail::AddBookParams params;

    cmd_input >> params.publication_year;
    std::getline(cmd_input, params.title);
    boost::algorithm::trim(params.title);

    auto author = SelectAuthor(unit);
    if (!author) {
        return std::nullopt;
    }
    params.author_id = author->id;
    return params;
}

std::optional<detail::AuthorInfo> View::SelectAuthor(app::UnitOfWork* unit) const {
    output_ << "Enter author name or empty line to select from list:"sv << std::endl;
    std::string str;
    std::getline(input_, str);
    if (str.empty()) {
        return SelectAuthorFromList(unit);
    }
    
    boost::algorithm::trim(str);
    if (auto author = unit->GetAuthorIfExists(str)) {
        return detail::AuthorInfo(author->GetId().ToString(), author->GetName());
    }
    output_ << "No author found. Do you want to add "sv << str << " (y/n)?"sv << std::endl;
    std::string yes_or_no;
    if (!std::getline(input_, yes_or_no) || yes_or_no.empty()) {
        throw std::exception();
    }
    if (yes_or_no == "y" || yes_or_no == "Y") {
        unit->AddAuthor(str);
        if (auto author = unit->GetAuthorIfExists(str)) {
            return detail::AuthorInfo(author->GetId().ToString(), author->GetName());
        }
    }
    throw std::exception();
}

std::optional<detail::AuthorInfo> View::SelectAuthorFromList(app::UnitOfWork* unit) const {
    output_ << "Select author:" << std::endl;
    auto authors = GetAuthors(unit);
    PrintVector(output_, authors);
    output_ << "Enter author # or empty line to cancel"sv << std::endl;

    std::string str;
    if (!std::getline(input_, str) || str.empty()) {
        return std::nullopt;
    }

    int author_idx;
    try {
        author_idx = std::stoi(str);
    }
    catch (std::exception const&) {
        throw std::runtime_error("Invalid author num");
    }

    --author_idx;
    if (author_idx < 0 or author_idx >= authors.size()) {
        throw std::runtime_error("Invalid author num");
    }

    return authors[author_idx];
}

std::vector<detail::AuthorInfo> View::GetAuthors(app::UnitOfWork* unit) const {
    std::vector<detail::AuthorInfo> dst_autors;
    for (auto& author : unit->GetAuthors()) {
        dst_autors.emplace_back(std::move(author.GetId().ToString()), author.GetName());
    }
    return dst_autors;
}

std::vector<domain::Book> View::GetBooks(app::UnitOfWork* unit) const {
    return unit->GetBooks();
}

std::vector<detail::BookInfo> View::GetAuthorBooks(app::UnitOfWork* unit, const std::string& author_id) const {
    std::vector<detail::BookInfo> books;
    for (auto& book : unit->GetBooksByAuthor(domain::AuthorId::FromString(author_id))) {
        books.emplace_back(book.GetTitle(), book.GetYear());
    }
    return books;
}

void View::AddTags(app::UnitOfWork* unit, const domain::BookId& book) const {
    output_ << "Enter tags (comma separated):" << std::endl;
    auto tags = ParseTags(input_);
    if (tags.empty()) {
        return;
    }
    unit->AddTags(book, tags);
}

std::optional<domain::Book> View::SelectBook(app::UnitOfWork* unit) const {
    auto books = GetBooks(unit);
    PrintVector(output_, books);
    output_ << "Enter the book # or empty line to cancel:"sv << std::endl;

    std::string str;
    std::getline(input_, str);
    boost::algorithm::trim(str);
    if (str.empty()) {
        return std::nullopt;
    }

    int book_idx;
    try {
        book_idx = std::stoi(str);
    }
    catch (std::exception const&) {
        throw std::runtime_error("Invalid author num");
    }

    --book_idx;
    if (book_idx < 0 or book_idx >= books.size()) {
        throw std::runtime_error("Invalid author num");
    }

    return books[book_idx];
}

std::optional<domain::Book> View::SelectBookFromCommand(app::UnitOfWork* unit, std::istream& cmd_input) const {
    std::string title;
    std::getline(cmd_input, title);
    boost::algorithm::trim(title);
    if (title.empty()) {
        return SelectBook(unit);
    }

    boost::algorithm::trim(title);
    auto books = unit->GetBooksByTitle(title);
    if (books.empty()) {
        return {};
    }
    if (books.size() == 1) {
        return books[0];
    }
    PrintVector(output_, books);
    output_ << "Enter the book # or empty line to cancel:" << std::endl;
    std::string str;
    std::getline(input_, str);
    boost::algorithm::trim(str);
    if (str.empty()) {
        return std::nullopt;
    }

    int book_idx;
    try {
        book_idx = std::stoi(str);
    }
    catch (std::exception const&) {
        throw std::runtime_error("Invalid book num");
    }

    --book_idx;
    if (book_idx < 0 or book_idx >= books.size()) {
        throw std::runtime_error("Invalid book num");
    }
    return books[book_idx];
}

}  // namespace ui