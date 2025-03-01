#ifndef LUASCRIPTHELPER_H_
#define LUASCRIPTHELPER_H_

#include "simulation/Particle.h"
#include "simulation/ElementDefs.h"
#include "common/String.h"
#include "LuaCompat.h"

class GameModel;
class GameController;
class Simulation;
class LuaScriptInterface;
class Graphics;
class Renderer;

extern GameModel * luacon_model;
extern GameController * luacon_controller;
extern Simulation * luacon_sim;
extern LuaScriptInterface * luacon_ci;
extern Graphics * luacon_g;
extern Renderer * luacon_ren;

extern bool *luacon_currentCommand;
extern String *luacon_lastError;

class LuaSmartRef;
extern int *lua_el_mode;
extern LuaSmartRef *lua_el_func, *lua_gr_func;

extern int getPartIndex_curIdx;
extern int tptProperties; //Table for some TPT properties
extern int tptPropertiesVersion;
extern int tptElements; //Table for TPT element names
extern int tptParts, tptPartsMeta, tptElementTransitions, tptPartsCData, tptPartMeta, cIndex;
extern LuaSmartRef *tptPart;

void luacon_hook(lua_State *L, lua_Debug *ar);
int luacon_eval(const char *command);
String luacon_geterror();
void luacon_close();
void initLegacyProps();
int luacon_partsread(lua_State* l);
int luacon_partswrite(lua_State* l);
int luacon_partread(lua_State* l);
int luacon_partwrite(lua_State* l);
int luacon_elementread(lua_State* l);
int luacon_elementwrite(lua_State* l);
int luacon_transitionread(lua_State* l);
int luacon_transitionwrite(lua_State* l);
//int process_command_lua(pixel *vid_buf, char *console, char *console_error);

//tpt. api
int luatpt_getelement(lua_State *l);

int luacon_graphicsReplacement(GRAPHICS_FUNC_ARGS, int i);
int luatpt_graphics_func(lua_State *l);

int luacon_elementReplacement(UPDATE_FUNC_ARGS);
int luatpt_element_func(lua_State *l);

int luatpt_error(lua_State* l);
int luatpt_drawtext(lua_State* l);

int luatpt_create(lua_State* l);

int luatpt_setpause(lua_State* l);

int luatpt_togglepause(lua_State* l);

int luatpt_togglewater(lua_State* l);

int luatpt_setconsole(lua_State* l);
int luatpt_log(lua_State* l);

int luatpt_set_pressure(lua_State* l);

int luatpt_set_gravity(lua_State* l);
int luatpt_reset_gravity_field(lua_State* l);

int luatpt_reset_velocity(lua_State* l);

int luatpt_reset_spark(lua_State* l);

int luatpt_set_property(lua_State* l);

int luatpt_get_property(lua_State* l);

int luatpt_set_wallmap(lua_State* l);

int luatpt_get_wallmap(lua_State* l);

int luatpt_set_elecmap(lua_State* l);

int luatpt_get_elecmap(lua_State* l);

int luatpt_drawpixel(lua_State* l);

int luatpt_drawrect(lua_State* l);

int luatpt_fillrect(lua_State* l);

int luatpt_drawline(lua_State* l);

int luatpt_textwidth(lua_State* l);
int luatpt_get_name(lua_State* l);

int luatpt_delete(lua_State* l);
int luatpt_input(lua_State* l);
int luatpt_message_box(lua_State* l);
int luatpt_confirm(lua_State* l);
int luatpt_get_numOfParts(lua_State* l);
int luatpt_start_getPartIndex(lua_State* l);
int luatpt_next_getPartIndex(lua_State* l);
int luatpt_getPartIndex(lua_State* l);
int luatpt_hud(lua_State* l);
int luatpt_gravity(lua_State* l);
int luatpt_airheat(lua_State* l);
int luatpt_active_menu(lua_State* l);
int luatpt_menu_enabled(lua_State* l);
int luatpt_num_menus(lua_State* l);
int luatpt_decorations_enable(lua_State* l);

int luatpt_heat(lua_State* l);
int luatpt_cmode_set(lua_State* l);
int luatpt_setfire(lua_State* l);
int luatpt_setdebug(lua_State* l);
int luatpt_autoreload_enable(lua_State* l);

int luatpt_setfpscap(lua_State* l);
int luatpt_setdrawcap(lua_State* l);

int luatpt_getscript(lua_State* l);

int luatpt_setwindowsize(lua_State* l);

int luatpt_screenshot(lua_State* l);
int luatpt_record(lua_State* l);
int luatpt_record_subframe(lua_State* l);
int luatpt_setrecordinterval(lua_State* l);

int luatpt_perfectCircle(lua_State* l);

int luatpt_set_bray_life_brightness_threshold(lua_State* l);

#endif /* LUASCRIPTHELPER_H_ */
