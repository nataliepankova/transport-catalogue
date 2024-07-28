#pragma once

#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace json {

    class Node;
    // ????????? ?????????? Dict ? Array ??? ?????????
    using Dict = std::map<std::string, Node>;
    using Array = std::vector<Node>;

    // ??? ?????? ?????? ????????????? ??? ??????? ???????? JSON
    class ParsingError : public std::runtime_error {
    public:
        using runtime_error::runtime_error;
    };


    class Node {
    public:
        /* ?????????? Node, ????????? std::variant */

        using Value = std::variant<nullptr_t, int, double, std::string, bool, Array, Dict>;

        Node() = default;
        Node(std::nullptr_t value);
        Node(Array array);
        Node(Dict map);
        Node(int value);
        Node(double value);
        Node(bool value);
        Node(std::string value);

        const Value& GetValue() const { return value_; }

        bool IsInt() const;
        bool IsDouble() const;
        bool IsPureDouble() const;
        bool IsBool() const;
        bool IsString() const;
        bool IsNull() const;
        bool IsArray() const;
        bool IsMap() const;


        const Array& AsArray() const;
        const Dict& AsMap() const;
        int AsInt() const;
        const std::string& AsString() const;
        bool AsBool() const;
        double AsDouble() const;

    private:
        Value value_;
    };

    bool operator==(const Node& left, const Node& right);

    bool operator!=(const Node& left, const Node& right);

    class Document {
    public:
        explicit Document(Node root);

        const Node& GetRoot() const;

    private:
        Node root_;
    };

    Document Load(std::istream& input);

    void Print(const Document& doc, std::ostream& output);

    bool operator==(const Document& lhs, const Document& rhs);
    bool operator!=(const Document& lhs, const Document& rhs);

}  // namespace json