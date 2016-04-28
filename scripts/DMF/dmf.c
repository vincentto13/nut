//TODO: not in final
#include <malamute.h>
#include <neon/ne_xml.h>
#include "snmp-ups.h"
#include "powerware-mib.c"
/*
 *      HEADER FILE
 *
 */
#define YES "yes"
#define DEFAULT_CAPACITY 16

#define MIB2NUT "mib2nut"
#define LOOKUP "lookup"
#define SNMP "snmp"
#define ALARM "alarm"

//#define MIB2NUT_NAME "name"
#define MIB2NUT_VERSION "version"
#define MIB2NUT_OID "oid"
#define MIB2NUT_MIB_NAME "mib_name"
#define MIB2NUT_AUTO_CHECK "auto_check"
#define MIB2NUT_POWER_STATUS "power_status"
#define MIB2NUT_SNMP "snmp_info"
#define MIB2NUT_ALARMS "alarms_info"

#define INFO_MIB2NUT_MAX_ATTRS 14
#define INFO_LOOKUP_MAX_ATTRS 4
#define INFO_SNMP_MAX_ATTRS 14
#define INFO_ALARM_MAX_ATTRS 6

#define INFO_LOOKUP "lookup_info"
#define LOOKUP_OID "oid"
#define LOOKUP_VALUE "value"

#define INFO_SNMP "snmp_info"
#define SNMP_NAME "name"
#define SNMP_MULTIPLIER "multiplier"
#define SNMP_OID "oid"
#define SNMP_DEFAULT "default"
#define SNMP_LOOKUP "lookup"
#define SNMP_SETVAR "setvar"
//Info_flags
#define SNMP_INFOFLAG_WRITABLE "writable"
#define SNMP_INFOFLAG_STRING "string"
//Flags
#define SNMP_FLAG_STATIC "static"
#define SNMP_FLAG_ABSENT "absent"
#define SNMP_FLAG_NEGINVALID "positive"
#define SNMP_FLAG_UNIQUE "unique"
#define SNMP_STATUS_PWR "power_status"
#define SNMP_STATUS_BATT "battery_status"
#define SNMP_STATUS_CAL "calibration"
#define SNMP_STATUS_RB "replace_baterry"
#define SNMP_TYPE_CMD "command"
#define SNMP_OUTLET_GROUP "outlet_group"
#define SNMP_OUTLET "outlet"
#define SNMP_OUTPUT_1 "output_1_phase"
#define SNMP_OUTPUT_3 "output_3_phase"
#define SNMP_INPUT_1 "input_1_phase"
#define SNMP_INPUT_3 "input_3_phase"
#define SNMP_BYPASS_1 "bypass_1_phase"
#define SNMP_BYPASS_3 "bypass_3_phase"
//Setvar
#define SETVAR_INPUT_PHASES "input_phases"
#define SETVAR_OUTPUT_PHASES "output_phases"
#define SETVAR_BYPASS_PHASES "bypass_phases"

#define INFO_ALARM "info_alarm"
#define ALARM_OID "oid"
#define ALARM_STATUS "status"
#define ALARM_ALARM "alarm"



typedef struct {
	void **values;
	int size;
	int capacity;
	char *name;
	void (*destroy)(void **self_p);
	void (*new_element)(void);
} alist_t;

typedef enum {
    ERR = -1,
    OK
} state_t;

// Create and initialize info_lkp_t
info_lkp_t *
    info_lkp_new (int oid, const char *value);

// Destroy and NULLify the reference to alist_t, list of collections
void
    info_lkp_destroy (void **self_p);

// Create new instance of alist with LOOKUP type, for storage a list of collections
alist_t *
    alist_new ();
    
unsigned long 
    compile_flags(const char **attrs);

int
    compile_info_flags(const char **attrs);

/*
 *
 *  C FILE
 *
 */
