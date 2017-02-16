

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "guiparser.h"
#include "util.h"

/* syntax :

   object = { leaf node | non-leaf node }

   leaf node:
   name:class[local-options]{placement-in-parent-options}

   non-leaf node:
   name:class[local-options]{placement-in-parent-options}(object,object,...)

   string: '...'

   name, [options] and {placement-in-parent} are optional
   (except on widgets that do require options for creation)

   classes and options currently accepted:

   class   container?  options: ([] and {} mean choices, not real syntax)

   label       N       text=string,[{left|center|right}]
   spinbutton  N       val=f,min=f,max=f,step=f,page=f,pagelen=f,
                       [accel=f],[digits=n]
   drawingarea N       [w=n,h=n],[events=[press+release+drag1+drag2+drag3+drag+move]]
                       (expose is always in, wouldn't make sense to turn expose off)
   dropbox     N       options=string
   radiobox    N       title=string,options=string
   curve       N

   button      Y(1)    [text=string],[border=n]
   checkbox    N       [text=string]
   frame       Y       [title=string],[border=n]
   hbox        Y       [homog],[gap=n],[border=n] (default gap=0)
   vbox        Y       [homog],[gap=n],[border=n] (default gap=0)
   table       Y       [homog],w=n,h=n,[border=n]

   (1) - only a container if no text is given

   {placement-in-parent} options for each class of parent:

   frame,button        none

   hbox,vbox           [packend],[gap=n],[expand],[fill]

   table               l=n,r=n,[t=n],[b=n],[vgap=n],[hgap=n],[gap=n],
                       [h[e][f][s]],[v[e][f][s]]
                       (if vgap and hgap aren't given, gap is assumed)
                       (e = expand, f=fill, s=shrink)

*/

/* classes */

#define GUIP_LABEL       0
#define GUIP_BUTTON      1
#define GUIP_HBOX        2
#define GUIP_FRAME       3
#define GUIP_VBOX        4
#define GUIP_TABLE       5
#define GUIP_SPINBUTTON  6
#define GUIP_DRAWINGAREA 7
#define GUIP_DROPBOX     8
#define GUIP_CHECKBOX    9
#define GUIP_CURVE       10
#define GUIP_RADIOBOX    11

#define NCLASSES 12

static char * klass_names[] = { "label", "button", "hbox", "frame", 
				"vbox",  "table",  "spinbutton", 
				"drawingarea", "dropbox", "checkbox", 
				"curve", "radiobox" };

/* If you come up with a widget tree deeper than 50 levels, you
   don't deserve to code GUIs */

GtkWidget     *stack[50];
GtkAdjustment *sa[50];
void          *se[50];
int            sk[50]; /* class of object */
int            stacktop = 0;

/* name table */

typedef struct _guinametable {
  char       name[64];
  GtkWidget     *object;
  GtkAdjustment *adj;
  void          *ext;
  struct    _guinametable *next;
} GuiNameTable;

GuiNameTable *guitable = 0;

/* private funcs */

GtkWidget * guip_create_recursive(const char *desc);
char      * guip_link_stack_top(char *p);

char      * guip_make_label(char *p);
char      * guip_make_frame(char *p);
char      * guip_make_box(char *p, int h);
char      * guip_make_table(char *p);
char      * guip_make_button(char *p);
char      * guip_make_spinbutton(char *p);
char      * guip_make_drawingarea(char *p);
char      * guip_make_dropbox(char *p);
char      * guip_make_checkbox(char *p);
char      * guip_make_curve(char *p);
char      * guip_make_radiobox(char *p);

void   clear_name_table();
void   clear_name_node(GuiNameTable *node);

char * eat_alpha_token(char *ptr, char *dest);
char * eat_string_token(char *ptr, char *dest);
char * eat_num_token(char *ptr, char *dest);
char * eat_fnum_token(char *ptr, char *dest);
int    klass_of(char *name);


GtkWidget * guip_create(const char *desc) {
  clear_name_table();
  stacktop = -1;
  guip_create_recursive(desc);
  return stack[0];
}

