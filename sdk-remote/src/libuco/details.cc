/*
 * Copyright (C) 2011, Gostai S.A.S.
 *
 * This software is provided "as is" without warranty of any kind,
 * either expressed or implied, including but not limited to the
 * implied warranties of fitness for a particular purpose.
 *
 * See the LICENSE file for more information.
 */

#include <libport/debug.hh>

#include <urbi/details.hh>

GD_CATEGORY(Urbi.Details);

namespace urbi
{
  namespace uobjects
  {

    StringPair
    uname_split(const std::string& name)
    {
      size_t p = name.find_last_of(".");
      if (p == name.npos)
      {
        GD_FWARN("invalid argument to split_name: %s", name);
        return StringPair(name, "");
      }
      std::string oname = name.substr(0, p);
      std::string slot = name.substr(p + 1, name.npos);
      return StringPair(oname, slot);
    }

  }
}