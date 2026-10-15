#include "svg.h"
#include <sstream>
#include <iomanip>
#include <string_view>

using namespace std;

namespace svg {

Circle& Circle::SetCenter(Point center) {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius) {
    radius_ = radius;
    return *this;
}

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();
    RenderObject(context);
    context.out << "\n";
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\"" << center_.x << "\" cy=\"" << center_.y << "\" r=\"" << radius_ << "\"";
    RenderAttrs(out);
    out << "/>";
}

void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<polyline points=\"";
    bool first = true;
    for (const auto& p : points_) {
        if (!first) out << " ";
        out << p.x << "," << p.y;
        first = false;
    }
    out << "\"";
    RenderAttrs(out);
    out << "/>";
}

void Text::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<text";
    RenderAttrs(out);
    out << " x=\"" << pos_.x << "\" y=\"" << pos_.y << "\" dx=\"" << offset_.x
        << "\" dy=\"" << offset_.y << "\" font-size=\"" << size_ << "\"";

    if (!font_family_.empty()) {
        out << " font-family=\"" << font_family_ << "\"";
    }
    if (!font_weight_.empty()) {
        out << " font-weight=\"" << font_weight_ << "\"";
    }

    out << ">" << data_ << "</text>";
}

ostream& operator<<(ostream& out, const Rgb& rgb) {
    out << "rgb(" << static_cast<int>(rgb.red) << ","
        << static_cast<int>(rgb.green) << "," << static_cast<int>(rgb.blue) << ")";
    return out;
}

ostream& operator<<(ostream& out, const Rgba& rgba) {
    out << "rgba(" << static_cast<int>(rgba.red) << ","
        << static_cast<int>(rgba.green) << "," << static_cast<int>(rgba.blue)
        << "," << rgba.opacity << ")";
    return out;
}

ostream& operator<<(ostream& out, const Color& color) {
    visit([&out](auto&& value) {
        using T = decay_t<decltype(value)>;
        if constexpr (is_same_v<T, monostate>) {
            out << "none";
        } else {
            out << value;
        }
    }, color);
    return out;
}

} // namespace svg