GtkWidget * guip_create_recursive(const char *desc) {
  GtkWidget *parent, *child=0;
  int  hasname = 0;
  char token[64],name[64],klass[64];
  int  k;
  GuiNameTable *nt;
  static char *p;

  //  printf("gc_rec called on [%s]\n\n",desc);

  if (stacktop == -1) {
    parent = 0;
  } else {
    parent = stack[stacktop];
  }

  p = (char *) desc;
  p = eat_alpha_token(p, token);

  if (*p == ':') {
    hasname = 1;
    strcpy(name, token);
    p = eat_alpha_token(p+1,token);
    strcpy(klass, token);
  } else {
    hasname = 0;
    strcpy(klass, token);
  }

  k = klass_of(klass);

  switch(k) {
  case GUIP_LABEL: 
    p = guip_make_label(p);
    child = stack[stacktop];      
    break;
  case GUIP_FRAME:
    p = guip_make_frame(p);
    child = stack[stacktop];
    break;
  case GUIP_HBOX:
    p = guip_make_box(p,1);
    child = stack[stacktop];
    break;
  case GUIP_VBOX:
    p = guip_make_box(p,0);
    child = stack[stacktop];
    break;
  case GUIP_TABLE:
    p = guip_make_table(p);
    child = stack[stacktop];
    break;
  case GUIP_BUTTON:
    p = guip_make_button(p);
    child = stack[stacktop];
    break;
  case GUIP_SPINBUTTON:
    p = guip_make_spinbutton(p);
    child = stack[stacktop];
    break;
  case GUIP_DRAWINGAREA:
    p = guip_make_drawingarea(p);
    child = stack[stacktop];
    break;
  case GUIP_DROPBOX:
    p = guip_make_dropbox(p);
    child = stack[stacktop];
    break;
  case GUIP_CHECKBOX:
    p = guip_make_checkbox(p);
    child = stack[stacktop];
    break;
  case GUIP_CURVE:
    p = guip_make_curve(p);
    child = stack[stacktop];
    break;
  case GUIP_RADIOBOX:
    p = guip_make_radiobox(p);
    child = stack[stacktop];
    break;
  default:
    printf("GUI PARSER: unsupported class %s\n",klass);
  }

  if (hasname && child!=0) {
    nt = (GuiNameTable *) malloc(sizeof(GuiNameTable));
    strcpy(nt->name, name);
    nt->object = child;
    nt->adj    = sa[stacktop];
    nt->ext    = se[stacktop];
    nt->next   = guitable;
    guitable = nt;
  }

  // parse children of child (if any)
  if (*p == '(') {
    ++p;
    while(1) {
      guip_create_recursive(p);
      if (*p == ')')
	break;
      ++p; // skip comma
    }
    ++p; // skip closing parenthesis
  }

  // pop child off stack
  stacktop--;

  return child;
}

