/* ORDR local single-player compatibility helpers.
 * UI frames are stored as lightweight state objects until a renderer adapter is wired.
 * No online authentication or multiplayer transport is implied.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
typedef struct { int used,parent,id,visible,enabled,alpha; float width,height; char name[128],text[1024],texture[512]; } ordr_frame_t;
static ordr_frame_t ordr_frames[4096];
static int ordr_next_frame=2;
static int ordr_frame_new(const char *name,int parent,int id) {
    if(ordr_next_frame>=4096) return 0;
    int n=ordr_next_frame++; ordr_frame_t *f=&ordr_frames[n];
    memset(f,0,sizeof(*f)); f->used=1;f->parent=parent;f->id=id;f->visible=1;f->enabled=1;f->alpha=255;
    snprintf(f->name,sizeof(f->name),"%s",name?name:"");return n;
}
static ordr_frame_t *ordr_frame(int id){return id>0&&id<ordr_next_frame&&ordr_frames[id].used?&ordr_frames[id]:NULL;}
uint32_t DzGetGameUI(jass_t *j){return jass_pushinteger(j,1);}
uint32_t DzCreateFrame(jass_t *j){const char *s=jass_checkstring(j,1);int p=jass_checkinteger(j,2),id=jass_checkinteger(j,3);return jass_pushinteger(j,ordr_frame_new(s,p,id));}
uint32_t DzCreateFrameByTagName(jass_t *j){const char *s=jass_checkstring(j,2);int p=jass_checkinteger(j,3),id=jass_checkinteger(j,5);return jass_pushinteger(j,ordr_frame_new(s,p,id));}
uint32_t DzFrameShow(jass_t *j){ordr_frame_t*f=ordr_frame(jass_checkinteger(j,1));int b=jass_checkboolean(j,2);if(f)f->visible=b;return 0;}
uint32_t DzFrameSetEnable(jass_t *j){ordr_frame_t*f=ordr_frame(jass_checkinteger(j,1));int b=jass_checkboolean(j,2);if(f)f->enabled=b;return 0;}
uint32_t DzFrameSetText(jass_t *j){ordr_frame_t*f=ordr_frame(jass_checkinteger(j,1));const char*s=jass_checkstring(j,2);if(f)snprintf(f->text,sizeof(f->text),"%s",s?s:"");return 0;}
uint32_t DzFrameGetText(jass_t *j){ordr_frame_t*f=ordr_frame(jass_checkinteger(j,1));return jass_pushstring(j,f?f->text:"");}
uint32_t DzFrameSetTexture(jass_t *j){ordr_frame_t*f=ordr_frame(jass_checkinteger(j,1));const char*s=jass_checkstring(j,2);if(f)snprintf(f->texture,sizeof(f->texture),"%s",s?s:"");return 0;}
uint32_t DzFrameSetSize(jass_t *j){ordr_frame_t*f=ordr_frame(jass_checkinteger(j,1));float w=jass_checknumber(j,2),h=jass_checknumber(j,3);if(f){f->width=w;f->height=h;}return 0;}
uint32_t DzFrameSetAlpha(jass_t *j){ordr_frame_t*f=ordr_frame(jass_checkinteger(j,1));int a=jass_checkinteger(j,2);if(f)f->alpha=a<0?0:a>255?255:a;return 0;}
uint32_t DzFrameFindByName(jass_t *j){const char*s=jass_checkstring(j,1);int id=jass_checkinteger(j,2);for(int i=1;i<ordr_next_frame;i++)if(ordr_frames[i].used&&ordr_frames[i].id==id&&s&&!strcmp(ordr_frames[i].name,s))return jass_pushinteger(j,i);return jass_pushinteger(j,0);}
uint32_t DzFrameSetFocus(jass_t *j){int id=jass_checkinteger(j,1);(void)jass_checkboolean(j,2);return jass_pushboolean(j,ordr_frame(id)!=NULL);}
uint32_t JNStringCount(jass_t *j){const char*s=jass_checkstring(j,1),*sub=jass_checkstring(j,2);int n=0;if(s&&sub&&*sub){size_t z=strlen(sub);while((s=strstr(s,sub))){n++;s+=z;}}return jass_pushinteger(j,n);}
uint32_t JNStringSplit(jass_t *j){const char*s=jass_checkstring(j,1),*sep=jass_checkstring(j,2);int index=jass_checkinteger(j,3);if(!s||!sep||!*sep||index<0)return jass_pushstring(j,"");size_t z=strlen(sep);for(int i=0;i<index;i++){const char*p=strstr(s,sep);if(!p)return jass_pushstring(j,"");s=p+z;}const char*e=strstr(s,sep);size_t n=e?(size_t)(e-s):strlen(s);char*out=malloc(n+1);if(!out)return jass_pushstring(j,"");memcpy(out,s,n);out[n]=0;uint32_t r=jass_pushstring(j,out);free(out);return r;}
uint32_t JNStringReplace(jass_t *j){const char*s=jass_checkstring(j,1),*old=jass_checkstring(j,2),*rep=jass_checkstring(j,3);if(!s||!old||!*old)return jass_pushstring(j,s?s:"");if(!rep)rep="";size_t a=strlen(old),b=strlen(rep),count=0;const char*p=s;while((p=strstr(p,old))){count++;p+=a;}size_t sl=strlen(s);if(b>a&&count>(SIZE_MAX-sl-1)/(b-a))return jass_pushstring(j,"");size_t len=b>=a?sl+count*(b-a):sl-count*(a-b);char*out=malloc(len+1);if(!out)return jass_pushstring(j,"");char*d=out;while((p=strstr(s,old))){size_t n=(size_t)(p-s);memcpy(d,s,n);d+=n;memcpy(d,rep,b);d+=b;s=p+a;}strcpy(d,s);uint32_t r=jass_pushstring(j,out);free(out);return r;}
