﻿#include "svg.h"
#include <regex>

namespace svg {

    using namespace std::literals;

    std::ostream& operator<<(std::ostream& out, StrokeLineCap line_cap) {
        using namespace std::literals;
        switch (line_cap) {
        case StrokeLineCap::BUTT:
            out << "butt"sv;
            break;
        case StrokeLineCap::ROUND:
            out << "round"sv;
            break;
        case StrokeLineCap::SQUARE:
            out << "square"sv;
            break;
        default:
            break;
        }
        return out;
    }

    std::ostream& operator<<(std::ostream& out, StrokeLineJoin line_join) {
        using namespace std::literals;
        switch (line_join) {
        case StrokeLineJoin::ARCS:
            out << "arcs"sv;
            break;
        case StrokeLineJoin::BEVEL:
            out << "bevel"sv;
            break;
        case StrokeLineJoin::MITER:
            out << "miter"sv;
            break;
        case StrokeLineJoin::MITER_CLIP:
            out << "miter-clip"sv;
            break;
        case StrokeLineJoin::ROUND:
            out << "round"sv;
            break;
        default:
            break;
        }
        return out;
    }

    void Object::Render(const RenderContext& context) const {
        context.RenderIndent();

        // Делегируем вывод тега своим подклассам
        RenderObject(context);

        context.out << std::endl;
    }

    // ---------- Circle ------------------

    Circle& Circle::SetCenter(Point center) {
        center_ = center;
        return *this;
    }

    Circle& Circle::SetRadius(double radius) {
        radius_ = radius;
        return *this;
    }

    void Circle::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
        out << "r=\""sv << radius_ << "\" "sv;
        RenderAttrs(context.out);
        out << "/>"sv;
    }

    // ---------- Polyline ------------------
    Polyline& Polyline::AddPoint(Point point) {
        points_.emplace_back(point);
        return *this;
    }

    void Polyline::RenderObject(const RenderContext& context) const {

        auto& out = context.out;
        out << "<polyline points=\""sv;
        if (!points_.empty()) {
            bool is_first = true;
            std::string delim;
            for (const auto& point : points_) {
                is_first ? delim = "" : delim = " ";
                out << delim << point.x << ","sv << point.y;
                is_first = false;
            }

        }
        out << "\""sv;
        RenderAttrs(context.out);

        out << " />"sv;

    }

    // ---------- Text ------------------
    Text& Text::SetPosition(Point pos) {
        position_ = pos;
        return *this;
    }

    Text& Text::SetOffset(Point offset) {
        offset_ = offset;
        return *this;
    }

    Text& Text::SetFontSize(uint32_t size) {
        font_size_ = size;
        return *this;
    }

    Text& Text::SetFontFamily(std::string font_family) {
        font_family_ = font_family;
        return *this;
    }

    Text& Text::SetFontWeight(std::string font_weight) {
        font_weight_ = font_weight;
        return *this;
    }

    Text& Text::SetData(std::string data) {
        data_ = data;
        return *this;
    }

    std::string Text::PrepareTextData(std::string data) const {
        std::vector<std::string> replace_values = { "&quot;"s , "&apos;"s , "&lt;"s , "&gt;"s , "&amp;"s };
        for (size_t i = 0; i < data.size(); ) {
            std::string curr_replace_value;
            switch (data[i])
            {
            case '\"':
                curr_replace_value = replace_values[0];
                break;
            case '\'':
                curr_replace_value = replace_values[1];
                break;
            case '<':
                curr_replace_value = replace_values[2];
                break;
            case '>':
                curr_replace_value = replace_values[3];
                break;
            case '&':
                curr_replace_value = replace_values[4];
                break;
            default:
                break;
            }
            if (!curr_replace_value.empty()) {
                data.insert(i, curr_replace_value);
                i += replace_values[0].size() - 1;
            }
            else {
                ++i;
            }

        }
        return data;
    }

    void Text::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<text"sv;

        RenderAttrs(context.out);
            
        out << " x = \""sv << position_.x << "\" y=\""sv << position_.y << "\" "sv;
        out << "dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\" "sv;

        out << "font-size=\""sv << font_size_ << "\" "sv;
        if (!font_family_.empty()) {
            out << "font-family=\""sv << font_family_ << "\" "sv;
        }

        if (!font_weight_.empty()) {
            out << "font-weight=\""sv << font_weight_ << "\" "sv;
        }
        

        out << ">"sv << PrepareTextData(data_) << "</text>"sv;
    }

    // ---------- Document ------------------
    void Document::AddPtr(std::unique_ptr<Object>&& obj) {
        objects_.emplace_back(std::move(obj));
    }

    void Document::Render(std::ostream& out) const {
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
        out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;
        for (const auto& object : objects_) {
            object->Render(RenderContext(out));
        }
        out << "</svg>"sv;
    }


}  // namespace svg