char * guip_link_stack_top(char *p) {
  int expand = 0, fill = 0;
  int l=0, r=0, t=0, b=0;
  int gap = 0, hgap = 0, vgap = 0;
  int packend = 0; /* pack_end / pack_start */
  char token[64];
  int hflag = 0, vflag = 0;
  int i;

  GtkWidget *x, *y;
  int        kx;

  //  printf("link called on [%s]\n\n",p);

  if (*p == '{') {
    ++p;
    while(*p != '}') {
      if (*p == ',') ++p;
      p = eat_alpha_token(p, token);      
      if (!strcmp(token,"expand")) {
	expand = 1;
	continue;
      }
      if (!strcmp(token,"fill")) {
	fill = 1;
	continue;
      }
      if (!strcmp(token,"packend")) {
	packend = 1;
	continue;
      }
      if (!strcmp(token,"gap")) {
	p = eat_num_token(p+1, token);
	gap = atoi(token);
	continue;
      }
      if (!strcmp(token,"hgap")) {
	p = eat_num_token(p+1, token);
	hgap = atoi(token);
	continue;
      }
      if (!strcmp(token,"vgap")) {
	p = eat_num_token(p+1, token);
	vgap = atoi(token);
	continue;
      }
      if (!strcmp(token,"l")) {
	p = eat_num_token(p+1, token);
	l = atoi(token);
	continue;
      }
      if (!strcmp(token,"r")) {
	p = eat_num_token(p+1, token);
	r = atoi(token);
	continue;
      }
      if (!strcmp(token,"t")) {
	p = eat_num_token(p+1, token);
	t = atoi(token);
	continue;
      }
      if (!strcmp(token,"b")) {
	p = eat_num_token(p+1, token);
	b = atoi(token);
	continue;
      }
      if (token[0] == 'h') {
	for(i=1;i<strlen(token);i++) {
	  switch(token[i]) {
	  case 'e': hflag |= GTK_EXPAND; break;
	  case 'f': hflag |= GTK_FILL;   break;
	  case 's': hflag |= GTK_SHRINK; break;
	  }
	}
	continue;
      }
      if (token[0] == 'v') {
	for(i=1;i<strlen(token);i++) {
	  switch(token[i]) {
	  case 'e': vflag |= GTK_EXPAND; break;
	  case 'f': vflag |= GTK_FILL;   break;
	  case 's': vflag |= GTK_SHRINK; break;
	  }
	}
	continue;
      }

    }
    ++p;
  }

  x  = stack[stacktop - 1];
  kx = sk[stacktop - 1];
  y  = stack[stacktop];

  switch(kx) {
  case GUIP_HBOX:
  case GUIP_VBOX:
    if (packend)
      gtk_box_pack_end(GTK_BOX(x), y, 
		       expand?TRUE:FALSE, fill?TRUE:FALSE,gap);
    else
      gtk_box_pack_start(GTK_BOX(x), y, 
			 expand?TRUE:FALSE, fill?TRUE:FALSE,gap);
    break;
  case GUIP_FRAME:
  case GUIP_BUTTON:
    gtk_container_add(GTK_CONTAINER(x), y);
    break;
  case GUIP_TABLE:
    if (hflag == 0) hflag = GTK_EXPAND | GTK_FILL;
    if (vflag == 0) vflag = GTK_EXPAND | GTK_FILL;
    if (hgap == 0) hgap = gap;
    if (vgap == 0) vgap = gap;
    if (r <= l) r = l + 1;
    if (b <= t) b = t + 1;
    gtk_table_attach(GTK_TABLE(x), y, l, r, t, b, 
		     hflag, vflag, hgap, vgap);
    break;
  default:
    printf("GUI PARSER: illegal widget parenthood.\n");
  }

  return p;
}

char * guip_make_label(char *p) {
  char token[64], text[2048];  
  GtkWidget *x = 0;
  int align = -1;

  if (*p != '[') {
    printf("GUI PARSER: bad label\n");
    return p;
  }

  ++p;
  text[0] = 0;
  while(*p != ']') {
    if (*p == ',') ++p;
    p = eat_alpha_token(p, token);
    if (!strcmp(token,"left"))   { align = 0; continue; }
    if (!strcmp(token,"center")) { align = 1; continue; }
    if (!strcmp(token,"right"))  { align = 2; continue; }
    if (!strcmp(token,"text")) {
      p=eat_string_token(p+1, text);
      continue;
    }
  }
  ++p;
  
  x = gtk_label_new(text);
  switch(align) {
  case 0: gtk_label_set_justify(GTK_LABEL(x), GTK_JUSTIFY_LEFT); break;
  case 1: gtk_label_set_justify(GTK_LABEL(x), GTK_JUSTIFY_CENTER); break;
  case 2: gtk_label_set_justify(GTK_LABEL(x), GTK_JUSTIFY_RIGHT); break;
  }
  
  ++stacktop;
  stack[stacktop] = x;
  sa[stacktop] = 0;
  se[stacktop] = 0;
  sk[stacktop] = GUIP_LABEL;

  // place it in parent
  if (stacktop>0)
    p=guip_link_stack_top(p);

  // show it, unless it's the main tree root
  if (stacktop>0)
    gtk_widget_show(x);

  return p;
}

