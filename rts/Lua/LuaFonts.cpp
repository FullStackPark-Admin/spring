/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaFonts.h"

#include "LuaInclude.h"

#include "LuaHandle.h"
#include "LuaHashString.h"
#include "LuaUtils.h"

#include "LuaOpenGL.h"

#include "Rendering/GL/myGL.h"
#include "Rendering/Fonts/glFont.h"
#include "System/Exceptions.h"


bool LuaFonts::PushEntries(lua_State* L)
{
	CreateMetatable(L);

	REGISTER_LUA_CFUNC(LoadFont);
	REGISTER_LUA_CFUNC(DeleteFont);

	return true;
}


bool LuaFonts::CreateMetatable(lua_State* L)
{
	luaL_newmetatable(L, "Font");

	HSTR_PUSH_CFUNC(L, "__gc",        meta_gc);
	HSTR_PUSH_CFUNC(L, "__index",     meta_index);
	LuaPushNamedString(L, "__metatable", "protected metatable");

		//! push userdata callouts
		REGISTER_LUA_CFUNC(Print);

		REGISTER_LUA_CFUNC(Begin);
		REGISTER_LUA_CFUNC(End);

		REGISTER_LUA_CFUNC(WrapText);

		REGISTER_LUA_CFUNC(GetTextWidth);
		REGISTER_LUA_CFUNC(GetTextHeight);

		REGISTER_LUA_CFUNC(SetTextColor);
		REGISTER_LUA_CFUNC(SetOutlineColor);

		REGISTER_LUA_CFUNC(SetAutoOutlineColor);

		REGISTER_LUA_CFUNC(BindTexture);

	lua_pop(L, 1);
	return true;
}


/******************************************************************************/
/******************************************************************************/

inline void CheckDrawingEnabled(lua_State* L, const char* caller)
{
	if (!LuaOpenGL::IsDrawingEnabled(L)) {
		luaL_error(L, "%s(): OpenGL calls can only be used in Draw() "
		              "call-ins, or while creating display lists", caller);
	}
}


inline CglFont* tofont(lua_State* L, int idx)
{
	CglFont** font = (CglFont**)luaL_checkudata(L, idx, "Font");

	if (*font == nullptr)
		luaL_error(L, "attempt to use a deleted font");

	return *font;
}


/******************************************************************************/
/******************************************************************************/

int LuaFonts::meta_gc(lua_State* L)
{
	if (lua_isnil(L, 1)) {
		return 0;
	}

	CglFont** font = (CglFont**)luaL_checkudata(L, 1, "Font");
	delete *font;
	*font = nullptr;
	return 0;
}