//DEBUGGING
void print_snmp_memory_struct(snmp_info_t *self)
{
  int i = 0;
  printf("SNMP: --> Info_type: %s //   Info_len: %f //   OID:  %s //   Default: %s",self->info_type, self->info_len, self->OID, self->dfl);
  if(self->setvar)
    printf("//   Setvar: %d", *self->setvar);
  printf("\n");
  
  if (self->oid2info)
	{
	    while(!((self->oid2info[i].oid_value == 0) && (!self->oid2info[i].info_value))){
	      printf("Info_lkp_t-----------> %d",self->oid2info[i].oid_value);
	      if(self->oid2info[i].info_value)
		printf("  value---> %s\n",self->oid2info[i].info_value);
	      i++;
  }    
  printf("*-*-*-->Info_flags %d\n", self->info_flags);
  printf("*-*-*-->Flags %lu\n", self->flags);
  }
}

void print_alarm_memory_struct(alarms_info_t *self)
{
  printf("Alarm: -->  OID: %s //   Status: %s //   Value: %s\n",self->OID, self->status_value, self->alarm_value);
}
void print_mib2nut_memory_struct(mib2nut_info_t *self){
  int i = 0;
  printf("\n");
  printf("       MIB2NUT: --> Mib_name: %s //   Version: %s //   Power_status: %s //   Auto_check: %s //   SysOID: %s\n",self->mib_name, self->mib_version, self->oid_pwr_status, self->oid_auto_check, self->sysOID);
  
  if (self->snmp_info)
	{
	    while(!((!self->snmp_info[i].info_type) && (self->snmp_info[i].info_len == 0) && (!self->snmp_info[i].OID) && (!self->snmp_info[i].dfl) && (self->snmp_info[i].flags == 0) && (!self->snmp_info[i].oid2info))){
	      print_snmp_memory_struct(self->snmp_info+i);
	      i++;
        }
  }
  i = 0;
  if (self->alarms_info)
	{
	    while((self->alarms_info[i].alarm_value) || (self->alarms_info[i].OID) || (self->alarms_info[i].status_value)){
	      print_alarm_memory_struct(self->alarms_info+i);
	      i++;
        }
  }
}
//END DEBUGGING

char
*get_param_by_name (const char *name, const char **items)
{
    int iname;

    if (!items || !name) return NULL;
    iname = 0;
    while (items[iname]) {
        if (strcmp (items[iname],name) == 0) {
            return strdup(items[iname+1]);
        }
        iname += 2;
    }
    return NULL;
}

//Create a lookup elemet
info_lkp_t *
info_lkp_new (int oid, const char *value)
{
    info_lkp_t *self = (info_lkp_t*) malloc (sizeof (info_lkp_t));
    assert (self);
    memset (self, 0, sizeof (info_lkp_t));
    self->oid_value = oid;
    if(value)
      self->info_value = strdup (value);
    return self;
}

//Create alarm element
alarms_info_t *
info_alarm_new (const char *oid, const char *status, const char *alarm)
{
    alarms_info_t *self = (alarms_info_t*) malloc (sizeof (alarms_info_t));
    assert (self);
    memset (self, 0, sizeof (alarms_info_t));
    if(oid)
      self->OID = strdup (oid);
    if(status)
      self->status_value = strdup (status);
    if(alarm)
      self->alarm_value = strdup (alarm);
    return self;
}
snmp_info_t *
info_snmp_new (const char *name, int info_flags, double multiplier, const char *oid, const char *dfl, unsigned long flags, info_lkp_t *lookup, int *setvar)
{
    snmp_info_t *self = (snmp_info_t*) malloc (sizeof (snmp_info_t));
    assert (self);
    memset (self, 0, sizeof (snmp_info_t));
    if(name)
      self->info_type = strdup (name);
    self->info_len = multiplier;
    if(oid)
      self->OID = strdup (oid);
    if(dfl)
      self->dfl = strdup (dfl);
    self->info_flags = info_flags;
    self->flags = flags;
    self->oid2info = lookup;
    self->setvar = setvar;
    return self;
}
mib2nut_info_t *
info_mib2nut_new (const char *name, const char *version, const char *oid_power_status, const char *oid_auto_check, snmp_info_t *snmp, const char *sysOID, alarms_info_t *alarms)
{
    mib2nut_info_t *self = (mib2nut_info_t*) malloc (sizeof (mib2nut_info_t));
    assert (self);
    memset (self, 0, sizeof (mib2nut_info_t));
    if(name)
      self->mib_name = strdup (name);
    if(version)
      self->mib_version = strdup (version);
    if(oid_power_status)
      self->oid_pwr_status = strdup (oid_power_status);
    if(oid_auto_check)
      self->oid_auto_check = strdup (oid_auto_check);
    if(sysOID)
      self->sysOID = strdup (sysOID);
    self->snmp_info = snmp;
    self->alarms_info = alarms;
    return self;
}
//Destroy full array of lookup elements
void
info_lkp_destroy (void **self_p)
{
    if (*self_p) {
        info_lkp_t *self = (info_lkp_t*) *self_p;
        if (self->info_value)
	{
            free ((char*)self->info_value);
            self->info_value = NULL;
        }
        free (self);
	*self_p = NULL;
    }
}

