/*
 * Copyright (C) 2007-2012, Gostai S.A.S.
 *
 * This software is provided "as is" without warranty of any kind,
 * either expressed or implied, including but not limited to the
 * implied warranties of fitness for a particular purpose.
 *
 * See the LICENSE file for more information.
 */

/**
 ** \file object/code.cc
 ** \brief Creation of the Urbi object code.
 */

#include <libport/lexical-cast.hh>

#include <ast/parametric-ast.hh>
#include <ast/print.hh>

#include <urbi/kernel/userver.hh>

#include <object/code.hh>
#include <urbi/object/job.hh>
#include <urbi/object/list.hh>
#include <urbi/object/string.hh>
#include <urbi/object/object.hh>
#include <urbi/object/symbols.hh>

#include <urbi/runner/raise.hh>
#include <runner/job.hh>
#include <eval/call.hh>

DECLARE_LOCATION_FILE;

namespace urbi
{
  namespace object
  {
    using ::kernel::runner;

    Code::Code(ast_type a,
               rObject call, rLobby lobby, rObject ths,
               captures_type captures)
      : ast_(a)
      , call_(call)
      , captures_(captures)
      , lobby_(lobby)
      , this_(ths)
    {
      proto_add(proto);
      proto_remove(Object::proto);
    }

    Code::Code(rCode model)
      : ast_(model->ast_)
      , call_(model->call_)
      , captures_(model->captures_)
      , lobby_(model->lobby_)
      , this_(model->this_)
    {
      proto_add(model);
      proto_remove(Object::proto);
    }

    URBI_CXX_OBJECT_INIT(Code)
      : ast_(0)
      , call_(0)
      , captures_(0)
      , lobby_(0)
      , this_(0)
    {
      PARAMETRIC_AST(ast, "function () {}");
      ast_ = ast.result<const ast::Routine>();
      proto_add(Executable::proto);
      proto_remove(Object::proto);
      bind_variadic<rObject, Code>(SYMBOL(apply), &Code::apply);
      BIND(EQ_EQ, operator==, bool, (const rObject&) const);
      BIND(asString,   as_string);
      BIND(bodyString, body_string);
      BIND(spawn);
      BIND(getImports, imports_get);
    }


    Code::ast_type Code::ast_get() const
    {
      return ast_;
    }

    rObject Code::call_get() const
    {
      return call_;
    }

    const Code::captures_type& Code::captures_get() const
    {
      return captures_;
    }

    rLobby Code::lobby_get() const
    {
      return lobby_;
    }

    void Code::lobby_set(const rLobby& l)
    {
      lobby_ = l;
    }

    rObject Code::this_get() const
    {
      return this_;
    }

    Code::ast_type& Code::ast_get()
    {
      return ast_;
    }

    rObject& Code::call_get()
    {
      return call_;
    }

    Code::captures_type& Code::captures_get()
    {
      return captures_;
    }

    void Code::this_set(rObject v)
    {
      this_ = v;
    }

    rObject Code::apply(const objects_type& apply_args)
    {
      check_arg_count(apply_args.size(), 1, 2);
      rList args = from_urbi<rList>(apply_args[0], 0u);

      runner::Job& r = runner();
      libport::Symbol s = (1 < apply_args.size())
        ? libport::Symbol(from_urbi<rString>(apply_args[1], 1u)->value_get())
        : SYMBOL(apply);
      if (args->value_get().empty())
        RAISE("argument list must begin with `this'");
      List::value_type a = args->value_get();

      return eval::call_apply(r, this, s, a);
    }

    std::string
    Code::as_string() const
    {
      return string_cast(*ast_);
    }

    std::string
    Code::body_string() const
    {
      if (proto == this)
        return SYMBOL(LT_Code_GT);
      return
        // See https://svn.boost.org/trac/boost/ticket/6264, we cannot
        // use string_cast here since Boost 1.48.
        libport::format("%s", *ast_->body_get()->body_get());
    }

    std::ostream&
    Code::special_slots_dump(std::ostream& o) const
    {
#define DISP(Attr)                                                      \
      << #Attr " = " << libport::deref << Attr ## _get() << libport::iendl
      return o
        DISP(ast)
        DISP(call)
        DISP(this)
        DISP(lobby)
        DISP(captures);
#undef DISP
    }

    rJob
    Code::spawn(bool clear_tags)
    {
      runner::Job& r = runner();
      runner::Job* new_runner = r.spawn_child(eval::call(this));
      new_runner->name_set(libport::fresh_string(r.name_get()));

      if (clear_tags)
        new_runner->state.tag_stack_clear();

      new_runner->time_shift_set(r.time_shift_get());
      new_runner->start_job();
      if (r.profile_get())
        new_runner->profile_start(r.profile_get(), SYMBOL(detach), this);
      return new_runner->as_job();
    }

    bool
    Code::operator==(const Code& that) const
    {
#define EQ(Attr)                                \
      (Attr ## _get() == that.Attr ## _get())
      return (EQ(captures)
              && EQ(this)
              && EQ(call)
              && EQ(lobby)
              && as_string() == that.as_string());
#undef EQ
    }

    bool
    Code::operator==(const rObject& that) const
    {
      if (Code* c = that->as<Code>())
        return *this == *c;
      return false;
    }
    rObject
    Code::operator() (object::objects_type args)
    {
      runner::Job& r = runner();
      aver(!args.empty());
      return eval::call_apply(r,
                              this,
                              libport::Symbol::make_empty(),
                              args);
    }

  } // namespace object
}
