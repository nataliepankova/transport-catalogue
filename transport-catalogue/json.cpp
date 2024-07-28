#include "json.h"

using namespace std;

namespace json {

    namespace {

        Node LoadNode(istream& input);

        std::string LoadLiteral(std::istream& input) {
            std::string str;

            while (std::isalpha(input.peek())) {
                str.push_back(static_cast<char>(input.get()));
            }
            return str;
        }

        Node LoadBool(std::istream& input) {
            const auto s = LoadLiteral(input);
            if (s == "true"sv) {
                return Node{ true };
            }
            else if (s == "false"sv) {
                return Node{ false };
            }
            else {
                throw ParsingError("Failed to parse '"s + s + "' as bool"s);
            }
        }

        Node LoadNull(std::istream& input) {
            auto literal = LoadLiteral(input);
            if (literal == "null"sv) {
                return Node{ nullptr };
            }
            else {
                throw ParsingError("Failed to parse '"s + literal + "' as null"s);
            }
        }

        using Number = std::variant<int, double>;

        Node LoadNumber(std::istream& input) {
            using namespace std::literals;

            std::string parsed_num;

            // ????????? ? parsed_num ????????? ?????? ?? input
            auto read_char = [&parsed_num, &input] {
                parsed_num += static_cast<char>(input.get());
                if (!input) {
                    throw ParsingError("Failed to read number from stream"s);
                }
                };

            // ????????? ???? ??? ????? ???? ? parsed_num ?? input
            auto read_digits = [&input, read_char] {
                if (!std::isdigit(input.peek())) {
                    throw ParsingError("A digit is expected"s);
                }
                while (std::isdigit(input.peek())) {
                    read_char();
                }
                };

            if (input.peek() == '-') {
                read_char();
            }
            // ?????? ????? ????? ?????
            if (input.peek() == '0') {
                read_char();
                // ????? 0 ? JSON ?? ????? ???? ?????? ?????
            }
            else {
                read_digits();
            }

            bool is_int = true;
            // ?????? ??????? ????? ?????
            if (input.peek() == '.') {
                read_char();
                read_digits();
                is_int = false;
            }

            // ?????? ???????????????? ????? ?????
            if (int ch = input.peek(); ch == 'e' || ch == 'E') {
                read_char();
                if (ch = input.peek(); ch == '+' || ch == '-') {
                    read_char();
                }
                read_digits();
                is_int = false;
            }

            try {
                if (is_int) {
                    // ??????? ??????? ????????????? ?????? ? int
                    try {
                        return Node(std::stoi(parsed_num));
                    }
                    catch (...) {
                        // ? ?????? ???????, ????????, ??? ????????????,
                        // ??? ???? ????????? ????????????? ?????? ? double
                    }
                }
                return Node(std::stod(parsed_num));
            }
            catch (...) {
                throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
            }
        }

        // ????????? ?????????? ?????????? ???????? JSON-?????????
        // ??????? ??????? ???????????? ????? ?????????? ???????????? ??????? ":
        Node LoadString(std::istream& input) {
            auto it = std::istreambuf_iterator<char>(input);
            auto end = std::istreambuf_iterator<char>();
            std::string s;
            while (true) {
                if (it == end) {
                    throw ParsingError("String parsing error");
                }
                const char ch = *it;
                if (ch == '"') {
                    ++it;
                    break;
                }
                else if (ch == '\\') {
                    ++it;
                    if (it == end) {
                        throw ParsingError("String parsing error");
                    }
                    const char escaped_char = *(it);
                    switch (escaped_char) {
                    case 'n':
                        s.push_back('\n');
                        break;
                    case 't':
                        s.push_back('\t');
                        break;
                    case 'r':
                        s.push_back('\r');
                        break;
                    case '"':
                        s.push_back('"');
                        break;
                    case '\\':
                        s.push_back('\\');
                        break;
                    default:
                        throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
                    }
                }
                else if (ch == '\n' || ch == '\r') {
                    throw ParsingError("Unexpected end of line"s);
                }
                else {
                    s.push_back(ch);
                }
                ++it;
            }

            return Node(std::move(s));
        }


