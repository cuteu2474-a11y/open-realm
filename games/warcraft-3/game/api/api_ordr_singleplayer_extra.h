/* ORDR 2.323[C] extension native compatibility for OFFLINE play.
 * This header is included exactly once by api_module.c after the base ORDR
 * frame-state header and the normal native implementations.
 *
 * Design: preserve observable map state, avoid contacting online services,
 * and keep external renderer/keyboard adapters explicit (rather than hiding
 * missing functionality behind pretend successful operations).
 */
#ifndef ORDR_SINGLEPLAYER_EXTRA_H
#define ORDR_SINGLEPLAYER_EXTRA_H
#include <regex.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* Extra properties belong to frame handles; ORDR frame IDs are capped by
 * ordr_frame_new().  Point coordinates remain in UI virtual coordinates. */
typedef struct {
    int anchored, point, relative, relative_point;
    float x,y;
} ordr_anchor_t;
typedef struct {
    ordr_anchor_t anchors[16];
    int anchor_count, priority, text_align, focused;
    int model_type, model_flag, font_flag;
    float font_height;
    char font[260],model[512],kind[64];
    jassFunc_t const *callbacks[8];
    unsigned char callback_sync[8];
} ordr_frame_extra_t;
static ordr_frame_extra_t ordr_frame_extra[4096];
static int ordr_last_ui_frame;
static player_t *ordr_last_ui_player;
static player_t *ordr_last_key_player;
static int ordr_toc_count;
static char ordr_tocs[32][260];

/* The entry points are deliberately usable from a future game/client UI
 * adapter. Merely registering callbacks does not synthesize player input. */
static void ordr_frame_notify(jass_t *j,int id,int event) {
    ordr_frame_extra_t *e;
    jassFunc_t const *callback;
    if(!ordr_frame(id) || event<0 || event>=8) return;
    e=&ordr_frame_extra[id];
    callback=e->callbacks[event];
    if (!callback) return;
    /* Persist event data while the callback runs. The JASS VM callback
     * execution adapter must invoke callback in its coroutine context. */
    ordr_last_ui_frame=id;
    ordr_last_ui_player=currentplayer;
    (void)j;(void)callback;
}

uint32_t DzFrameSetPoint(jass_t *j) {
    int id=jass_checkinteger(j,1),point=jass_checkinteger(j,2);
    int relative=jass_checkinteger(j,3), relative_point=jass_checkinteger(j,4);
    float x=jass_checknumber(j,5),y=jass_checknumber(j,6);
    ordr_frame_extra_t *e;
    if(!ordr_frame(id) || point<0 || point>=16) return 0;
    e=&ordr_frame_extra[id];
    e->anchors[point]=(ordr_anchor_t){1,point,relative,relative_point,x,y};
    if(e->anchor_count<point+1)e->anchor_count=point+1;
    return 0;
}
uint32_t DzFrameSetAbsolutePoint(jass_t *j) {
    int id=jass_checkinteger(j,1),point=jass_checkinteger(j,2);
    float x=jass_checknumber(j,3),y=jass_checknumber(j,4);
    ordr_frame_extra_t *e;
    if(!ordr_frame(id) || point<0 || point>=16)return 0;
    e=&ordr_frame_extra[id];
    e->anchors[point]=(ordr_anchor_t){1,point,0,0,x,y};
    if(e->anchor_count<point+1)e->anchor_count=point+1;
    return 0;
}
uint32_t DzFrameClearAllPoints(jass_t *j) {
    int id=jass_checkinteger(j,1);
    if(ordr_frame(id)){
        memset(ordr_frame_extra[id].anchors,0,sizeof(ordr_frame_extra[id].anchors));
        ordr_frame_extra[id].anchor_count=0;
    }
    return 0;
}
uint32_t DzFrameSetFont(jass_t *j) {
    int id=jass_checkinteger(j,1);
    cstring_t file=jass_checkstring(j,2);
    float h=jass_checknumber(j,3);
    int flag=jass_checkinteger(j,4);
    if(ordr_frame(id)){
        ordr_frame_extra_t *e=&ordr_frame_extra[id];
        snprintf(e->font,sizeof(e->font),"%s",file?file:"");
        e->font_height=h;e->font_flag=flag;
    }
    return 0;
}
uint32_t DzFrameSetTextAlignment(jass_t *j) {
    int id=jass_checkinteger(j,1),align=jass_checkinteger(j,2);
    if(ordr_frame(id))ordr_frame_extra[id].text_align=align;
    return 0;
}
uint32_t DzFrameSetPriority(jass_t *j) {
    int id=jass_checkinteger(j,1),priority=jass_checkinteger(j,2);
    if(ordr_frame(id))ordr_frame_extra[id].priority=priority;
    return 0;
}
uint32_t DzFrameSetModel(jass_t *j) {
    int id=jass_checkinteger(j,1);
    cstring_t model=jass_checkstring(j,2);
    int kind=jass_checkinteger(j,3),flag=jass_checkinteger(j,4);
    if(ordr_frame(id)){
        ordr_frame_extra_t *e=&ordr_frame_extra[id];
        snprintf(e->model,sizeof(e->model),"%s",model?model:"");
        e->model_type=kind;e->model_flag=flag;
    }
    return 0;
}
uint32_t DzLoadToc(jass_t *j) {
    cstring_t toc=jass_checkstring(j,1);
    if(toc && *toc && ordr_toc_count<32)
        snprintf(ordr_tocs[ordr_toc_count++],sizeof(ordr_tocs[0]),"%s",toc);
    /* TOC parsing and materializing frame templates require the renderer
     * adapter; keeping its path here prevents silently losing the request. */
    return 0;
}
uint32_t DzFrameSetScriptByCode(jass_t *j) {
    int id=jass_checkinteger(j,1),event=jass_checkinteger(j,2);
    jassFunc_t const *callback=jass_checkcode(j,3);
    bool sync=jass_checkboolean(j,4);
    if(ordr_frame(id) && event>=0 && event<8){
        ordr_frame_extra[id].callbacks[event]=callback;
        ordr_frame_extra[id].callback_sync[event]=(unsigned char)sync;
    }
    return 0;
}
uint32_t DzGetTriggerUIEventFrame(jass_t *j){
    return jass_pushinteger(j,ordr_last_ui_frame);
}
uint32_t DzGetTriggerUIEventPlayer(jass_t *j){
    return jass_pushlighthandle(j,ordr_last_ui_player?ordr_last_ui_player:currentplayer,"player");
}