char * guip_make_frame(char *p) {
  char token[64], text[2048];  
  GtkWidget *x = 0;
  int border = -1;

  //  printf("make_frame called on [%s]\n\n",p);

  text[0] = 0;
  if (*p == '[') {
    ++p;
    while(*p != ']') {
      if (*p == ',') ++p;
      p = eat_alpha_token(p, token);
      if (!strcmp(token,"border")) {
	p = eat_num_token(p+1, token);
	border = atoi(token);
	continue;
      }
      if (!strcmp(token,"title")) {
	p=eat_string_token(p+1, text);
	continue;
      }
    }
    ++p;
  }

  if (text[0]) {
    x = gtk_frame_new(text);
  } else {
    x = gtk_frame_new(0);
  }

  if (border >= 0)
    gtk_container_set_border_width(GTK_CONTAINER(x), border);
  
  ++stacktop;
  stack[stacktop] = x;
  sa[stacktop] = 0;
  se[stacktop] = 0;
  sk[stacktop] = GUIP_FRAME;

  // place it in parent
  if (stacktop>0)
    p=guip_link_stack_top(p);

  // show it, unless it's the main tree root
  if (stacktop>0)
    gtk_widget_show(x);

  return p;
}

char * guip_make_button(char *p) {
  char token[64], text[2048];  
  GtkWidget *x = 0;
  int border = -1;

  text[0] = 0;
  if (*p == '[') {
    ++p;
    while(*p != ']') {
      if (*p == ',') ++p;
      p = eat_alpha_token(p, token);
      if (!strcmp(token,"border")) {
	p = eat_num_token(p+1, token);
	border = atoi(token);
	continue;
      }
      if (!strcmp(token,"text")) {
	p=eat_string_token(p+1, text);
	continue;
      }
    }
    ++p;
  }

  if (text[0]) {
    x = gtk_button_new_with_label(text);
  } else {
    x = gtk_button_new();
  }

  if (border >= 0)
    gtk_container_set_border_width(GTK_CONTAINER(x), border);
  
  ++stacktop;
  stack[stacktop] = x;
  sa[stacktop] = 0;
  se[stacktop] = 0;
  sk[stacktop] = GUIP_BUTTON;

  // place it in parent
  if (stacktop>0)
    p=guip_link_stack_top(p);

  // show it, unless it's the main tree root
  if (stacktop>0)
    gtk_widget_show(x);

  return p;
}

char * guip_make_checkbox(char *p) {
  char token[64], text[2048];  
  GtkWidget *x = 0;
  int border = -1;

  text[0] = 0;
  if (*p == '[') {
    ++p;
    while(*p != ']') {
      if (*p == ',') ++p;
      p = eat_alpha_token(p, token);
      if (!strcmp(token,"border")) {
	p = eat_num_token(p+1, token);
	border = atoi(token);
	continue;
      }
      if (!strcmp(token,"text")) {
	p=eat_string_token(p+1, text);
	continue;
      }
    }
    ++p;
  }

  if (text[0]) {
    x = gtk_check_button_new_with_label(text);
  } else {
    x = gtk_check_button_new();
  }

  if (border >= 0)
    gtk_container_set_border_width(GTK_CONTAINER(x), border);
  
  ++stacktop;
  stack[stacktop] = x;
  sa[stacktop] = 0;
  se[stacktop] = 0;
  sk[stacktop] = GUIP_CHECKBOX;

  // place it in parent
  if (stacktop>0)
    p=guip_link_stack_top(p);

  // show it, unless it's the main tree root
  if (stacktop>0)
    gtk_widget_show(x);

  return p;
}

