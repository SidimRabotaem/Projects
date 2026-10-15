#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <variant>

namespace svg {

enum class StrokeLineCap {
    BUTT,
    ROUND,
    SQUARE,
};

enum class StrokeLineJoin {
    ARCS,
    BEVEL,
    MITER,
    MITER_CLIP,
    ROUND,
};

std::ostream& operator<<(std::ostream& out, StrokeLineCap line_cap);
std::ostream& operator<<(std::ostream& out, StrokeLineJoin line_join);

struct Point {
    Point() = default;
    Point(double x, double y)
        : x(x)
        , y(y) {
    }
    double x = 0;
    double y = 0;
};

struct RenderContext {
    RenderContext(std::ostream& out)
        : out(out) {
    }

    RenderContext(std::ostream& out, int indent_step, int indent = 0)
        : out(out)
        , indent_step(indent_step)
        , indent(indent) {
    }

    RenderContext Indented() const {
        return {out, indent_step, indent + indent_step};
    }

    void RenderIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    std::ostream& out;
    int indent_step = 0;
    int indent = 0;
};

struct Rgb {
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
};

struct Rgba {
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    double opacity = 1.0;
};

using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;

inline const Color NoneColor{"none"};

std::ostream& operator<<(std::ostream& out, const Rgb& rgb);
std::ostream& operator<<(std::ostream& out, const Rgba& rgba);
std::ostream& operator<<(std::ostream& out, const Color& color);


template <typename Owner>
class PathProps {
public:
    Owner& SetFillColor(Color color) {
        fill_color_ = std::move(color);
        return AsOwner();
    }

    Owner& SetStrokeColor(Color color) {
        stroke_color_ = std::move(color);
        return AsOwner();
    }

    Owner& SetStrokeWidth(double width) {
        stroke_width_ = width;
        return AsOwner();
    }

    Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
        stroke_line_cap_ = line_cap;
        return AsOwner();
    }

    Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
        stroke_line_join_ = line_join;
        return AsOwner();
    }

protected:
    ~PathProps() = default;
    
    void RenderAttrs(std::ostream& out) const {
        if (fill_color_) {
            out << " fill=\"" << *fill_color_ << "\"";
        }
        if (stroke_color_) {
            out << " stroke=\"" << *stroke_color_ << "\"";
        }
        if (stroke_width_) {
            out << " stroke-width=\"" << *stroke_width_ << "\"";
        }
        if (stroke_line_cap_) {
            out << " stroke-linecap=\"" << *stroke_line_cap_ << "\"";
        }
        if (stroke_line_join_) {
            out << " stroke-linejoin=\"" << *stroke_line_join_ << "\"";
        }
    }

private:
    Owner& AsOwner() {
        return static_cast<Owner&>(*this);
    }

    std::optional<Color> fill_color_;
    std::optional<Color> stroke_color_;
    std::optional<double> stroke_width_;
    std::optional<StrokeLineCap> stroke_line_cap_;
    std::optional<StrokeLineJoin> stroke_line_join_;
};

class Object {
public:
    void Render(const RenderContext& context) const;

    virtual ~Object() = default;

private:
    virtual void RenderObject(const RenderContext& context) const = 0;
};

class Circle final : public Object, public PathProps<Circle> {
public:
    Circle& SetCenter(Point center);
    Circle& SetRadius(double radius);

private:
    void RenderObject(const RenderContext& context) const override;

    Point center_;
    double radius_ = 1.0;
};

class Polyline : public Object, public PathProps<Polyline> {
public:

    Polyline& AddPoint(Point point) {
        points_.push_back(point);
        return *this;
    }
    

    private:
    void RenderObject(const RenderContext& context) const override;
    
    std::vector<Point> points_;
};

class Text : public Object, public PathProps<Text> {
public:

    Text& SetPosition(Point pos) {
        pos_ = pos;
        return *this;
    }

    Text& SetOffset(Point offset) {
        offset_ = offset;
        return *this;
    }

    Text& SetFontSize(uint32_t size) {
        size_ = size;
        return *this;
    }

    Text& SetFontFamily(std::string font_family) {
        font_family_ = std::move(font_family);
        return *this;
    }

    Text& SetFontWeight(std::string font_weight) {
        font_weight_ = std::move(font_weight);
        return *this;
    }

    Text& SetData(std::string data) {
        data_ = std::move(data);
        return *this;
    }

private:
    void RenderObject(const RenderContext& context) const override;

    Point pos_;
    Point offset_;
    uint32_t size_ = 1; 
    std::string font_family_;  
    std::string font_weight_;
    std::string data_;
};

class ObjectContainer {
    public:
    virtual ~ObjectContainer() = default;

    template <typename Obj>
    void Add(Obj obj) {
    AddPtr(std::make_unique<Obj>(std::move(obj)));
}

    virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;
};

class Drawable {
    public:
    
    virtual ~Drawable() = default;

    virtual void Draw(ObjectContainer& container) const = 0;
};

class Document : public ObjectContainer {
public:
   
    Document() = default;

    void AddPtr(std::unique_ptr<Object>&& obj) override {
        objects_.push_back(std::move(obj));
    }
    
    void Render(std::ostream& out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n";
    RenderContext ctx(out, 2, 2);
    for (const auto& obj : objects_) {
        obj->Render(ctx);
    }
    out << "</svg>";
}

    private:
    std::vector<std::unique_ptr<Object>> objects_;
};

   

   inline std::ostream& operator<<(std::ostream& out, StrokeLineCap line_cap) {
    switch (line_cap) {
        case StrokeLineCap::BUTT: return out << "butt";
        case StrokeLineCap::ROUND: return out << "round";
        case StrokeLineCap::SQUARE: return out << "square";
    }
    return out;
}

inline std::ostream& operator<<(std::ostream& out, StrokeLineJoin line_join) {
    switch (line_join) {
        case StrokeLineJoin::ARCS: return out << "arcs";
        case StrokeLineJoin::BEVEL: return out << "bevel";
        case StrokeLineJoin::MITER: return out << "miter";
        case StrokeLineJoin::MITER_CLIP: return out << "miter-clip";
        case StrokeLineJoin::ROUND: return out << "round";
    }
    return out;
}





}  // namespace svg