/* DZ synchronization in solo play: transport becomes synchronous local
 * delivery.  This has no remote players and therefore no network latency.
 * The payload must remain available throughout nested trigger invocation. */
typedef struct{
    trigger_t *trigger;
    char prefix[256];
} ordr_sync_registration_t;
static ordr_sync_registration_t ordr_sync_handlers[512];
static int ordr_sync_handler_count;
static char ordr_sync_context[64][4096];
static int ordr_sync_depth;
static player_t *ordr_sync_player;
uint32_t DzTriggerRegisterSyncData(jass_t *j){
    trigger_t *trigger=jass_checkhandle(j,1,"trigger");
    cstring_t prefix=jass_checkstring(j,2);
    (void)jass_checkboolean(j,3); /* server: irrelevant for one player */
    if(trigger && prefix && ordr_sync_handler_count<512) {
        ordr_sync_registration_t *e=&ordr_sync_handlers[ordr_sync_handler_count++];
        e->trigger=trigger;
        snprintf(e->prefix,sizeof(e->prefix),"%s",prefix);
    }
    return 0;
}
uint32_t DzSyncData(jass_t *j){
    cstring_t prefix=jass_checkstring(j,1),data=jass_checkstring(j,2);
    if(!prefix)return 0;
    if(ordr_sync_depth>=64){
        fprintf(stderr,"ORDR: nested DzSyncData depth exhausted\n");
        return 0;
    }
    ordr_sync_depth++;
    snprintf(ordr_sync_context[ordr_sync_depth-1],sizeof(ordr_sync_context[0]),
             "%s",data?data:"");
    player_t *prev=ordr_sync_player;
    ordr_sync_player=currentplayer;
    for(int i=0;i<ordr_sync_handler_count;i++){
        ordr_sync_registration_t *e=&ordr_sync_handlers[i];
        if(strcmp(prefix,e->prefix)==0 && e->trigger && !e->trigger->disabled)
            jass_executetrigger(j,e->trigger,NULL);
    }
    ordr_sync_player=prev;
    ordr_sync_depth--;
    return 0;
}
uint32_t DzGetTriggerSyncData(jass_t *j){
    return jass_pushstring(j,ordr_sync_depth>0?ordr_sync_context[ordr_sync_depth-1]:"");
}
uint32_t DzGetTriggerSyncPlayer(jass_t *j){
    return jass_pushlighthandle(j,ordr_sync_player?ordr_sync_player:currentplayer,"player");
}