//Destroy full array of alarm elements
void
info_alarm_destroy (void **self_p)
{
    if (*self_p) {
        alarms_info_t *self = (alarms_info_t*) *self_p;
        if (self->OID)
	{
            free ((char*)self->OID);
            self->OID = NULL;
        }
        if (self->status_value)
	{
            free ((char*)self->status_value);
            self->status_value = NULL;
        }
        if (self->alarm_value)
	{
            free ((char*)self->alarm_value);
            self->alarm_value = NULL;
        }
        free (self);
	*self_p = NULL;
    }
}

void
info_snmp_destroy (void **self_p)
{
    int i = 0;
    if (*self_p) {
        snmp_info_t *self = (snmp_info_t*) *self_p;
        if (self->info_type)
	{
            free ((char*)self->info_type);
            self->info_type = NULL;
        }
        if (self->OID)
	{
            free ((char*)self->OID);
            self->OID = NULL;
        }
        if (self->dfl)
	{
            free ((char*)self->dfl);
            self->dfl = NULL;
        }
        if (self->oid2info)
	{
	    while(!((self->oid2info[i].oid_value == 0) && (!self->oid2info[i].info_value))){
	      if(self->oid2info[i].info_value){
		free((void*)self->oid2info[i].info_value);
		self->oid2info[i].info_value = NULL;
	      }
	      i++;
	    }
            free ((info_lkp_t*)self->oid2info);
            self->oid2info = NULL;
        }
        free (self);
	*self_p = NULL;
    }
}

void
info_mib2nut_destroy (void **self_p)
{
    int i = 0;
    //int j = 0;
    if (*self_p) {
        mib2nut_info_t *self = (mib2nut_info_t*) *self_p;
        if (self->mib_name)
	{
            free ((char*)self->mib_name);
            self->mib_name = NULL;
        }
        if (self->mib_version)
	{
            free ((char*)self->mib_version);
            self->mib_version = NULL;
        }
        if (self->oid_pwr_status)
	{
            free ((char*)self->oid_pwr_status);
            self->oid_pwr_status = NULL;
        }
        if (self->oid_auto_check)
	{
            free ((char*)self->oid_auto_check);
            self->oid_auto_check = NULL;
        }
        if (self->sysOID)
	{
            free ((char*)self->sysOID);
            self->sysOID = NULL;
        }
        if (self->snmp_info)
	{
	    while(!((!self->snmp_info[i].info_type) && (self->snmp_info[i].info_len == 0) && (!self->snmp_info[i].OID) && (!self->snmp_info[i].dfl) && (self->snmp_info[i].flags == 0) && (!self->snmp_info[i].oid2info))){
	      if(self->snmp_info[i].info_type){
		free((void*)self->snmp_info[i].info_type);
		self->snmp_info[i].info_type = NULL;
	      }
	      if(self->snmp_info[i].OID){
		free((void*)self->snmp_info[i].OID);
		self->snmp_info[i].OID = NULL;
	      }
	      if(self->snmp_info[i].dfl){
		free((void*)self->snmp_info[i].dfl);
		self->snmp_info[i].dfl = NULL;
	      }
	      i++;
	    }
            free ((snmp_info_t*)self->snmp_info);
            self->snmp_info = NULL;
        }
        i = 0;
        if (self->alarms_info)
	{
	    while((self->alarms_info[i].alarm_value) || (self->alarms_info[i].OID) || (self->alarms_info[i].status_value)){
	      if(self->alarms_info[i].alarm_value){
		free((void*)self->alarms_info[i].alarm_value);
		self->alarms_info[i].alarm_value = NULL;
	      }
	      if(self->alarms_info[i].OID){
		free((void*)self->alarms_info[i].OID);
		self->alarms_info[i].OID = NULL;
	      }
	      if(self->alarms_info[i].status_value){
		free((void*)self->alarms_info[i].status_value);
		self->alarms_info[i].status_value = NULL;
	      }
	      i++;
	    }
            free ((alarms_info_t*)self->alarms_info);
            self->alarms_info = NULL;
        }
        free (self);
	*self_p = NULL;
    }
}
//New generic list element (can be the root element)
alist_t *alist_new (const char *name, void (*destroy)(void **self_p), void (*new_element)(void))
{
  alist_t *self = (alist_t*) malloc (sizeof (alist_t));
  assert (self);
  memset (self, 0, sizeof(alist_t));
  self->size = 0;
  self->capacity = DEFAULT_CAPACITY;
  self->values = (void**) malloc (self->capacity * sizeof (void*));
  assert (self->values);
  memset (self->values, 0, self->capacity);
  self->destroy = destroy;
  self->new_element = new_element;
  if(name)
    self->name = strdup(name);
  else 
    self->name = NULL;
  return self;
}