        Node LoadArray(istream& input) {
            Array result;

            for (char c; input >> c && c != ']';) {
                if (c != ',') {
                    input.putback(c);
                }
                result.push_back(LoadNode(input));
            }
            if (!input) {
                throw ParsingError("Array parsing error"s);
            }

            return Node(move(result));
        }

        Node LoadDict(std::istream& input) {
            Dict result;

            for (char ch; input >> ch && ch != '}';) {
                if (ch == '"') {
                    std::string key = LoadString(input).AsString();
                    if (input >> ch && ch == ':') {
                        if (result.find(key) != result.end()) {
                            throw ParsingError("duplicate key '"s + key + "'found");
                        }
                        result.emplace(std::move(key), LoadNode(input));
                    }
                    else {
                        throw ParsingError(": expected. but '"s + ch + "' found"s);
                    }
                }
                else if (ch != ',') {
                    throw ParsingError("',' expected. but '"s + ch + "' found"s);
                }
            }

            if (!input) {
                throw ParsingError("Failed to read Dict from stream"s);
            }
            else {
                return Node(result);
            }

        }

        Node LoadNode(istream& input) {
            char c;
            input >> c;

            if (c == '[') {
                return LoadArray(input);
            }
            else if (c == '{') {
                return LoadDict(input);
            }
            else if (c == '"') {
                return LoadString(input);
            }
            else if (c == 't' || c == 'f') {
                input.putback(c);
                return LoadBool(input);
            }
            else if (c == 'n') {
                input.putback(c);
                return LoadNull(input);
            }
            else {
                input.putback(c);
                return Node(LoadNumber(input));
            }

        }

    }  // namespace

    // ============ Constructors ============
    Node::Node(std::nullptr_t value)
        : value_(value) {
    }

    Node::Node(Array array)
        : value_(move(array)) {
    }

    Node::Node(Dict map)
        : value_(move(map)) {
    }

    Node::Node(int value)
        : value_(value) {
    }

    Node::Node(double value)
        : value_(value) {
    }

    Node::Node(bool value)
        : value_(value) {
    }

    Node::Node(string value)
        : value_(move(value)) {
    }

    // ========== bool methods ===========

    bool Node::IsInt() const {
        return holds_alternative<int>(value_);
    }
    bool Node::IsDouble() const {
        return IsInt() || IsPureDouble();
    }
    bool Node::IsPureDouble() const {
        return holds_alternative<double>(value_);
    }
    bool Node::IsBool() const {
        return holds_alternative<bool>(value_);
    }
    bool Node::IsString() const {
        return holds_alternative<string>(value_);
    }
    bool Node::IsNull() const {
        return holds_alternative<nullptr_t>(value_);
    }
    bool Node::IsArray() const {
        return holds_alternative<Array>(value_);
    }
    bool Node::IsMap() const {
        return holds_alternative<Dict>(value_);
    }

    // ========== get value methods ============

    const Array& Node::AsArray() const {
        if (IsArray()) {
            return get<Array>(value_);
        }
        throw std::logic_error("invalid type");
    }
    const Dict& Node::AsMap() const {
        if (IsMap()) {
            return get<Dict>(value_);
        }
        throw std::logic_error("invalid type");
    }
    int Node::AsInt() const {
        if (IsInt()) {
            return get<int>(value_);
        }
        throw std::logic_error("invalid type");
    }
    const std::string& Node::AsString() const {
        if (IsString()) {
            return get<string>(value_);
        }
        throw std::logic_error("invalid type");
    }
    bool Node::AsBool() const {
        if (IsBool()) {
            return get<bool>(value_);
        }
        throw std::logic_error("invalid type");
    }
    double Node::AsDouble() const {
        if (IsDouble()) {
            return IsPureDouble() ? get<double>(value_) : static_cast<double>(get<int>(value_));
        }
        throw std::logic_error("invalid type");
    }