char * guip_make_box(char *p, int h) {
  char token[64];
  GtkWidget *x = 0;
  int gap = 0, homog = 0, border = -1;

  if (*p == '[') {
    ++p;
    while(*p != ']') {
      if (*p == ',') ++p;
      p = eat_alpha_token(p, token);
      if (!strcmp(token,"border")) {
	p = eat_num_token(p+1, token);
	border = atoi(token);
	continue;
      }
      if (!strcmp(token,"gap")) {
	p = eat_num_token(p+1, token);
	gap = atoi(token);
	continue;
      }
      if (!strcmp(token,"homog")) {
	homog = 1;
	continue;
      }
    }
    ++p;
  }

  if (h)
    x = gtk_hbox_new(homog?TRUE:FALSE, gap);
  else
    x = gtk_vbox_new(homog?TRUE:FALSE, gap);

  if (border >= 0)
    gtk_container_set_border_width(GTK_CONTAINER(x), border);
  
  ++stacktop;
  stack[stacktop] = x;
  sa[stacktop] = 0;
  se[stacktop] = 0;
  sk[stacktop] = h ? GUIP_HBOX : GUIP_VBOX;

  // place it in parent
  if (stacktop>0)
    p=guip_link_stack_top(p);

  // show it, unless it's the main tree root
  if (stacktop>0)
    gtk_widget_show(x);

  return p;
}


char * guip_make_drawingarea(char *p) {
  char token[64];
  GtkWidget *x = 0;
  int w=-1, h=-1, flags=0;

  flags = GDK_EXPOSURE_MASK;

  if (*p == '[') {
    ++p;
    while(*p != ']') {
      if (*p == ',') ++p;
      p = eat_alpha_token(p, token);
      if (!strcmp(token,"w")) {
	p = eat_num_token(p+1, token);
	w = atoi(token);
	continue;
      }
      if (!strcmp(token,"h")) {
	p = eat_num_token(p+1, token);
	h = atoi(token);
	continue;
      }
      if (!strcmp(token,"events")) {
	while((*p) == '=' || (*p) == '+') {
	  p = eat_alpha_token(p+1, token);
	  if (!strcmp(token, "press")) flags|=GDK_BUTTON_PRESS_MASK; else
	    if (!strcmp(token, "release")) flags|=GDK_BUTTON_RELEASE_MASK; else
	      if (!strcmp(token, "drag1")) flags|=GDK_BUTTON1_MOTION_MASK; else
		if (!strcmp(token, "drag2")) flags|=GDK_BUTTON2_MOTION_MASK; else
		  if (!strcmp(token, "drag3")) flags|=GDK_BUTTON3_MOTION_MASK; else
		    if (!strcmp(token, "move")) flags|=GDK_POINTER_MOTION_MASK; else
		      if (!strcmp(token, "drag")) 
			flags|=GDK_BUTTON1_MOTION_MASK|GDK_BUTTON2_MOTION_MASK|
			  GDK_BUTTON3_MOTION_MASK;
	}
      }
    }
    ++p;
  }

  x = gtk_drawing_area_new();
  gtk_widget_set_events(x, flags);

  if (w>=0 && h>=0)
    gtk_drawing_area_size(GTK_DRAWING_AREA(x), w, h);
  
  ++stacktop;
  stack[stacktop] = x;
  sa[stacktop] = 0;
  se[stacktop] = 0;
  sk[stacktop] = GUIP_DRAWINGAREA;

  // place it in parent
  if (stacktop>0)
    p=guip_link_stack_top(p);

  // show it, unless it's the main tree root
  if (stacktop>0)
    gtk_widget_show(x);

  return p;
}