//Destroy full array of generic list elements
void
alist_destroy (alist_t **self_p)
{
    if (*self_p)
    {
        alist_t *self = *self_p;
	
        for (;self->size>0; self->size--){
	  self->destroy(& self->values [self->size-1]);
	}
	if(self->name)
	  free(self->name);
        free (self->values);
        free (self);
	*self_p = NULL;
    }
}

//Add a generic element at the end of the list
void alist_append(alist_t *self,void *element)
{
  if(self->size==self->capacity)
  {
    self->capacity+=DEFAULT_CAPACITY;
    self->values = (void**) realloc(self->values, self->capacity * sizeof(void*));
  }
    self->values[self->size] = element;
    self->size++;
}

//Return the last element of the list
alist_t *alist_get_last_element(alist_t *self)
{
  if(self)
    return (alist_t*)self->values[self->size-1];
  return NULL;
}

alist_t *alist_get_element_by_name(alist_t *self, char *name)
{
  int i;
  if(self)
    for(i = 0; i < self->size; i++)
      if(strcmp(((alist_t*)self->values[i])->name, name) == 0)
	return (alist_t*)self->values[i];
  return NULL;
}
//I splited because with the error control is going a grow a lot
void
mib2nut_info_node_handler(alist_t *list, const char **attrs){
    alist_t *element = alist_get_last_element(list);
    int i=0;
    snmp_info_t *snmp = NULL;
    alarms_info_t *alarm = NULL;
    
    char **arg = (char**) malloc ((INFO_MIB2NUT_MAX_ATTRS + 1) * sizeof (void**));
    assert (arg);
    memset (arg, 0, (INFO_MIB2NUT_MAX_ATTRS + 1) * sizeof(void**));
    
    arg[0] = get_param_by_name(MIB2NUT_MIB_NAME, attrs);
    arg[1] = get_param_by_name(MIB2NUT_VERSION, attrs);
    arg[2] = get_param_by_name(MIB2NUT_OID, attrs);
    arg[3] = get_param_by_name(MIB2NUT_POWER_STATUS, attrs);
    arg[4] = get_param_by_name(MIB2NUT_AUTO_CHECK, attrs);
    arg[5] = get_param_by_name(MIB2NUT_SNMP, attrs);
    arg[6] = get_param_by_name(MIB2NUT_ALARMS, attrs);
    
    if(arg[5]){
      alist_t *lkp = alist_get_element_by_name(list, arg[5]);
      snmp = (snmp_info_t*) malloc((lkp->size + 1) * sizeof(snmp_info_t));
      for(i = 0; i < lkp->size; i++){
	snmp[i].info_flags = ((snmp_info_t*) lkp->values[i])->info_flags;
	snmp[i].info_len = ((snmp_info_t*) lkp->values[i])->info_len;
	snmp[i].flags = ((snmp_info_t*) lkp->values[i])->flags;
	if(((snmp_info_t*) lkp->values[i])->info_type)
	  snmp[i].info_type = strdup(((snmp_info_t*) lkp->values[i])->info_type);
	else snmp[i].info_type = NULL;
	if(((snmp_info_t*) lkp->values[i])->OID)
	  snmp[i].OID = strdup(((snmp_info_t*) lkp->values[i])->OID);
	else snmp[i].OID = NULL;
	if(((snmp_info_t*) lkp->values[i])->dfl)
	  snmp[i].dfl = strdup(((snmp_info_t*) lkp->values[i])->dfl);
	else snmp[i].dfl = NULL;
	if(((snmp_info_t*) lkp->values[i])->setvar)
	  snmp[i].setvar = ((snmp_info_t*) lkp->values[i])->setvar;
	else snmp[i].setvar = NULL;
	if(((snmp_info_t*) lkp->values[i])->oid2info)
	  snmp[i].oid2info = ((snmp_info_t*) lkp->values[i])->oid2info;
	else snmp[i].oid2info = NULL;
      }
      snmp[i].info_flags = 0;
      snmp[i].info_type = NULL;
      snmp[i].info_len = 0;
      snmp[i].OID = NULL;
      snmp[i].flags = 0;
      snmp[i].dfl = NULL;
      snmp[i].setvar = NULL;
      snmp[i].oid2info = NULL;
    }
    if(arg[6]){
      alist_t *alm = alist_get_element_by_name(list, arg[6]);
      alarm = (alarms_info_t*) malloc((alm->size + 1) * sizeof(alarms_info_t));
      for(i = 0; i < alm->size; i++){
	if(((alarms_info_t*) alm->values[i])->OID)
	  alarm[i].OID = strdup(((alarms_info_t*) alm->values[i])->OID);
	else alarm[i].OID = NULL;
	if(((alarms_info_t*) alm->values[i])->status_value)
	  alarm[i].status_value = strdup(((alarms_info_t*) alm->values[i])->status_value);
	else alarm[i].status_value = NULL;
	if(((alarms_info_t*) alm->values[i])->alarm_value)
	  alarm[i].alarm_value = strdup(((alarms_info_t*) alm->values[i])->alarm_value);
	else alarm[i].alarm_value = NULL;
      }
      alarm[i].OID = NULL;
      alarm[i].status_value = NULL;
      alarm[i].alarm_value = NULL;
    }
    if(arg[0])
	  alist_append(element, ((mib2nut_info_t *(*) (const char *, const char *, const char *, const char *, snmp_info_t *, const char *, alarms_info_t *)) element->new_element) (arg[0], arg[1], arg[3], arg[4], snmp, arg[2], alarm));
    
    for(i = 0; i < (INFO_ALARM_MAX_ATTRS + 1); i++)
      free (arg[i]);
    
    free (arg);
}