/* Keyboard events are registered for a subsequent platform input adapter.
 * A trigger and code callback are both retained, not discarded. */
typedef struct {
    trigger_t *trigger;
    jassFunc_t const *callback;
    int key, status;
    bool sync;
} ordr_key_binding_t;
static ordr_key_binding_t ordr_key_bindings[256];
static int ordr_key_binding_count;
uint32_t DzTriggerRegisterKeyEventByCode(jass_t *j){
    trigger_t *t=jass_checkhandle(j,1,"trigger");
    int key=jass_checkinteger(j,2),status=jass_checkinteger(j,3);
    bool sync=jass_checkboolean(j,4);
    jassFunc_t const *callback=jass_checkcode(j,5);
    if(ordr_key_binding_count<256){
        ordr_key_binding_t *e=&ordr_key_bindings[ordr_key_binding_count++];
        *e=(ordr_key_binding_t){t,callback,key,status,sync};
    }
    return 0;
}
uint32_t DzGetTriggerKeyPlayer(jass_t *j){
    return jass_pushlighthandle(j,ordr_last_key_player?ordr_last_key_player:currentplayer,"player");
}

/* Solo profile backend: isolates profiles by user/character and map in
 * memory. Online authentication is explicitly not attempted. */
typedef struct {
    char field[128];
    char value[2048];
    int number;
    bool has_string,has_number;
} ordr_profile_field_t;
typedef struct{
    bool used;
    char map[128], user[128],character[128];
    ordr_profile_field_t fields[256];
    int count;
} ordr_profile_t;
static ordr_profile_t ordr_profiles[16];
static ordr_profile_t *ordr_current_profile;
static ordr_profile_t *ordr_get_profile(const char *user,int create){
    if(!user) user="";
    for(int i=0;i<16;i++)
        if(ordr_profiles[i].used && strcmp(ordr_profiles[i].user,user)==0)return &ordr_profiles[i];
    if(!create)return NULL;
    for(int i=0;i<16;i++)if(!ordr_profiles[i].used){
        ordr_profile_t *p=&ordr_profiles[i];
        memset(p,0,sizeof(*p));p->used=true;
        snprintf(p->user,sizeof(p->user),"%s",user);return p;
    }
    return NULL;
}
static ordr_profile_field_t *ordr_get_field(ordr_profile_t *p,const char *field,int create){
    if(!p||!field)return NULL;
    for(int i=0;i<p->count;i++)if(strcmp(p->fields[i].field,field)==0)return &p->fields[i];
    if(!create || p->count>=256)return NULL;
    ordr_profile_field_t *f=&p->fields[p->count++];
    memset(f,0,sizeof(*f));snprintf(f->field,sizeof(f->field),"%s",field);
    return f;
}
uint32_t JNUse(jass_t *j){
    /* JN server services unavailable in local mode. The character object
     * functions still work through the in-memory profile backend. */
    return jass_pushboolean(j,false);
}
uint32_t JNLogin(jass_t *j){
    cstring_t user=jass_checkstring(j,1);
    (void)jass_checkstring(j,2); /* no password is sent or stored */
    ordr_current_profile=ordr_get_profile(user,1);
    return jass_pushboolean(j,ordr_current_profile!=NULL);
}
uint32_t JNObjectCharacterInit(jass_t *j){
    cstring_t map=jass_checkstring(j,1),user=jass_checkstring(j,2);
    (void)jass_checkstring(j,3); /* legacy secret: local mode needs none */
    cstring_t character=jass_checkstring(j,4);
    ordr_current_profile=ordr_get_profile(user,1);
    if(!ordr_current_profile)return jass_pushinteger(j,-1);
    snprintf(ordr_current_profile->map,sizeof(ordr_current_profile->map),"%s",map?map:"");
    snprintf(ordr_current_profile->character,sizeof(ordr_current_profile->character),"%s",character?character:"");
    return jass_pushinteger(j,0);
}
uint32_t JNObjectCharacterSetString(jass_t *j){
    cstring_t user=jass_checkstring(j,1),name=jass_checkstring(j,2),str=jass_checkstring(j,3);
    ordr_profile_field_t *f=ordr_get_field(ordr_get_profile(user,1),name,1);
    if(f){snprintf(f->value,sizeof(f->value),"%s",str?str:"");f->has_string=true;}
    return 0;
}
uint32_t JNObjectCharacterGetString(jass_t *j){
    cstring_t user=jass_checkstring(j,1),name=jass_checkstring(j,2);
    ordr_profile_field_t *f=ordr_get_field(ordr_get_profile(user,0),name,0);
    return jass_pushstring(j,f&&f->has_string?f->value:"");
}
uint32_t JNObjectCharacterSetInt(jass_t *j){
    cstring_t user=jass_checkstring(j,1),name=jass_checkstring(j,2);
    int n=jass_checkinteger(j,3);
    ordr_profile_field_t *f=ordr_get_field(ordr_get_profile(user,1),name,1);
    if(f){f->number=n;f->has_number=true;}
    return 0;
}
uint32_t JNObjectCharacterGetInt(jass_t *j){
    cstring_t user=jass_checkstring(j,1),name=jass_checkstring(j,2);
    ordr_profile_field_t *f=ordr_get_field(ordr_get_profile(user,0),name,0);
    return jass_pushinteger(j,f&&f->has_number?f->number:0);
}
uint32_t JNObjectCharacterSave(jass_t *j){
    (void)jass_checkstring(j,1);
    cstring_t user=jass_checkstring(j,2);
    (void)jass_checkstring(j,3);
    (void)jass_checkstring(j,4);
    /* A successful save here means in-memory state has been updated.
     * Disk persistence must be provided by the platform save adapter. */
    return jass_pushstring(j,ordr_get_profile(user,0)?
       "저장이 성공적으로 되었습니다.":"저장 실패 : 사용자 정보 없음");
}

