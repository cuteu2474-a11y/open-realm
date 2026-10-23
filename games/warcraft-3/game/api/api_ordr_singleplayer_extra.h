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
#include <limits.h>
#include "common/game_keys.h"

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
    jassFunc_t const *callbacks[32];
    unsigned char callback_sync[32];
} ordr_frame_extra_t;
static ordr_frame_extra_t ordr_frame_extra[4096];
static int ordr_focused_frame;
static int ordr_last_ui_frame;
static player_t *ordr_last_ui_player;
static player_t *ordr_last_key_player;
static int ordr_toc_count;
static char ordr_tocs[32][260];

/* The entry points are deliberately usable from a future game/client UI
 * adapter. Merely registering callbacks does not synthesize player input. */
/* Callable by the game UI adapter when a frame receives input. */
void ORDR_DispatchFrameEvent(jass_t *j,int id,int event) {
    if(!j || !ordr_frame(id) || event<0 || event>=32)return;
    ordr_frame_trace("dispatch-event",id,ordr_frame(id)->name);
    jassFunc_t const *callback=ordr_frame_extra[id].callbacks[event];
    if(!callback)return;
    ordr_last_ui_frame=id;
    ordr_last_ui_player=currentplayer;
    jassContext_t context={0};
    context.func=callback;
    context.playerState=currentplayer;
    context.localPlayerState=currentplayer;
    jasscoroutine_t *co=jass_startcoroutine(j,&context);
    if(co)jass_resume(jass_getroot(j),co);
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
    if(ordr_frames[id].ui && point<=FRAMEPOINT_BOTTOMRIGHT && relative_point>=0 &&
       relative_point<=FRAMEPOINT_BOTTOMRIGHT)
        UI_SetPoint(ordr_frames[id].ui,point,ordr_parent_ui(relative),relative_point,x,y);
    ordr_frames_dirty=true;
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
    if(ordr_frames[id].ui && point<=FRAMEPOINT_BOTTOMRIGHT)
        UI_SetPoint(ordr_frames[id].ui,point,NULL,FRAMEPOINT_BOTTOMLEFT,x,y);
    ordr_frames_dirty=true;
    return 0;
}
uint32_t DzFrameClearAllPoints(jass_t *j) {
    int id=jass_checkinteger(j,1);
    if(ordr_frame(id)){
        memset(ordr_frame_extra[id].anchors,0,sizeof(ordr_frame_extra[id].anchors));
        ordr_frame_extra[id].anchor_count=0;
        if(ordr_frames[id].ui){
            memset(&ordr_frames[id].ui->Points,0,sizeof(ordr_frames[id].ui->Points));
            ordr_frames[id].ui->AnyPointsSet=false;
        }
        ordr_frames_dirty=true;
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
        if(ordr_frames[id].ui){
            ordr_frames[id].ui->Font.Size=h;
            ordr_frames[id].ui->Font.Index=UI_FdfFontIndex(file,(uint32_t)(h*UI_FONT_COORD_SCALE));
            ordr_frames_dirty=true;
        }
    }
    return 0;
}
uint32_t DzFrameSetTextAlignment(jass_t *j) {
    int id=jass_checkinteger(j,1),align=jass_checkinteger(j,2);
    if(ordr_frame(id)){
        ordr_frame_extra[id].text_align=align;
        if(ordr_frames[id].ui && align>=0 && align<=10 && (align&3)!=3){
            /* Dz FRAME_ALIGN is a three-by-three point grid (0/1/2,
             * 4/5/6, 8/9/10); FDF font justification has different enums. */
            static uiFontJustificationH_t const h[]={FONT_JUSTIFYLEFT,FONT_JUSTIFYCENTER,FONT_JUSTIFYRIGHT};
            static uiFontJustificationV_t const v[]={FONT_JUSTIFYTOP,FONT_JUSTIFYMIDDLE,FONT_JUSTIFYBOTTOM};
            ordr_frames[id].ui->Font.Justification.Horizontal=h[align&3];
            ordr_frames[id].ui->Font.Justification.Vertical=v[align>>2];
            ordr_frames_dirty=true;
        }
    }
    return 0;
}
uint32_t DzFrameSetPriority(jass_t *j) {
    int id=jass_checkinteger(j,1),priority=jass_checkinteger(j,2);
    if(ordr_frame(id)){
        ordr_frame_extra[id].priority=priority;
        ordr_frames_dirty=true;
    }
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
        if(ordr_frames[id].ui){
            ordr_frames[id].ui->Portrait.model=UI_LoadModel(model,false);
            ordr_frames_dirty=true;
        }
    }
    return 0;
}
uint32_t DzLoadToc(jass_t *j) {
    cstring_t toc=jass_checkstring(j,1);
    uint32_t size=0;
    void *data;
    char *copy,*line;
    if(!toc || !*toc)return 0;
    data=gi.ReadFile(toc,&size);
    if(!data || !size || size>1024*1024){
        fprintf(stderr,"ORDR DzLoadToc: cannot read %s (%u bytes)\n",toc,(unsigned)size);
        if(data)gi.MemFree(data);
        return 0;
    }
    copy=gi.MemAlloc(size+1);
    memcpy(copy,data,size);copy[size]='\0';
    gi.MemFree(data);
    /* A TOC lists actual archive-relative FDF files, not a UI template name.
     * UI_EnsureFDF caches the data and shares its parser with the stock HUD. */
    for(line=copy;line<copy+size;){
        char *end=strpbrk(line,"\r\n"),*start=line,*last;
        if(end){*end++='\0';while(*end=='\r'||*end=='\n')end++;}
        else end=copy+size;
        if((unsigned char)start[0]==0xef && (unsigned char)start[1]==0xbb &&
           (unsigned char)start[2]==0xbf)start+=3;
        while(*start==' '||*start=='\t')start++;
        last=start+strlen(start);
        while(last>start && (last[-1]==' '||last[-1]=='\t'))*--last='\0';
        if(*start && *start!='#' && strncmp(start,"//",2)){
            for(char *p=start;*p;p++)if(*p=='/')*p='\\';
            if(!UI_EnsureFDF(start))
                fprintf(stderr,"ORDR DzLoadToc: missing FDF %s (from %s)\n",start,toc);
        }
        line=end;
    }
    gi.MemFree(copy);
    ordr_frame_trace("load-toc",0,toc);
    if(ordr_toc_count<32)snprintf(ordr_tocs[ordr_toc_count++],sizeof(ordr_tocs[0]),"%s",toc);
    return 0;
}
uint32_t DzFrameSetScriptByCode(jass_t *j) {
    int id=jass_checkinteger(j,1),event=jass_checkinteger(j,2);
    jassFunc_t const *callback=jass_checkcode(j,3);
    bool sync=jass_checkboolean(j,4);
    if(ordr_frame(id) && event>=0 && event<32){
        ordr_frame_trace("register-callback",id,ordr_frame(id)->name);
        ordr_frame_extra[id].callbacks[event]=callback;
        ordr_frame_extra[id].callback_sync[event]=(unsigned char)sync;
        if(event==1 && ordr_frames[id].ui){
            /* The universal client reports actual button clicks via OnClick;
             * the game command validates the source before dispatching JASS. */
            UI_SetOnClick(ordr_frames[id].ui,"dzframe %d",id);
            ordr_frames_dirty=true;
        }
        if(event==9 && ordr_frames[id].ui &&
           (ordr_frames[id].ui->Type==FT_EDITBOX || ordr_frames[id].ui->Type==FT_GLUEEDITBOX ||
            ordr_frames[id].ui->Type==FT_SLASHCHATBOX)){
            UI_SetOnClick(ordr_frames[id].ui,"dzedit %d",id);
            ordr_frames_dirty=true;
        }
    }
    return 0;
}
uint32_t DzFrameSetFocus(jass_t *j) {
    int id=jass_checkinteger(j,1);
    bool enable=jass_checkboolean(j,2);
    ordr_frame_t *frame=ordr_frame(id);
    if(!frame || !frame->ui)return jass_pushboolean(j,false);
    if(enable){
        if(ordr_focused_frame>0)ordr_frame_extra[ordr_focused_frame].focused=0;
        ordr_focused_frame=id;
        ordr_frame_extra[id].focused=1;
    } else if(ordr_focused_frame==id){
        ordr_focused_frame=0;
        ordr_frame_extra[id].focused=0;
    }
    ordr_frames_dirty=true;
    return jass_pushboolean(j,true);
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

/* The platform input path can forward physical and touch-mapped key actions
 * here. This entry point does not invent events without actual player input. */
void ORDR_DispatchKeyEvent(jass_t *j,int key,int status) {
    if(!j)return;
    ordr_last_key_player=currentplayer;
    for(int i=0;i<ordr_key_binding_count;i++){
        ordr_key_binding_t *e=&ordr_key_bindings[i];
        if(e->key!=key||e->status!=status)continue;
        if(e->trigger&&!e->trigger->disabled)jass_executetrigger(j,e->trigger,NULL);
        if(e->callback){
            jassContext_t ctx={0};
            ctx.func=e->callback;
            ctx.playerState=currentplayer;
            ctx.localPlayerState=currentplayer;
            jasscoroutine_t *co=jass_startcoroutine(j,&ctx);
            if(co)jass_resume(jass_getroot(j),co);
        }
    }
}
uint32_t DzGetTriggerKeyPlayer(jass_t *j){
    return jass_pushlighthandle(j,ordr_last_key_player?ordr_last_key_player:currentplayer,"player");
}

/* Retire map-owned handles before UI_ResetHud clears their FDF backing pool.
 * A prior map's callbacks must never target a replacement map's JASS VM. */
void ORDR_DzReset(void) {
    memset(ordr_frames,0,sizeof(ordr_frames));
    memset(ordr_frame_extra,0,sizeof(ordr_frame_extra));
    memset(ordr_sync_handlers,0,sizeof(ordr_sync_handlers));
    memset(ordr_key_bindings,0,sizeof(ordr_key_bindings));
    ordr_next_frame=2;
    ordr_frame_trace_count=0;
    ordr_frames_dirty=false;
    ordr_last_ui_frame=0;
    ordr_focused_frame=0;
    ordr_last_ui_player=ordr_last_key_player=ordr_sync_player=NULL;
    ordr_sync_handler_count=ordr_sync_depth=ordr_key_binding_count=0;
    ordr_toc_count=0;
}

/* Dz priorities are sibling-local and the most recently created equal-priority
 * sibling goes last. Write the real FDF handles in stable priority order so
 * both client draw order and reverse-order hit testing agree with DzAPI. */
static void ordr_write_frame_tree(int id) {
    ordr_frame_t *frame=ordr_frame(id);
    if(!frame || !frame->ui || frame->ui->hidden)return;
    if(frame->ui->Type==FT_EDITBOX || frame->ui->Type==FT_GLUEEDITBOX ||
       frame->ui->Type==FT_SLASHCHATBOX)
        UI_WriteFrameValue(frame->ui,ordr_frame_extra[id].focused?1.0f:0.0f);
    else UI_WriteFrame(frame->ui);
    int last_prio=INT_MIN, last_id=0;
    while(true){
        int next=0, next_prio=INT_MAX;
        for(int n=2;n<ordr_next_frame;n++){
            ordr_frame_t *child=&ordr_frames[n];
            if(!child->used || child->parent!=id || !child->ui || child->ui->hidden ||
               UI_IsEmbeddedControlArtPart(frame->ui,child->ui))continue;
            int prio=ordr_frame_extra[n].priority;
            if(prio<last_prio || (prio==last_prio && n<=last_id))continue;
            if(!next || prio<next_prio || (prio==next_prio && n<next)){
                next=n;
                next_prio=prio;
            }
        }
        if(!next)break;
        ordr_write_frame_tree(next);
        last_prio=next_prio;
        last_id=next;
    }
}

/* The game materializes scripted FDF roots using the existing svc_layout
 * serializer. UI text, texture and anchors are evaluated before transmission;
 * the universal client does not need to know what a DzAPI function is. */
void ORDR_DzRenderFrames(void) {
    ordr_frame_t *root=ordr_frame(1);
    bool emitted=false;
    if(!ordr_frames_dirty || !root || !root->ui || ordr_next_frame<=2)return;
    FOR_LOOP(i,game.max_clients) {
        gameClient_t *client=&game.clients[i];
        if(!client->connected)continue;
        edict_t *ent=G_GetPlayerEntityByNumber(client->ps.number);
        if(!ent || !ent->client)continue;
        UI_SetCurrentClient(ent->client);
        UI_WriteStart(LAYER_GAME_3);
        ordr_write_frame_tree(1);
        UI_WriteEnd(ent);
        UI_SetCurrentClient(NULL);
        emitted=true;
    }
    if(emitted)ordr_frames_dirty=false;
}

/* Only a live, visible, enabled clickable frame may forward client input into
 * the current script. This prevents fabricated/stale button handles from
 * calling registered JASS code outside the actual displayed frame tree. */
void ORDR_DzClickFrame(edict_t *ent,uint32_t handle) {
    ordr_frame_t *f=ordr_frame(handle);
    player_t *old;
    if(!ent || !ent->client || !ent->client->connected || !level.vm || !G_IsSinglePlayer() ||
       !f || !f->ui || !ordr_frame_extra[handle].callbacks[1])return;
    for(int p=(int)handle;p>0;){
        ordr_frame_t *ancestor=ordr_frame(p);
        if(!ancestor || !ancestor->visible || !ancestor->enabled)return;
        p=ancestor->parent;
    }
    old=currentplayer;
    currentplayer=&ent->client->ps;
    ORDR_DispatchFrameEvent(level.vm,(int)handle,1);
    currentplayer=old;
}

/* Only an enabled, visible script-owned editor may accept client text. */
void ORDR_DzEditFrame(edict_t *ent,uint32_t handle,cstring_t text) {
    ordr_frame_t *f=ordr_frame(handle);
    player_t *old;
    if(!ent || !ent->client || !ent->client->connected || !level.vm || !G_IsSinglePlayer() ||
       !f || !f->ui || !text ||
       (f->ui->Type!=FT_EDITBOX && f->ui->Type!=FT_GLUEEDITBOX && f->ui->Type!=FT_SLASHCHATBOX))return;
    for(int p=(int)handle;p>0;){
        ordr_frame_t *ancestor=ordr_frame(p);
        if(!ancestor || !ancestor->visible || !ancestor->enabled)return;
        p=ancestor->parent;
    }
    snprintf(f->text,sizeof(f->text),"%.255s",text);
    UI_SetText(f->ui,"%s",f->text);
    ordr_frames_dirty=true;
    old=currentplayer;
    currentplayer=&ent->client->ps;
    ORDR_DispatchFrameEvent(level.vm,(int)handle,9);
    currentplayer=old;
}

/* Keep Dz key codes in the game. The client command transports one sampled
 * key edge; it does not grant access to other players' script contexts. */
void ORDR_DzKeyInput(edict_t *ent,int key,int down) {
    player_t *old;
    if(!ent || !ent->client || !ent->client->connected || !level.vm || !G_IsSinglePlayer() ||
       key<0 || key>255 || (down!=0 && down!=1))return;
    /* The generic client reports Quake key IDs. DzAPI expects Windows VKs;
     * map these at the game boundary so the universal client stays agnostic. */
    static const struct { int client, vk; } special[]={
        {K_F1,0x70},{K_F2,0x71},{K_F3,0x72},{K_F4,0x73},{K_F5,0x74},{K_F6,0x75},
        {K_F7,0x76},{K_F8,0x77},{K_F9,0x78},{K_F10,0x79},{K_F11,0x7a},{K_F12,0x7b},
        {K_UPARROW,0x26},{K_DOWNARROW,0x28},{K_LEFTARROW,0x25},{K_RIGHTARROW,0x27},
        {K_PAGEUP,0x21},{K_PAGEDOWN,0x22},{K_HOME,0x24},{K_END,0x23},
        {K_SHIFT,0x10},{K_CTRL,0x11},{K_ALT,0x12},
        {127,0x2e},
    };
    if(key>='a' && key<='z')key+=('A'-'a');
    else for(size_t i=0;i<sizeof(special)/sizeof(*special);i++)
        if(key==special[i].client){key=special[i].vk;break;}
    old=currentplayer;
    currentplayer=&ent->client->ps;
    ORDR_DispatchKeyEvent(level.vm,key,down);
    currentplayer=old;
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

/* The engine chooses a private writable save path appropriate for iOS,
 * Windows and Linux. No login password or secret is written to disk. */
static uint64_t ordr_profile_hash(const char *value,uint64_t hash){
    if(!value)value="";
    for(const unsigned char *p=(const unsigned char*)value;*p;p++){
        hash^=(uint64_t)*p;hash*=UINT64_C(1099511628211);
    }
    hash^=255;hash*=UINT64_C(1099511628211);
    return hash;
}
static bool ordr_profile_disk_path(ordr_profile_t const *p,char *out,size_t size,bool write){
    uint64_t h=UINT64_C(14695981039346656037);
    h=ordr_profile_hash(p->map,h);
    h=ordr_profile_hash(p->user,h);
    h=ordr_profile_hash(p->character,h);
    char name[64];
    snprintf(name,sizeof(name),"ordr-local-%016llx.dat",(unsigned long long)h);
    if(write)return gi.ScriptCachePath && gi.ScriptCachePath(name,out,(uint32_t)size);
    return gi.ScriptCacheReadPath && gi.ScriptCacheReadPath(name,out,(uint32_t)size);
}
static bool ordr_profile_disk_load(ordr_profile_t *p){
    char path[MAX_PATHLEN],magic[8];
    unsigned int count=0;
    if(!ordr_profile_disk_path(p,path,sizeof(path),false))return false;
    FILE *file=fopen(path,"rb");
    if(!file)return false;
    bool ok=fread(magic,1,sizeof(magic),file)==sizeof(magic) &&
        memcmp(magic,"ORDRSP01",sizeof(magic))==0 &&
        fread(&count,sizeof(count),1,file)==1 && count<=256;
    if(ok){
        memset(p->fields,0,sizeof(p->fields));
        p->count=0;
        for(unsigned int i=0;i<count;i++){
            ordr_profile_field_t field;
            if(fread(&field,sizeof(field),1,file)!=1 ||
               !memchr(field.field,0,sizeof(field.field)) ||
               !memchr(field.value,0,sizeof(field.value))){
                ok=false;break;
            }
            p->fields[p->count++]=field;
        }
    }
    if(!ok){p->count=0;memset(p->fields,0,sizeof(p->fields));}
    fclose(file);
    return ok;
}
static bool ordr_profile_disk_save(ordr_profile_t const *p){
    char path[MAX_PATHLEN],tmp[MAX_PATHLEN];
    if(!ordr_profile_disk_path(p,path,sizeof(path),true))return false;
    if(snprintf(tmp,sizeof(tmp),"%s.tmp",path)>=(int)sizeof(tmp))return false;
    FILE *f=fopen(tmp,"wb");if(!f)return false;
    const char magic[8]={'O','R','D','R','S','P','0','1'};
    unsigned int n=(unsigned int)p->count;
    bool ok=fwrite(magic,1,sizeof(magic),f)==sizeof(magic) &&
        fwrite(&n,sizeof(n),1,f)==1;
    for(unsigned int i=0;ok&&i<n;i++)
        ok=fwrite(&p->fields[i],sizeof(p->fields[i]),1,f)==1;
    if(fflush(f)!=0)ok=false;
    if(fclose(f)!=0)ok=false;
    if(!ok){remove(tmp);return false;}
    if(rename(tmp,path)!=0){remove(tmp);return false;}
    return true;
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
    /* A fresh identity starts with its saved data, or an empty local profile. */
    ordr_current_profile->count=0;
    memset(ordr_current_profile->fields,0,sizeof(ordr_current_profile->fields));
    (void)ordr_profile_disk_load(ordr_current_profile);
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
    ordr_profile_t *profile=ordr_get_profile(user,0);
    if(!profile)return jass_pushstring(j,"저장 실패 : 사용자 정보 없음");
    return jass_pushstring(j,ordr_profile_disk_save(profile)?
        "저장이 성공적으로 되었습니다.":"저장 실패 : 로컬 파일 쓰기 실패");
}

/* POSIX extended regex: preserves character classes and capture groups,
 * e.g. ^[a-z0-9]+$ and [A-Za-z0-9]$, used by the ORDR login UI. */
/* ORDR uses JNStringCount with regular-expression character classes
 * (e.g. "[forbidden]"), not just fixed substring matching. */
uint32_t ORDR_JNStringCount(jass_t *j) {
    cstring_t input=jass_checkstring(j,1),pattern=jass_checkstring(j,2);
    if(!input||!pattern||!*pattern)return jass_pushinteger(j,0);
    regex_t re;
    if(regcomp(&re,pattern,REG_EXTENDED)!=0)return jass_pushinteger(j,0);
    int count=0;const char *cursor=input;regmatch_t match;
    while(*cursor && regexec(&re,cursor,1,&match,0)==0) {
        if(match.rm_so<0||match.rm_eo<=match.rm_so)break;
        ++count;cursor+=match.rm_eo;
    }
    regfree(&re);
    return jass_pushinteger(j,count);
}
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