void
alarm_info_node_handler(alist_t *list, const char **attrs)
{
    alist_t *element = alist_get_last_element(list);
    int i=0;
    char **arg = (char**) malloc ((INFO_ALARM_MAX_ATTRS + 1) * sizeof (void**));
    assert (arg);
    memset (arg, 0, (INFO_ALARM_MAX_ATTRS + 1) * sizeof(void**));
    
    arg[0] = get_param_by_name(ALARM_ALARM, attrs);
    arg[1] = get_param_by_name(ALARM_STATUS, attrs);
    arg[2] = get_param_by_name(ALARM_OID, attrs);
    
    if(arg[0])
	  alist_append(element, ((alarms_info_t *(*) (const char *, const char *, const char *)) element->new_element) (arg[0], arg[1], arg[2]));
    
    for(i = 0; i < (INFO_ALARM_MAX_ATTRS + 1); i++)
      free (arg[i]);
    
    free (arg);
}

void
lookup_info_node_handler(alist_t *list, const char **attrs)
{
    alist_t *element = alist_get_last_element(list);
    int i=0;
    char **arg = (char**) malloc ((INFO_LOOKUP_MAX_ATTRS + 1) * sizeof (void**));
    assert (arg);
    memset (arg, 0, (INFO_LOOKUP_MAX_ATTRS + 1) * sizeof(void**));
    
    arg[0] = get_param_by_name(LOOKUP_OID, attrs);
    arg[1] = get_param_by_name(LOOKUP_VALUE, attrs);

    if(arg[0])
	alist_append(element, ((info_lkp_t *(*) (int, const char *)) element->new_element) (atoi(arg[0]), arg[1]));
    
    for(i = 0; i < (INFO_LOOKUP_MAX_ATTRS + 1); i++)
      free (arg[i]);
    
    free (arg);
}