char * guip_make_spinbutton(char *p) {
  char token[64];
  GtkWidget *x = 0;
  GtkAdjustment *adj;
  float val=0.0,lower=0.0,upper=0.0,si=0.0,pi=0.0,pl=0.0;
  float accel=1.0;
  int   digits=0;

  if (*p != '[') {
    printf("GUI PARSER: bad spinbutton\n");
    return p;
  }

  ++p;
  while(*p != ']') {
    if (*p == ',') ++p;
    p = eat_alpha_token(p, token);
    if (!strcmp(token,"val")) {
      p = eat_fnum_token(p+1, token);
      val = atof(token);
      continue;
    }
    if (!strcmp(token,"min")) {
      p = eat_fnum_token(p+1, token);
      lower = atof(token);
      continue;
    }
    if (!strcmp(token,"max")) {
      p = eat_fnum_token(p+1, token);
      upper = atof(token);
      continue;
    }
    if (!strcmp(token,"step")) {
      p = eat_fnum_token(p+1, token);
      si = atof(token);
      continue;
    }
    if (!strcmp(token,"page")) {
      p = eat_fnum_token(p+1, token);
      pi = atof(token);
      continue;
    }
    if (!strcmp(token,"pagelen")) {
      p = eat_fnum_token(p+1, token);
      pl = atof(token);
      continue;
    }
    if (!strcmp(token,"accel")) {
      p = eat_fnum_token(p+1, token);
      accel = atof(token);
      continue;
    }
    if (!strcmp(token,"digits")) {
      p = eat_num_token(p+1, token);
      digits = atoi(token);
      continue;
    }
  }
  ++p;

  adj = (GtkAdjustment *) gtk_adjustment_new(val,lower,upper,si,pi,pl);
  x = gtk_spin_button_new(adj, accel, digits);

  ++stacktop;
  stack[stacktop] = x;
  sa[stacktop] = adj;
  se[stacktop] = 0;
  sk[stacktop] = GUIP_SPINBUTTON;

  // place it in parent
  if (stacktop>0)
    p=guip_link_stack_top(p);

  // show it, unless it's the main tree root
  if (stacktop>0)
    gtk_widget_show(x);

  return p;
}

char * guip_make_curve(char *p) {
  RCurve *r;
  GtkWidget *x;
  
  r = rcurve_new();
  x = r->widget;
  
  ++stacktop;
  stack[stacktop] = x;
  sa[stacktop] = 0;
  se[stacktop] = (void *) r;
  sk[stacktop] = GUIP_CURVE;

  // place it in parent
  if (stacktop>0)
    p=guip_link_stack_top(p);

  return p;
}

char * guip_make_radiobox(char *p) {
  char token[64], text[2048], title[512];
  GtkWidget *x = 0;
  RadioBox *y = 0;

  if (*p != '[') {
    printf("GUI PARSER: bad radiobox\n");
    return p;
  }

  ++p;
  text[0] = 0;
  while(*p != ']') {
    if (*p == ',') ++p;
    p = eat_alpha_token(p, token);
    if (!strcmp(token,"options")) {
      p=eat_string_token(p+1, text);
      continue;
    }
    if (!strcmp(token,"title")) {
      p=eat_string_token(p+1, title);
      continue;
    }
  }
  ++p;
  
  y = radiobox_new(title,text);
  x = y->widget;
  
  ++stacktop;
  stack[stacktop] = x;
  sa[stacktop] = 0;
  se[stacktop] = (void *) y;
  sk[stacktop] = GUIP_RADIOBOX;

  // place it in parent
  if (stacktop>0)
    p=guip_link_stack_top(p);

  // show it, unless it's the main tree root
  if (stacktop>0)
    gtk_widget_show(x);

  return p;
}

char * guip_make_dropbox(char *p) {
  char token[64], text[2048];  
  GtkWidget *x = 0;
  DropBox *y = 0;

  if (*p != '[') {
    printf("GUI PARSER: bad dropbox\n");
    return p;
  }

  ++p;
  text[0] = 0;
  while(*p != ']') {
    if (*p == ',') ++p;
    p = eat_alpha_token(p, token);
    if (!strcmp(token,"options")) {
      p=eat_string_token(p+1, text);
      continue;
    }
  }
  ++p;
  
  y = dropbox_new(text);
  x = y->widget;
  
  ++stacktop;
  stack[stacktop] = x;
  sa[stacktop] = 0;
  se[stacktop] = (void *) y;
  sk[stacktop] = GUIP_DROPBOX;

  // place it in parent
  if (stacktop>0)
    p=guip_link_stack_top(p);

  // show it, unless it's the main tree root
  if (stacktop>0)
    gtk_widget_show(x);

  return p;
}

