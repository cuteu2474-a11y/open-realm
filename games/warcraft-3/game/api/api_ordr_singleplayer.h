/* ORDR local single-player compatibility helpers.
 * DzAPI frames own real Warcraft FDF clones in the gameplay HUD frame pool.
 * No online authentication or multiplayer transport is implied. */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
typedef struct {
    int used, parent, id, visible, enabled, alpha;
    float width, height;
    char name[128], text[1024], texture[512];
    frameDef_t *ui;
} ordr_frame_t;
static ordr_frame_t ordr_frames[4096];
static int ordr_next_frame=2;
static int ordr_frame_trace_count=0;
static bool ordr_frames_dirty;
static void ORDR_DzLoseFocusInsideFrame(int ancestor_id);
static void ordr_frame_root_init(void) {
    if (!ordr_frames[1].used) {
        ordr_frames[1].used=1;
        ordr_frames[1].visible=1;
        ordr_frames[1].enabled=1;
        ordr_frames[1].alpha=255;
        snprintf(ordr_frames[1].name,sizeof(ordr_frames[1].name),"GAME_UI");
    }
    if (!ordr_frames[1].ui) {
        ordr_frames[1].ui=UI_Spawn(FT_FRAME,NULL);
        if (ordr_frames[1].ui) {
            UI_SetSize(ordr_frames[1].ui,UI_BASE_WIDTH,UI_BASE_HEIGHT);
            UI_SetPoint(ordr_frames[1].ui,FRAMEPOINT_BOTTOMLEFT,NULL,FRAMEPOINT_BOTTOMLEFT,0,0);
        }
    }
}
static void ordr_frame_trace(const char *action,int handle,const char *detail) {
    if (ordr_frame_trace_count++ < 200)
        fprintf(stderr,"ORDR-UI: %s handle=%d %s\n",action,handle,detail?detail:"");
}

/* The same registry represents JASS handles and cloned FDF child frames.
 * A child is visible to DzFrameFindByName with its creator's context ID. */
static int ordr_frame_new(const char *name,int parent,int id,frameDef_t *ui) {
    ordr_frame_root_init();
    if(ordr_next_frame>=4096) { ordr_frame_trace("registry-full",0,name);return 0; }
    int n=ordr_next_frame++; ordr_frame_t *f=&ordr_frames[n];
    memset(f,0,sizeof(*f)); f->used=1;f->parent=parent;f->id=id;f->visible=1;f->enabled=1;f->alpha=255;
    f->ui=ui;
    if (ui) {
        /* An FDF may create hidden/disabled children before DzFrameShow runs;
         * the script mirror must begin with the authored state. */
        f->visible=!ui->hidden;
        f->enabled=!ui->disabled;
        f->width=ui->Width; f->height=ui->Height;
        snprintf(f->text,sizeof(f->text),"%s",ui->Text?ui->Text:"");
        /* A newly created list/popup has no selected row. Leaving the zero
         * sentinel suppressed event 10 on the first real row-zero click. */
        if (ui->Type==FT_LISTBOX) ui->ListBox.SelectedIndex=-1;
        if (ui->Type==FT_POPUPMENU || ui->Type==FT_GLUEPOPUPMENU) ui->Popup.SelectedIndex=-1;
    }
    snprintf(f->name,sizeof(f->name),"%s",name?name:"");ordr_frame_trace("create",n,f->name);
    if(ui && (ui->Type==FT_EDITBOX || ui->Type==FT_GLUEEDITBOX || ui->Type==FT_SLASHCHATBOX))
        UI_SetOnClick(ui,"dzedit %d",n);
    ordr_frames_dirty=true;
    return n;
}
static ordr_frame_t *ordr_frame(int id){return id>0&&id<ordr_next_frame&&ordr_frames[id].used?&ordr_frames[id]:NULL;}
/* Cloned descendants are native Dz handles too; do not flatten their parent
 * relationship or invent duplicates when a template contains nested frames. */