void
snmp_info_node_handler(alist_t *list, const char **attrs)
{
    //temporal
    double multiplier = 128;
    //end tremporal
    unsigned long flags;
    int info_flags;
    info_lkp_t *lookup = NULL;
    alist_t *element = alist_get_last_element(list);
    int i=0;
    char **arg = (char**) malloc ((INFO_SNMP_MAX_ATTRS + 1) * sizeof (void**));
    assert (arg);
    memset (arg, 0, (INFO_SNMP_MAX_ATTRS + 1) * sizeof(void**));
    
    arg[0] = get_param_by_name(SNMP_NAME, attrs);
    arg[1] = get_param_by_name(SNMP_MULTIPLIER, attrs);
    arg[2] = get_param_by_name(SNMP_OID, attrs);
    arg[3] = get_param_by_name(SNMP_DEFAULT, attrs);
    arg[4] = get_param_by_name(SNMP_LOOKUP, attrs);
    arg[5] = get_param_by_name(SNMP_OID, attrs);
    arg[6] = get_param_by_name(SNMP_SETVAR, attrs);
    
    //Info_flags
    info_flags = compile_info_flags(attrs);
    //Flags
    flags = compile_flags(attrs);
    
    if(arg[4]){
      alist_t *lkp = alist_get_element_by_name(list, arg[4]);
      lookup = (info_lkp_t*) malloc((lkp->size + 1) * sizeof(info_lkp_t));
      for(i = 0; i < lkp->size; i++){
	lookup[i].oid_value = ((info_lkp_t*) lkp->values[i])->oid_value;
	if(((info_lkp_t*) lkp->values[i])->info_value)
	  lookup[i].info_value = strdup(((info_lkp_t*) lkp->values[i])->info_value);
	else lookup[i].info_value = NULL;
      }
      lookup[i].oid_value = 0;
      lookup[i].info_value = NULL;
    }
    if(arg[1])
      multiplier = atof(arg[1]);
    if(arg[6]){
      if(strcmp(arg[6], SETVAR_INPUT_PHASES) == 0)
        alist_append(element, ((snmp_info_t *(*) (const char *, int, double, const char *, const char *, unsigned long, info_lkp_t *, int *)) element->new_element) (arg[0], info_flags, multiplier, arg[2], arg[3], flags, lookup, &input_phases));
      else if(strcmp(arg[6], SETVAR_OUTPUT_PHASES) == 0)
        alist_append(element, ((snmp_info_t *(*) (const char *, int, double, const char *, const char *, unsigned long, info_lkp_t *, int *)) element->new_element) (arg[0], info_flags, multiplier, arg[2], arg[3], flags, lookup, &output_phases));
      else if(strcmp(arg[6], SETVAR_BYPASS_PHASES) == 0)
        alist_append(element, ((snmp_info_t *(*) (const char *, int, double, const char *, const char *, unsigned long, info_lkp_t *, int *)) element->new_element) (arg[0], info_flags, multiplier, arg[2], arg[3], flags, lookup, &bypass_phases));
    }else
      alist_append(element, ((snmp_info_t *(*) (const char *, int, double, const char *, const char *, unsigned long, info_lkp_t *, int *)) element->new_element) (arg[0], info_flags, multiplier, arg[2], arg[3], flags, lookup, NULL));
    
    for(i = 0; i < (INFO_SNMP_MAX_ATTRS + 1); i++)
      free (arg[i]);
    
    free (arg);
}

