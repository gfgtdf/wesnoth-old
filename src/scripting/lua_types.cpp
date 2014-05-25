/*
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#include "lua_types.hpp"

#include "lua/lualib.h"
#include "lua/lauxlib.h"

#include "scripting/lua_api.hpp"
#include "variable.hpp"

#include <cassert>
#include <cstring>
#include <boost/foreach.hpp>
/* Dummy pointer for getting unique keys for Lua's registry. */
static char const v_dlgclbkKey = 0;
static char const v_executeKey = 0;
static char const v_getsideKey = 0;
static char const v_gettextKey = 0;
static char const v_gettypeKey = 0;
static char const v_getraceKey = 0;
static char const v_getunitKey = 0;
static char const v_tstringKey = 0;
static char const v_unitvarKey = 0;
static char const v_ustatusKey = 0;
static char const v_vconfigKey = 0;


luatypekey const dlgclbkKey = static_cast<void *>(const_cast<char *>(&v_dlgclbkKey));
luatypekey const executeKey = static_cast<void *>(const_cast<char *>(&v_executeKey));
luatypekey const getsideKey = static_cast<void *>(const_cast<char *>(&v_getsideKey));
luatypekey const gettextKey = static_cast<void *>(const_cast<char *>(&v_gettextKey));
luatypekey const gettypeKey = static_cast<void *>(const_cast<char *>(&v_gettypeKey));
luatypekey const getraceKey = static_cast<void *>(const_cast<char *>(&v_getraceKey));
luatypekey const getunitKey = static_cast<void *>(const_cast<char *>(&v_getunitKey));
luatypekey const tstringKey = static_cast<void *>(const_cast<char *>(&v_tstringKey));
luatypekey const unitvarKey = static_cast<void *>(const_cast<char *>(&v_unitvarKey));
luatypekey const ustatusKey = static_cast<void *>(const_cast<char *>(&v_ustatusKey));
luatypekey const vconfigKey = static_cast<void *>(const_cast<char *>(&v_vconfigKey));



#define return_tstring_attrib(name, accessor) \
	if (strcmp(m, name) == 0) { \
		luaW_pushtstring(L, accessor); \
		return 1; \
	}

#define return_cstring_attrib(name, accessor) \
	if (strcmp(m, name) == 0) { \
		lua_pushstring(L, accessor); \
		return 1; \
	}

#define return_string_attrib(name, accessor) \
	return_cstring_attrib(name, accessor.c_str())

#define return_int_attrib(name, accessor) \
	if (strcmp(m, name) == 0) { \
		lua_pushinteger(L, accessor); \
		return 1; \
	}

#define return_float_attrib(name, accessor) \
	if (strcmp(m, name) == 0) { \
		lua_pushnumber(L, accessor); \
		return 1; \
	}

#define return_bool_attrib(name, accessor) \
	if (strcmp(m, name) == 0) { \
		lua_pushboolean(L, accessor); \
		return 1; \
	}

#define return_cfg_attrib(name, accessor) \
	if (strcmp(m, name) == 0) { \
		config cfg; \
		accessor; \
		luaW_pushconfig(L, cfg); \
		return 1; \
	}

#define return_cfgref_attrib(name, accessor) \
	if (strcmp(m, name) == 0) { \
		luaW_pushconfig(L, accessor); \
		return 1; \
	}

#define return_vector_string_attrib(name, accessor) \
	if (strcmp(m, name) == 0) { \
		const std::vector<std::string>& vector = accessor; \
		lua_createtable(L, vector.size(), 0); \
		int i = 1; \
		BOOST_FOREACH(const std::string& s, vector) { \
			lua_pushstring(L, s.c_str()); \
			lua_rawseti(L, -2, i); \
			++i; \
		} \
		return 1; \
	}

#define modify_tstring_attrib(name, accessor) \
	if (strcmp(m, name) == 0) { \
		t_string value = luaW_checktstring(L, 3); \
		accessor; \
		return 0; \
	}

#define modify_string_attrib(name, accessor) \
	if (strcmp(m, name) == 0) { \
		const char *value = luaL_checkstring(L, 3); \
		accessor; \
		return 0; \
	}

#define modify_int_attrib(name, accessor) \
	if (strcmp(m, name) == 0) { \
		int value = luaL_checkinteger(L, 3); \
		accessor; \
		return 0; \
	}