static void ordr_register_children(frameDef_t *parent,int parent_id,int context) {
    FOR_LOOP(i,MAX_UI_CLASSES) {
        frameDef_t *child=&frames[i];
        if (!child->inuse || child->Parent!=ordr_frames[parent_id].ui) continue;
        int id=ordr_frame_new(child->Name,parent_id,context,child);
        if (!id) return;
        ordr_register_children(child,id,context);
    }
}
static frameDef_t *ordr_parent_ui(int parent) {
    ordr_frame_t *p=ordr_frame(parent);
    return p?p->ui:NULL;
}
/* Only types that the common WC3 FDF serializer knows are accepted. */
static FRAMETYPE ordr_frame_type(const char *name) {
    static const struct { const char *name; FRAMETYPE type; } types[]={
        {"BACKDROP",FT_BACKDROP},{"TEXT",FT_TEXT},{"TEXTBUTTON",FT_TEXTBUTTON},
        {"GLUETEXTBUTTON",FT_GLUETEXTBUTTON},{"BUTTON",FT_BUTTON},{"SIMPLEBUTTON",FT_SIMPLEBUTTON},
        {"SIMPLEFRAME",FT_SIMPLEFRAME},{"FRAME",FT_FRAME},{"MODEL",FT_MODEL},
        {"SPRITE",FT_SPRITE},{"HIGHLIGHT",FT_HIGHLIGHT},{"TEXTAREA",FT_TEXTAREA},
        {"EDITBOX",FT_EDITBOX},{"GLUEEDITBOX",FT_GLUEEDITBOX},{"SIMPLEFONTSTRING",FT_STRING},
        {"CHECKBOX",FT_CHECKBOX},{"GLUECHECKBOX",FT_GLUECHECKBOX},
        {"SIMPLECHECKBOX",FT_SIMPLECHECKBOX},{"POPUPMENU",FT_POPUPMENU},
        {"GLUEPOPUPMENU",FT_GLUEPOPUPMENU},{"LISTBOX",FT_LISTBOX},{"SCROLLBAR",FT_SCROLLBAR},
        {"STRING",FT_STRING},{"TEXTURE",FT_TEXTURE},{"SIMPLESTATUSBAR",FT_SIMPLESTATUSBAR},
    };
    for (size_t i=0;i<sizeof(types)/sizeof(*types);i++)
        if (name && !strcmp(name,types[i].name)) return types[i].type;
    return FT_NONE;
}
uint32_t DzGetGameUI(jass_t *j){ordr_frame_root_init();return jass_pushinteger(j,1);}
uint32_t DzCreateFrame(jass_t *j){
    const char *name=jass_checkstring(j,1);
    int parent=jass_checkinteger(j,2),ctx=jass_checkinteger(j,3),id;
    frameDef_t *source=UI_FindFrame(name),*ui=NULL;
    if (source) ui=UI_CloneFrameTree(source,ordr_parent_ui(parent));
    else fprintf(stderr,"ORDR DzCreateFrame: FDF template not found: %s\n",name?name:"");
    /* Missing templates must not masquerade as live frames: such handles
     * cannot render or receive the Dz click that a map expects. */
    if (!ui) return jass_pushinteger(j,0);
    id=ordr_frame_new(name,parent,ctx,ui);
    if (ui && id) ordr_register_children(ui,id,ctx);
    return jass_pushinteger(j,id);
}
uint32_t DzCreateFrameByTagName(jass_t *j){
    const char *type=jass_checkstring(j,1),*name=jass_checkstring(j,2);
    int parent=jass_checkinteger(j,3),ctx=jass_checkinteger(j,5),id;
    const char *tmpl=jass_checkstring(j,4);
    FRAMETYPE kind=ordr_frame_type(type);
    frameDef_t *ui=NULL;
    if (kind!=FT_NONE && tmpl && *tmpl && !UI_FindFrame(tmpl)) {
        fprintf(stderr,"ORDR DzCreateFrameByTagName: FDF template not found: %s\n",tmpl);
        return jass_pushinteger(j,0);
    }
    if (kind!=FT_NONE) {
        ui=UI_Spawn(kind,ordr_parent_ui(parent));
        if (ui) {
            snprintf(ui->Name,sizeof(ui->Name),"%s",name?name:"");
            if (tmpl && *tmpl) UI_InheritFrom(ui,tmpl);
        }
    } else fprintf(stderr,"ORDR DzCreateFrameByTagName: unknown type: %s\n",type?type:"");
    if (!ui) return jass_pushinteger(j,0);
    id=ordr_frame_new(name,parent,ctx,ui);
    return jass_pushinteger(j,id);
}
uint32_t DzFrameShow(jass_t *j){ordr_frame_t*f=ordr_frame(jass_checkinteger(j,1));int b=jass_checkboolean(j,2);if(f){f->visible=b;if(f->ui)UI_SetHidden(f->ui,!b);if(!b)ORDR_DzLoseFocusInsideFrame((int)(f-ordr_frames));ordr_frames_dirty=true;ordr_frame_trace(b?"show":"hide",(int)(f-ordr_frames),f->name);}return 0;}
uint32_t DzFrameSetEnable(jass_t *j){ordr_frame_t*f=ordr_frame(jass_checkinteger(j,1));int b=jass_checkboolean(j,2);if(f){f->enabled=b;if(f->ui)UI_SetEnabled(f->ui,b);if(!b)ORDR_DzLoseFocusInsideFrame((int)(f-ordr_frames));ordr_frames_dirty=true;}return 0;}
uint32_t DzFrameSetText(jass_t *j){ordr_frame_t*f=ordr_frame(jass_checkinteger(j,1));const char*s=jass_checkstring(j,2);if(f){snprintf(f->text,sizeof(f->text),"%s",s?s:"");if(f->ui)UI_SetText(f->ui,"%s",f->text);ordr_frames_dirty=true;ordr_frame_trace("text",(int)(f-ordr_frames),f->text);}return 0;}
uint32_t DzFrameGetText(jass_t *j){ordr_frame_t*f=ordr_frame(jass_checkinteger(j,1));return jass_pushstring(j,f?f->text:"");}
/* Preserve the authored UI art path in the ORDR execution log and update the
 * FDF-backed image/Backdrop payload consumed by the gameplay serializer. */