unsigned long
compile_flags(const char **attrs)
{
  int i = 0;
  unsigned long flags = 0;
  char *aux_flags = NULL;
  aux_flags = get_param_by_name(SNMP_FLAG_STATIC, attrs);
    if(aux_flags)if(strcmp(aux_flags, YES) == 0){
      flags = flags | SU_FLAG_STATIC;
      i++;
    }
  if(aux_flags)free(aux_flags);
  aux_flags = get_param_by_name(SNMP_FLAG_ABSENT, attrs);
  if(aux_flags)if(strcmp(aux_flags, YES) == 0){
      flags = flags | SU_FLAG_ABSENT;
      i++;
    }
  if(aux_flags)free(aux_flags);
  aux_flags = get_param_by_name(SNMP_FLAG_NEGINVALID, attrs);
    if(aux_flags)if(strcmp(aux_flags, YES) == 0){
      flags = flags | SU_FLAG_NEGINVALID;
      i++;
    }
  if(aux_flags)free(aux_flags);
  aux_flags = get_param_by_name(SNMP_FLAG_UNIQUE, attrs);
  if(aux_flags)if(strcmp(aux_flags, YES) == 0){
      flags = flags | SU_FLAG_UNIQUE;
      i++;
    }
  if(aux_flags)free(aux_flags);
  aux_flags = get_param_by_name(SNMP_STATUS_PWR, attrs);
    if(aux_flags)if(strcmp(aux_flags, YES) == 0){
      flags = flags | SU_STATUS_PWR;
      i++;
    }
  if(aux_flags)free(aux_flags);
  aux_flags = get_param_by_name(SNMP_STATUS_BATT, attrs);
  if(aux_flags)if(strcmp(aux_flags, YES) == 0){
      flags = flags | SU_STATUS_BATT;
      i++;
    }
  if(aux_flags)free(aux_flags);
    aux_flags = get_param_by_name(SNMP_STATUS_CAL, attrs);
    if(aux_flags)if(strcmp(aux_flags, YES) == 0){
      flags = flags | SU_STATUS_CAL;
      i++;
    }
  if(aux_flags)free(aux_flags);
  aux_flags = get_param_by_name(SNMP_STATUS_RB, attrs);
  if(aux_flags)if(strcmp(aux_flags, YES) == 0){
      flags = flags | SU_STATUS_RB;
      i++;
    }
  if(aux_flags)free(aux_flags);
  aux_flags = get_param_by_name(SNMP_TYPE_CMD, attrs);
    if(aux_flags)if(strcmp(aux_flags, YES) == 0){
      flags = flags | SU_TYPE_CMD;
      i++;
    }
  if(aux_flags)free(aux_flags);
  aux_flags = get_param_by_name(SNMP_OUTLET_GROUP, attrs);
  if(aux_flags)if(strcmp(aux_flags, YES) == 0){
      flags = flags | SU_OUTLET_GROUP;
      i++;
    }
  if(aux_flags)free(aux_flags);
  aux_flags = get_param_by_name(SNMP_OUTLET, attrs);
    if(aux_flags)if(strcmp(aux_flags, YES) == 0){
      flags = flags | SU_OUTLET;
      i++;
    }
  if(aux_flags)free(aux_flags);
  aux_flags = get_param_by_name(SNMP_OUTPUT_1, attrs);
  if(aux_flags)if(strcmp(aux_flags, YES) == 0){
      flags = flags | SU_OUTPUT_1;
      i++;
    }
  if(aux_flags)free(aux_flags);
   aux_flags = get_param_by_name(SNMP_OUTPUT_3, attrs);
  if(aux_flags)if(strcmp(aux_flags, YES) == 0){
      flags = flags | SU_OUTPUT_3;
      i++;
    }
  if(aux_flags)free(aux_flags);
  aux_flags = get_param_by_name(SNMP_INPUT_1, attrs);
  if(aux_flags)if(strcmp(aux_flags, YES) == 0){
      flags = flags | SU_INPUT_1;
      i++;
    }
  if(aux_flags)free(aux_flags);
  aux_flags = get_param_by_name(SNMP_INPUT_3, attrs);
    if(aux_flags)if(strcmp(aux_flags, YES) == 0){
      flags = flags | SU_INPUT_3;
      i++;
    }
  if(aux_flags)free(aux_flags);
  aux_flags = get_param_by_name(SNMP_BYPASS_1, attrs);
  if(aux_flags)if(strcmp(aux_flags, YES) == 0){
      flags = flags | SU_BYPASS_1;
      i++;
    }
  if(aux_flags)free(aux_flags);
   aux_flags = get_param_by_name(SNMP_BYPASS_3, attrs);
  if(aux_flags)if(strcmp(aux_flags, YES) == 0){
      flags = flags | SU_BYPASS_3;
      i++;
    }
  if(aux_flags)free(aux_flags);
  return flags;
}
int
compile_info_flags(const char **attrs)
{
  int i = 0;
  int info_flags = 0;
  char *aux_flags = NULL;
  aux_flags = get_param_by_name(SNMP_INFOFLAG_WRITABLE, attrs);
  if(aux_flags)if(strcmp(aux_flags, YES) == 0){
      info_flags = info_flags | ST_FLAG_RW;
      i++;
    }
  if(aux_flags)free(aux_flags);
  aux_flags = get_param_by_name(SNMP_INFOFLAG_STRING, attrs);
  if(aux_flags)if(strcmp(aux_flags, YES) == 0){
      info_flags = info_flags | ST_FLAG_STRING;
      i++;
    }
  if(aux_flags)free(aux_flags);
  
  return info_flags;
}