char * guip_make_table(char *p) {
  char token[64];
  GtkWidget *x = 0;
  int border = -1, w=-1, h=-1, homog=0;

  if (*p != '[') {
    printf("GUI PARSER: bad table\n");
    return p;
  }

  ++p;
  while(*p != ']') {
    if (*p == ',') ++p;
    p = eat_alpha_token(p, token);
    if (!strcmp(token,"border")) {
      p = eat_num_token(p+1, token);
      border = atoi(token);
      continue;
    }
    if (!strcmp(token,"w")) {
      p = eat_num_token(p+1, token);
      w = atoi(token);
      continue;
    }
    if (!strcmp(token,"h")) {
      p = eat_num_token(p+1, token);
      h = atoi(token);
      continue;
    }
    if (!strcmp(token,"homog")) {
      homog = 1;
      continue;
    }
  }
  ++p;

  if (w<0 || h<0) {
    printf("GUI PARSER: missing arguments.\n");
    exit(99);
  }

  x = gtk_table_new(h,w,homog?TRUE:FALSE);

  if (border >= 0)
    gtk_container_set_border_width(GTK_CONTAINER(x), border);
  
  ++stacktop;
  stack[stacktop] = x;
  sa[stacktop] = 0;
  se[stacktop] = 0;
  sk[stacktop] = GUIP_TABLE;

  // place it in parent
  if (stacktop>0)
    p=guip_link_stack_top(p);

  // show it, unless it's the main tree root
  if (stacktop>0)
    gtk_widget_show(x);

  return p;
}

GtkWidget * guip_get(const char *name) {
  GuiNameTable *x;  
  for(x=guitable;x!=0;x=x->next)
    if (!strcmp(x->name,name))
      return(x->object);
  printf("search failed: [%s]\n",name);
  return 0;
}

GtkAdjustment * guip_get_adj(const char *name) {
  GuiNameTable *x;  
  for(x=guitable;x!=0;x=x->next)
    if (!strcmp(x->name,name))
      return(x->adj);
  printf("search failed: [%s]\n",name);
  return 0;
}

void * guip_get_ext(const char *name) {
  GuiNameTable *x;  
  for(x=guitable;x!=0;x=x->next)
    if (!strcmp(x->name,name))
      return(x->ext);
  printf("search failed: [%s]\n",name);
  return 0;
}

/* generic helpers */

void clear_name_table() {
  if (guitable!=0)
    clear_name_node(guitable);
  guitable = 0;
}

void clear_name_node(GuiNameTable *node) {
  if (node->next != 0)
    clear_name_node(node->next);
  free(node);    
}

/* lexical helpers */

char * eat_alpha_token(char *ptr, char *dest) {
  char c, *d;

  d = dest;
  c = *ptr;
  while(( c>='A' && c<='Z' ) || (c>='a' && c<='z') || (c>='0' && c<='9') ) {
    *d = c;
    ++d; ++ptr;
    c = *ptr;
    if (!c) break;
  }
  *d = 0;

  return ptr;
}

char * eat_num_token(char *ptr, char *dest) {
  char c, *d;

  d = dest;
  c = *ptr;
  while( c>='0' && c<='9' ) {
    *d = c;
    ++d; ++ptr;
    c = *ptr;
    if (!c) break;
  }
  *d = 0;

  return ptr;
}

char * eat_fnum_token(char *ptr, char *dest) {
  char c, *d;

  d = dest;
  c = *ptr;
  while(( c>='0' && c<='9' ) || (c=='-') || (c=='.') ) {
    *d = c;
    ++d; ++ptr;
    c = *ptr;
    if (!c) break;
  }
  *d = 0;

  return ptr;
}

char * eat_string_token(char *ptr, char *dest) {
  char *d;

  if (*ptr != '\'')
    return(eat_alpha_token(ptr, dest));
  
  d = dest;
  ++ptr;

  while( (*ptr) != '\'' ) {
    *d = *ptr;
    ++d; ++ptr;
    if (! (*ptr) ) break;
  }
  *d = 0;
  if ((*ptr) == '\'') ++ptr;

  return ptr;
}

int klass_of(char *name) {
  int i;
  for(i=0;i<NCLASSES;i++) {
    if (!strcmp(name, klass_names[i]))
      return i;
  }
  return -1;
}