uint32_t DzFrameSetTexture(jass_t *j) {
    int id=jass_checkinteger(j,1);
    ordr_frame_t *f=ordr_frame(id);
    const char *s=jass_checkstring(j,2);
    static unsigned seen;
    if(f){
        if(s && *s && strcmp(f->texture,s) && seen<120){
            char detail[320];
            snprintf(detail,sizeof(detail),"frame=%d name=%.70s asset=%.205s",id,f->name,s);
            jass_ordr_log_event(j,"ORDR_FRAME_TEXTURE",detail);
            seen++;
        }
        snprintf(f->texture,sizeof(f->texture),"%s",s?s:"");
        if(f->ui && s){
            /* DzAPI's BACKDROP texture is the backdrop background payload;
             * setting Texture.Image alone makes no visible backdrop art. */
            if(f->ui->Type==FT_BACKDROP){
                f->ui->Backdrop.Background=UI_LoadTexture(s,false);
                f->ui->Backdrop.TileBackground=jass_checkinteger(j,3)!=0;
            } else UI_SetTexture(f->ui,s,false);
        }
        ordr_frames_dirty=true;
    }
    return 0;
}
uint32_t DzFrameSetSize(jass_t *j){ordr_frame_t*f=ordr_frame(jass_checkinteger(j,1));float w=jass_checknumber(j,2),h=jass_checknumber(j,3);if(f){f->width=w;f->height=h;if(f->ui)UI_SetSize(f->ui,w,h);ordr_frames_dirty=true;}return 0;}
uint32_t DzFrameSetAlpha(jass_t *j){ordr_frame_t*f=ordr_frame(jass_checkinteger(j,1));int a=jass_checkinteger(j,2);if(f){f->alpha=a<0?0:a>255?255:a;if(f->ui)f->ui->Color.a=f->alpha;ordr_frames_dirty=true;}return 0;}
/* DzAPI resolves the most recently created frame in a repeated-name context. */
uint32_t DzFrameFindByName(jass_t *j){const char*s=jass_checkstring(j,1);int id=jass_checkinteger(j,2);for(int i=ordr_next_frame-1;i>=1;i--)if(ordr_frames[i].used&&ordr_frames[i].id==id&&s&&!strcmp(ordr_frames[i].name,s))return jass_pushinteger(j,i);return jass_pushinteger(j,0);}
uint32_t DzFrameSetFocus(jass_t *j);
uint32_t JNStringCount(jass_t *j){const char*s=jass_checkstring(j,1),*sub=jass_checkstring(j,2);int n=0;if(s&&sub&&*sub){size_t z=strlen(sub);while((s=strstr(s,sub))){n++;s+=z;}}return jass_pushinteger(j,n);}
uint32_t JNStringSplit(jass_t *j){const char*s=jass_checkstring(j,1),*sep=jass_checkstring(j,2);int index=jass_checkinteger(j,3);if(!s||!sep||!*sep||index<0)return jass_pushstring(j,"");size_t z=strlen(sep);for(int i=0;i<index;i++){const char*p=strstr(s,sep);if(!p)return jass_pushstring(j,"");s=p+z;}const char*e=strstr(s,sep);size_t n=e?(size_t)(e-s):strlen(s);char*out=malloc(n+1);if(!out)return jass_pushstring(j,"");memcpy(out,s,n);out[n]=0;uint32_t r=jass_pushstring(j,out);free(out);return r;}
uint32_t JNStringReplace(jass_t *j){const char*s=jass_checkstring(j,1),*old=jass_checkstring(j,2),*rep=jass_checkstring(j,3);if(!s||!old||!*old)return jass_pushstring(j,s?s:"");if(!rep)rep="";size_t a=strlen(old),b=strlen(rep),count=0;const char*p=s;while((p=strstr(p,old))){count++;p+=a;}size_t sl=strlen(s);if(b>a&&count>(SIZE_MAX-sl-1)/(b-a))return jass_pushstring(j,"");size_t len=b>=a?sl+count*(b-a):sl-count*(a-b);char*out=malloc(len+1);if(!out)return jass_pushstring(j,"");char*d=out;while((p=strstr(s,old))){size_t n=(size_t)(p-s);memcpy(d,s,n);d+=n;memcpy(d,rep,b);d+=b;s=p+a;}strcpy(d,s);uint32_t r=jass_pushstring(j,out);free(out);return r;}