int xml_dict_start_cb(void *userdata, int parent,
                      const char *nspace, const char *name,
                      const char **attrs)
{
  alist_t *list = (alist_t*) userdata;
  char *auxname = get_param_by_name("name",attrs);
  if(!userdata)return ERR;
  if(strcmp(name,MIB2NUT) == 0)
  {
    alist_append(list, alist_new(auxname, info_mib2nut_destroy,(void (*)(void)) info_mib2nut_new));
    mib2nut_info_node_handler(list,attrs);
  }
  else if(strcmp(name,LOOKUP) == 0)
  {
    alist_append(list, alist_new(auxname, info_lkp_destroy,(void (*)(void)) info_lkp_new));
  }
  else if(strcmp(name,ALARM) == 0)
  {
    alist_append(list, alist_new(auxname, info_alarm_destroy, (void (*)(void)) info_alarm_new));
  }
  else if(strcmp(name,SNMP) == 0)
  {
    alist_append(list, alist_new(auxname, info_snmp_destroy, (void (*)(void)) info_snmp_new));
  }
  else if(strcmp(name,INFO_LOOKUP) == 0)
  {
    lookup_info_node_handler(list,attrs);
  }
  else if(strcmp(name,INFO_ALARM) == 0)
  {
    alarm_info_node_handler(list,attrs);
  }else if(strcmp(name,INFO_SNMP) == 0)
  {
    snmp_info_node_handler(list,attrs);
  }
  free(auxname);
  return 1;
}

int xml_end_cb(void *userdata, int state, const char *nspace, const char *name)
{
  alist_t *list = (alist_t*) userdata;
  alist_t *element = alist_get_last_element(list);
  
  if(!userdata)return ERR;
  if(strcmp(name,MIB2NUT) == 0)
  {
    print_mib2nut_memory_struct((mib2nut_info_t*)element->values[0]);
  }
  return OK;
  
}

int main ()
{
    alist_t * list = alist_new(NULL,(void (*)(void **))alist_destroy, NULL);
    char buffer[1024];
    int result = 0;ne_xml_parser *parser = ne_xml_create ();
    ne_xml_push_handler (parser, xml_dict_start_cb, NULL, xml_end_cb, list);
    FILE *f = fopen ("powerware-mib.dmf", "r");
    if (f) {
        while (!feof (f)) {
            size_t len = fread(buffer, sizeof(char), sizeof(buffer), f);
            if (len == 0) {
                result = 1;
                break;
            } else {
                if ((result = ne_xml_parse (parser, buffer, len))) {
                    break;
                }
            }
        }
        if (!result) ne_xml_parse (parser, buffer, 0);
	/*printf("aqui %s", buffer);*/
        fclose (f);
    } else {
        result = 1;
    }
    ne_xml_destroy (parser);
    
    alist_destroy(&list);
    
    //Debugging
    printf("\n\n");
    printf("Original C structures:\n\n");
    print_mib2nut_memory_struct(&powerware);
    print_mib2nut_memory_struct(&pxgx_ups);
    //End debugging
}