    // ========== document methods =========

    Document::Document(Node root)
        : root_(move(root)) {
    }

    const Node& Document::GetRoot() const {
        return root_;
    }

    Document Load(istream& input) {
        return Document{ LoadNode(input) };
    }

    // ========= printing json ==============

    // ???????? ??????, ?????? ?????? ?? ????? ?????? ? ??????? ?????
    struct PrintContext {
        std::ostream& out;
        int indent_step = 4;
        int indent = 0;

        void PrintIndent() const {
            for (int i = 0; i < indent; ++i) {
                out.put(' ');
            }
        }

        // ?????????? ????? ???????? ?????? ? ??????????? ?????????
        [[nodiscard]] PrintContext Indented() const {
            return { out, indent_step, indent_step + indent };
        }
    };

    void PrintString(const std::string& value, std::ostream& out) {
        out.put('"');
        for (const char c : value) {
            switch (c) {
            case '\r':
                out << "\\r"sv;
                break;
            case '\n':
                out << "\\n"sv;
                break;
            case '\t':
                out << "\\t"sv;
                break;
            case '"':
                // ??????? " ? \ ????????? ??? \" ??? \\, ??????????????
                [[fallthrough]];
            case '\\':
                out.put('\\');
                [[fallthrough]];
            default:
                out.put(c);
                break;
            }
        }
        out.put('"');
    }

    void PrintNode(const Node& node, const PrintContext& context);

    struct PrintValue {
        // ??? ?????? double ? int
        const PrintContext& ctx;

        void operator()(const int value) const {
            ctx.out << value;
        }
        void operator()(const double value) const {
            ctx.out << value;
        }
        void operator()(const string& value) const {
            PrintString(value, ctx.out);
        }
        void operator()(const nullptr_t) const {
            ctx.out << "null"sv;
        }
        void operator()(const bool value) const {
            ctx.out << boolalpha << value;
        }
        void operator()(const Array& arr) const {
            std::ostream& out = ctx.out;
            out << "[\n"sv;
            bool first = true;
            auto inner_context = ctx.Indented();

            for (const Node& node : arr) {
                if (first) {
                    first = false;
                }
                else {
                    out << ",\n"sv;
                }

                inner_context.PrintIndent();
                PrintNode(node, inner_context);
            }

            out.put('\n');
            ctx.PrintIndent();
            out.put(']');
        }
        void operator()(const Dict& dict) const {
            std::ostream& out = ctx.out;
            out << "{\n"sv;
            bool first = true;
            auto inner_context = ctx.Indented();

            for (const auto& [key, node] : dict) {
                if (first) {
                    first = false;
                }
                else {
                    out << ",\n"sv;
                }

                inner_context.PrintIndent();
                PrintString(key, ctx.out);
                out << ": "sv;
                PrintNode(node, inner_context);
            }

            out.put('\n');
            ctx.PrintIndent();
            out.put('}');
        }
    };

    void PrintNode(const Node& node, const PrintContext& ctx) {
        visit(PrintValue{ ctx }, node.GetValue());
    }

    void Print(const Document& doc, std::ostream& output) {

        PrintNode(doc.GetRoot(), PrintContext{ output });

        // ?????????? ??????? ??????????????
    }
    //========== comparison operators ==============
    bool operator==(const Node& left, const Node& right) {
        return left.GetValue() == right.GetValue();
    }

    bool operator!=(const Node& left, const Node& right) {
        return !(left == right);
    }

    bool operator==(const Document& lhs, const Document& rhs) {
        return lhs.GetRoot() == rhs.GetRoot();
    }
    bool operator!=(const Document& lhs, const Document& rhs) {
        return !(lhs == rhs);
    }

}  // namespace json