/*
 * Copyright (C) 2010-2012, Gostai S.A.S.
 *
 * This software is provided "as is" without warranty of any kind,
 * either expressed or implied, including but not limited to the
 * implied warranties of fitness for a particular purpose.
 *
 * See the LICENSE file for more information.
 */

#include <libport/lexical-cast.hh>

#include <urbi/object/symbols.hh>

#include <urbi/object/cxx-conversions.hh>
#include <urbi/object/file.hh>
#include <urbi/object/global.hh>
#include <urbi/object/path.hh>
#include <urbi/object/position.hh>
#include <urbi/object/string.hh>

namespace urbi
{
  namespace object
  {

    /*---------------.
    | Construction.  |
    `---------------*/

    Position::Position()
      : pos_()
    {
      proto_add(proto ? rObject(proto) : Object::proto);
    }

    Position::Position(const value_type& pos)
      : pos_(pos)
    {
      proto_add(proto ? rObject(proto) : Object::proto);
    }

    Position::Position(libport::Symbol* f, unsigned int l, unsigned int c)
      : pos_(f, l, c)
    {
      proto_add(proto ? rObject(proto) : Object::proto);
    }

    Position::Position(rPosition model)
      : pos_(model->pos_)
    {
      CAPTURE_GLOBAL(Orderable);
      proto_add(Orderable);
      proto_add(proto);
    }

    URBI_CXX_OBJECT_INIT(Position)
      : pos_()
    {
      BIND(init, init, void, ());
      BIND(init, init, void, (const value_type& pos));
      BIND(init, init, void, (unsigned from, unsigned to));
      BIND(init, init, void, (rObject file, unsigned from, unsigned to));
      typedef bool (Position::*comparison_type)(rPosition rhs) const;
      BIND(EQ_EQ, operator==, comparison_type);
      BIND(LT, operator<, comparison_type);

      BIND(MINUS, operator -);
      BIND(PLUS, operator +);
      BIND(lines);
      BIND(columns);
      BIND(asString, as_string);

#define DECLARE(Name)                                     \
      bind(#Name, &Position::Name ##_get, &Position::Name ##_set);

      DECLARE(file);

#undef DECLARE

      // For some reason, cl.exe refuses "&Position::value_type::line"
      // with error: function cannot access 'yy::position::line'
#define DECLARE(Name)                           \
      bind(SYMBOL_(Name), &yy::position::Name)
      DECLARE(line);
      DECLARE(column);
#undef DECLARE
    }

    void
    Position::init()
    {}

    void
    Position::init(const value_type& pos)
    {
      pos_ = pos;
    }

    void
    Position::init(unsigned from, unsigned to)
    {
      pos_ = value_type(0, from, to);
    }

    void
    Position::init(rObject file, unsigned from, unsigned to)
    {
      libport::Symbol* f = 0;
      if (file != nil_class)
        // FIXME: leak
        f = new libport::Symbol(from_urbi<std::string>(file));
      pos_ = value_type(f, from, to);
    }

    /*-------------.
    | Comparison.  |
    `-------------*/

    bool
    operator ==(const Position::value_type& lhs,
                const Position::value_type& rhs)
    {
      return lhs.line == rhs.line && lhs.column == rhs.column;
    }

    bool
    operator <(const Position::value_type& lhs,
               const Position::value_type& rhs)
    {
      return lhs.line < rhs.line
        || ( lhs.line == rhs.line && lhs.column < rhs.column);
    }

    /*-------------.
    | Operations.  |
    `-------------*/

    rPosition
    Position::operator - (int rhs) const
    {
      return this->operator+(-rhs);
    }

    rPosition
    Position::operator + (int rhs) const
    {
      return (new Position(pos_))->columns(rhs);
    }

    rPosition
    Position::lines(int count)
    {
      pos_.lines(count);
      return this;
    }

    rPosition
    Position::columns(int count)
    {
      // bison position does not handle cases when the count operand is lower
      // than (- column).
      int column = pos_.column;
      count = std::max(count, -column);

      pos_.columns(count);
      return this;
    }

    /*--------------.
    | Conversions.  |
    `--------------*/

    std::string
    Position::as_string() const
    {
      return string_cast(pos_);
    }

    /*-----------.
    | Accessor.  |
    `-----------*/

    boost::optional<libport::path>
    Position::file_get() const
    {
      if (pos_.filename)
        return libport::path(pos_.filename->name_get());
      return 0;
    }

    void
    Position::file_set(rObject o)
    {
      delete pos_.filename;
      pos_.filename = 0;
      if (o != nil_class)
        pos_.filename = new libport::Symbol(from_urbi<std::string>(o));
    }
  } // namespace object
} // namespace urbi