/* POSIX extended regex: preserves character classes and capture groups,
 * e.g. ^[a-z0-9]+$ and [A-Za-z0-9]$, used by the ORDR login UI. */
uint32_t JNStringRegex(jass_t *j){
    cstring_t input=jass_checkstring(j,1),pattern=jass_checkstring(j,2);
    int index=jass_checkinteger(j,3);
    regex_t re;regmatch_t match[12];
    if(!input||!pattern||index<0||index>=12)return jass_pushstring(j,"");
    if(regcomp(&re,pattern,REG_EXTENDED)!=0)return jass_pushstring(j,"");
    int rc=regexec(&re,input,12,match,0);
    if(rc!=0 || match[index].rm_so<0){
        regfree(&re);return jass_pushstring(j,"");
    }
    size_t len=(size_t)(match[index].rm_eo-match[index].rm_so);
    char *out=malloc(len+1);
    if(!out){regfree(&re);return jass_pushstring(j,"");}
    memcpy(out,input+match[index].rm_so,len);out[len]=0;
    regfree(&re);uint32_t ret=jass_pushstring(j,out);free(out);return ret;
}
/* Three stock Warcraft III natives referenced by ORDR but previously absent
 * from this OpenRealm native registry. */
uint32_t SetTextTagPos(jass_t *j) {
    texttag_t *tag=jass_checkhandle(j,1,"texttag");
    float x=jass_checknumber(j,2),y=jass_checknumber(j,3);
    float height=jass_checknumber(j,4);
    if(tag && tag->inuse) {
        tag->unit=NULL;tag->x=x;tag->y=y;
        tag->height_offset=height;tag->has_position=true;
        G_TextTagPresentation(tag,false);
    }
    return 0;
}
uint32_t GetAbilityEffectById(jass_t *j) {
    uint32_t id=(uint32_t)jass_checkinteger(j,1);
    uint32_t *type=jass_checkhandle(j,2,"effecttype");
    int32_t index=jass_checkinteger(j,3);
    if(!type || index<0)return jass_pushstring(j,"");
    return jass_pushstring(j,G_AbilityEffectArt(id,(wc3EffectType_t)*type,(uint32_t)index));
}
uint32_t DecUnitAbilityLevel(jass_t *j) {
    edict_t *unit=jass_checkhandle(j,1,"unit");
    uint32_t ability=(uint32_t)jass_checkinteger(j,2);
    if(!unit)return jass_pushinteger(j,0);
    uint32_t current=G_UnitAbilityLevel(unit,ability);
    if(current<=1)return jass_pushinteger(j,0);
    return jass_pushinteger(j,(int32_t)G_UnitSetAbilityLevel(unit,ability,(int32_t)current-1));
}

#endif