int LuaFonts::meta_index(lua_State* L)
{
	//! first check if there is a function
	luaL_getmetatable(L, "Font");
	lua_pushvalue(L, 2);
	lua_rawget(L, -2);
	if (!lua_isnil(L, -1))
		return 1;

	lua_pop(L, 1);

	//! couldn't find a function, so check properties
	CglFont* f = tofont(L, 1);

	if (lua_israwstring(L, 2)) {
		const char* key = lua_tostring(L, 2);

		switch (hashString(key)) {
			case hashString("size"): {
				lua_pushnumber(L, f->GetSize());
				return 1;
			} break;
			case hashString("path"): {
				lua_pushsstring(L, f->GetFilePath());
				return 1;
			} break;

			case hashString("height"):
			case hashString("lineheight"): {
				lua_pushnumber(L, f->GetLineHeight());
				return 1;
			} break;

			case hashString("descender"): {
				lua_pushnumber(L, f->GetDescender());
				return 1;
			} break;

			case hashString("outlinewidth"): {
				lua_pushnumber(L, f->GetOutlineWidth());
				return 1;
			} break;
			case hashString("outlineweight"): {
				lua_pushnumber(L, f->GetOutlineWeight());
				return 1;
			} break;

			case hashString("family"): {
				lua_pushsstring(L, f->GetFamily());
				return 1;
			} break;
			case hashString("style"): {
				lua_pushsstring(L, f->GetStyle());
				return 1;
			} break;

			case hashString("texturewidth"): {
				lua_pushnumber(L, f->GetTextureWidth());
				return 1;
			} break;
			case hashString("textureheight"): {
				lua_pushnumber(L, f->GetTextureHeight());
				return 1;
			} break;

			default: {
			} break;
		}
	}

	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaFonts::LoadFont(lua_State* L)
{
	CglFont* f = CglFont::LoadFont(luaL_checkstring(L, 1), luaL_optint(L, 2, 14), luaL_optint(L, 3, 2), luaL_optfloat(L, 4, 15.0f));
	if (f == nullptr)
		return 0;

	CglFont** font = (CglFont**) lua_newuserdata(L, sizeof(CglFont*));
	*font = f;

	luaL_getmetatable(L, "Font");
	lua_setmetatable(L, -2);
	return 1;
}


int LuaFonts::DeleteFont(lua_State* L)
{
	return meta_gc(L);
}


/******************************************************************************/
/******************************************************************************/

int LuaFonts::Print(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	const int args = lua_gettop(L); // number of arguments

	CglFont* f = tofont(L, 1);

	const string text(luaL_checkstring(L, 2),lua_strlen(L, 2));
	const float x     = luaL_checkfloat(L, 3);
	const float y     = luaL_checkfloat(L, 4);
	const float size  = luaL_optfloat(L, 5, f->GetSize());

	int options = FONT_NEAREST;

	if ((args >= 6) && lua_isstring(L, 6)) {
		const char* c = lua_tostring(L, 6);
		while (*c != 0) {
	  		switch (*c) {
				case 'c': { options |= FONT_CENTER;        break; }
				case 'r': { options |= FONT_RIGHT;         break; }

				case 'a': { options |= FONT_ASCENDER;      break; }
				case 't': { options |= FONT_TOP;           break; }
				case 'v': { options |= FONT_VCENTER;       break; }
				case 'x': { options |= FONT_BASELINE;      break; }
				case 'b': { options |= FONT_BOTTOM;        break; }
				case 'd': { options |= FONT_DESCENDER;     break; }

				case 's': { options |= FONT_SHADOW;        break; }
				case 'o':
				case 'O': { options |= FONT_OUTLINE;       break; }

				case 'n': { options ^= FONT_NEAREST;       break; }
				default: break;
			}
	  		c++;
		}
	}

	f->glPrint(x, y, size, options, text);

	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaFonts::Begin(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	CglFont* f = tofont(L, 1);
	f->Begin();
	return 0;
}


int LuaFonts::End(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	CglFont* f = tofont(L, 1);
	f->End();
	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaFonts::WrapText(lua_State* L)
{
	CglFont* f = tofont(L, 1);
	string text(luaL_checkstring(L, 2),lua_strlen(L, 2));
	const float maxWidth   = luaL_checkfloat(L, 3);
	const float maxHeight  = luaL_optfloat(L, 4, 1e9);
	const float size       = luaL_optfloat(L, 5, f->GetSize());

	const int lines = f->WrapInPlace(text,size,maxWidth,maxHeight);

	lua_pushsstring(L, text);
	lua_pushnumber(L, lines);
	return 2;
}

/******************************************************************************/
/******************************************************************************/

int LuaFonts::GetTextWidth(lua_State* L)
{
	CglFont* f = tofont(L, 1);

	lua_pushnumber(L, f->GetTextWidth(std::string(luaL_checkstring(L, 2), lua_strlen(L, 2))));
	return 1;
}


int LuaFonts::GetTextHeight(lua_State* L)
{
	CglFont* f = tofont(L, 1);
	const string text(luaL_checkstring(L, 2),lua_strlen(L, 2));
	float descender;
	int lines;
	const float height = f->GetTextHeight(text,&descender,&lines);
	lua_pushnumber(L, height);
	lua_pushnumber(L, descender);
	lua_pushnumber(L, lines);
	return 3;
}


/******************************************************************************/
/******************************************************************************/

int LuaFonts::SetTextColor(lua_State* L)
{
	CglFont* f = tofont(L, 1);

	const int args = lua_gettop(L); // number of arguments

	if (args < 2)
		luaL_error(L, "Incorrect arguments to font:SetTextColor([\"textColor\"])");

	float4 color;

	if (lua_istable(L, 2)) {
		const int count = LuaUtils::ParseFloatArray(L, 2, &color.x, 4);

		if (count < 3)
			luaL_error(L, "Incorrect arguments to font:SetTextColor([\"textColor\"])");

		if (count == 3)
			color.w = 1.0f;

	} else if (args >= 4) {
		color.x = luaL_checkfloat(L, 2);
		color.y = luaL_checkfloat(L, 3);
		color.z = luaL_checkfloat(L, 4);
		color.w = luaL_optfloat(L, 5, 1.0f);
	} else if (!lua_isnil(L, 2)) {
		luaL_error(L, "Incorrect arguments to font:SetTextColor([\"textColor\"])");
	}

	f->SetTextColor(&color);
	return 0;
}


int LuaFonts::SetOutlineColor(lua_State* L)
{
	CglFont* f = tofont(L, 1);

	const int args = lua_gettop(L); // number of arguments

	if (args < 2)
		luaL_error(L, "Incorrect arguments to font:SetOutlineColor([\"outlineColor\"])");

	float4 color;

	if (lua_istable(L, 2)) {
		const int count = LuaUtils::ParseFloatArray(L, 2, &color.x, 4);

		if (count < 3)
			luaL_error(L, "Incorrect arguments to font:SetOutlineColor([\"outlineColor\"])");

		if (count == 3)
			color.w = 1.0f;

	} else if (args >= 4) {
		color.x = luaL_checkfloat(L, 2);
		color.y = luaL_checkfloat(L, 3);
		color.z = luaL_checkfloat(L, 4);
		color.w = luaL_optfloat(L, 5, 1.0f);
	} else if (!lua_isnoneornil(L, 2)) {
		luaL_error(L, "Incorrect arguments to font:SetOutlineColor([\"outlineColor\"])");
	}

	f->SetOutlineColor(&color);
	return 0;
}


int LuaFonts::SetAutoOutlineColor(lua_State* L)
{
	CglFont* f = tofont(L, 1);
	f->SetAutoOutlineColor(luaL_checkboolean(L, 2));
	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaFonts::BindTexture(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);

	CglFont* f = tofont(L, 1);

	glBindTexture(GL_TEXTURE_2D, f->GetTexture());
	glEnable(GL_TEXTURE_2D);

	return 0;
}

/******************************************************************************/
/******************************************************************************/