#define modify_int_attrib_check_range(name, accessor, allowed_min, allowed_max) \
	if (strcmp(m, name) == 0) { \
		int value = luaL_checkinteger(L, 3); \
		if (value < allowed_min || allowed_max < value) return luaL_argerror(L, 3, "out of bounds"); \
		accessor; \
		return 0; \
	}

#define modify_bool_attrib(name, accessor) \
	if (strcmp(m, name) == 0) { \
		bool value = lua_toboolean(L, 3); \
		accessor; \
		return 0; \
	}

#define modify_vector_string_attrib(name, accessor) \
	if (strcmp(m, name) == 0) { \
		std::vector<std::string> vector; \
		char const* message = "table with unnamed indices holding strings expected"; \
		if (!lua_istable(L, 3)) return luaL_argerror(L, 3, message); \
		unsigned length = lua_rawlen(L, 3); \
		for (unsigned i = 1; i <= length; ++i) { \
			lua_rawgeti(L, 3, i); \
			char const* string = lua_tostring(L, 4); \
			if(!string) return luaL_argerror(L, 2 + i, message); \
			vector.push_back(string); \
			lua_pop(L, 1); \
		} \
		accessor; \
		return 0; \
	}



/**
 * Converts a Lua value at position @a src and appends it to @a dst.
 * @note This function is private to lua_tstring_concat. It expects two things.
 *       First, the t_string metatable is at the top of the stack on entry. (It
 *       is still there on exit.) Second, the caller hasn't any valuable object
 *       with dynamic lifetime, since they would be leaked on error.
 */
static void tstring_concat_aux(lua_State *L, t_string &dst, int src)
{
	switch (lua_type(L, src)) {
		case LUA_TNUMBER:
		case LUA_TSTRING:
			dst += lua_tostring(L, src);
			break;
		case LUA_TUSERDATA:
			// Compare its metatable with t_string's metatable.
			if (!lua_getmetatable(L, src) || !lua_rawequal(L, -1, -2))
				luaL_typerror(L, src, "string");
			dst += *static_cast<t_string *>(lua_touserdata(L, src));
			lua_pop(L, 1);
			break;
		default:
			luaL_typerror(L, src, "string");
	}
}

/**
 * Appends a scalar to a t_string object (__concat metamethod).
 */
static int impl_tstring_concat(lua_State *L)
{
	// Create a new t_string.
	t_string *t = new(lua_newuserdata(L, sizeof(t_string))) t_string;

	lua_pushlightuserdata(L
			, tstringKey);

	lua_rawget(L, LUA_REGISTRYINDEX);

	// Append both arguments to t.
	tstring_concat_aux(L, *t, 1);
	tstring_concat_aux(L, *t, 2);

	lua_setmetatable(L, -2);
	return 1;
}

/**
 * Destroys a t_string object before it is collected (__gc metamethod).
 */
static int impl_tstring_collect(lua_State *L)
{
	t_string *t = static_cast<t_string *>(lua_touserdata(L, 1));
	t->t_string::~t_string();
	return 0;
}

/**
 * Converts a t_string object to a string (__tostring metamethod);
 * that is, performs a translation.
 */
static int impl_tstring_tostring(lua_State *L)
{
	t_string *t = static_cast<t_string *>(lua_touserdata(L, 1));
	lua_pushstring(L, t->c_str());
	return 1;
}

void register_lua_tstring(lua_State *L)
{
	lua_pushlightuserdata(L, tstringKey);
	lua_createtable(L, 0, 4);
	lua_pushcfunction(L, impl_tstring_concat);
	lua_setfield(L, -2, "__concat");
	lua_pushcfunction(L, impl_tstring_collect);
	lua_setfield(L, -2, "__gc");
	lua_pushcfunction(L, impl_tstring_tostring);
	lua_setfield(L, -2, "__tostring");
	lua_pushstring(L, "translatable string");
	lua_setfield(L, -2, "__metatable");
	lua_rawset(L, LUA_REGISTRYINDEX);
}


/**
 * Gets the parsed field of a vconfig object (_index metamethod).
 * Special fields __literal, __shallow_literal, __parsed, and
 * __shallow_parsed, return Lua tables.
 */
static int impl_vconfig_get(lua_State *L)
{
	vconfig *v = static_cast<vconfig *>(lua_touserdata(L, 1));

	if (lua_isnumber(L, 2))
	{
		vconfig::all_children_iterator i = v->ordered_begin();
		unsigned len = std::distance(i, v->ordered_end());
		unsigned pos = lua_tointeger(L, 2) - 1;
		if (pos >= len) return 0;
		std::advance(i, pos);
		lua_createtable(L, 2, 0);
		lua_pushstring(L, i.get_key().c_str());
		lua_rawseti(L, -2, 1);
		new(lua_newuserdata(L, sizeof(vconfig))) vconfig(i.get_child());
		lua_pushlightuserdata(L
				, vconfigKey);

		lua_rawget(L, LUA_REGISTRYINDEX);
		lua_setmetatable(L, -2);
		lua_rawseti(L, -2, 2);
		return 1;
	}

	char const *m = luaL_checkstring(L, 2);
	if (strcmp(m, "__literal") == 0) {
		luaW_pushconfig(L, v->get_config());
		return 1;
	}
	if (strcmp(m, "__parsed") == 0) {
		luaW_pushconfig(L, v->get_parsed_config());
		return 1;
	}

	bool shallow_literal = strcmp(m, "__shallow_literal") == 0;
	if (shallow_literal || strcmp(m, "__shallow_parsed") == 0)
	{
		lua_newtable(L);
		BOOST_FOREACH(const config::attribute &a, v->get_config().attribute_range()) {
			if (shallow_literal)
				luaW_pushscalar(L, a.second);
			else
				luaW_pushscalar(L, v->expand(a.first));
			lua_setfield(L, -2, a.first.c_str());
		}
		vconfig::all_children_iterator i = v->ordered_begin(),
			i_end = v->ordered_end();
		if (shallow_literal) {
			i.disable_insertion();
			i_end.disable_insertion();
		}
		for (int j = 1; i != i_end; ++i, ++j)
		{
			lua_createtable(L, 2, 0);
			lua_pushstring(L, i.get_key().c_str());
			lua_rawseti(L, -2, 1);
			luaW_pushvconfig(L, i.get_child());
			lua_rawseti(L, -2, 2);
			lua_rawseti(L, -2, j);
		}
		return 1;
	}

	if (v->null() || !v->has_attribute(m)) return 0;
	luaW_pushscalar(L, (*v)[m]);
	return 1;
}

/**
 * Returns the number of a child of a vconfig object.
 */
static int impl_vconfig_size(lua_State *L)
{
	vconfig *v = static_cast<vconfig *>(lua_touserdata(L, 1));
	lua_pushinteger(L, v->null() ? 0 :
		std::distance(v->ordered_begin(), v->ordered_end()));
	return 1;
}

/**
 * Destroys a vconfig object before it is collected (__gc metamethod).
 */
static int impl_vconfig_collect(lua_State *L)
{
	vconfig *v = static_cast<vconfig *>(lua_touserdata(L, 1));
	v->vconfig::~vconfig();
	return 0;
}

void register_lua_vconfig(lua_State *L)
{
	lua_pushlightuserdata(L
			, vconfigKey);
	lua_createtable(L, 0, 4);
	lua_pushcfunction(L, impl_vconfig_collect);
	lua_setfield(L, -2, "__gc");
	lua_pushcfunction(L, impl_vconfig_get);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, impl_vconfig_size);
	lua_setfield(L, -2, "__len");
	lua_pushstring(L, "wml object");
	lua_setfield(L, -2, "__metatable");
	lua_rawset(L, LUA_REGISTRYINDEX);
}

/**
 * Creates a t_string object (__call metamethod).
 * - Arg 1: userdata containing the domain.
 * - Arg 2: string to translate.
 * - Ret 1: string containing the translatable string.
 */
static int impl_gettext(lua_State *L)
{
	char const *m = luaL_checkstring(L, 2);
	char const *d = static_cast<char *>(lua_touserdata(L, 1));
	// Hidden metamethod, so d has to be a string. Use it to create a t_string.
	luaW_pushtstring(L, t_string(m, d));
	return 1;
}

void register_lua_gettext(lua_State *L)
{	
	lua_pushlightuserdata(L
			, gettextKey);
	lua_createtable(L, 0, 2);
	lua_pushcfunction(L, impl_gettext);
	lua_setfield(L, -2, "__call");
	lua_pushstring(L, "message domain");
	lua_setfield(L, -2, "__metatable");
	lua_rawset(L, LUA_REGISTRYINDEX);
}

