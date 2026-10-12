    #include <stdio.h>
    #include <string.h>
    #include <unistd.h>
    #include <stdlib.h>
    #include <time.h>
    #include <pthread.h>

#define LOG_FILE "csvFiles\\db_log.csv"
#define CC_MAX_TABLES 10
#define INDEX_SIZE 1013
typedef struct IndexItem {
    int key;//pk of the row
    void *ptr;//pointer points at a row in memory
} IndexItem;
IndexItem *dummyIndexItem = NULL;
IndexItem *orgIndex[INDEX_SIZE] = {0};//hash index for organizations each element is a pointer to an IndexItem
IndexItem *roleIndex[INDEX_SIZE] = {0};
IndexItem *userIndex[INDEX_SIZE] = {0};
IndexItem *zoneIndex[INDEX_SIZE] = {0};
IndexItem *gateIndex[INDEX_SIZE] = {0};
IndexItem *vehicleIndex[INDEX_SIZE] = {0};
IndexItem *permissionIndex[INDEX_SIZE] = {0};
IndexItem *accessLogIndex[INDEX_SIZE] = {0};

int gateZoneKey(int zoneID, int gateID) {
    return zoneID * 100000 + gateID;//unique int as long as gate_id<100000
}
IndexItem *gateZoneIndex[INDEX_SIZE] = {0};

// Hash function
//converts a key into an array position to know where to start inside the hash index array
int indexHashCode(int key) {
    if (key < 0) key = -key;//safty
    return key % INDEX_SIZE;//convert the key into an array
}
// Create dummy item once
void initDummyIndexItem() {
    if (dummyIndexItem == NULL) {
    dummyIndexItem = (IndexItem*)malloc(sizeof(IndexItem));
    dummyIndexItem->key = -1;//deletion mark
    dummyIndexItem->ptr = NULL;
}
}
// Generic insert into a hash array
void indexInsert(IndexItem *hashArray[], int key, void *ptr) {
    IndexItem *item = (IndexItem*)malloc(sizeof(IndexItem));
    item->key = key;
    item->ptr = ptr;
    int index = indexHashCode(key);//Gets the first position in the array where we want to insert
    int start = index;
    while (hashArray[index] != NULL && hashArray[index]->key != -1) {
        if (hashArray[index]->key == key) {
        hashArray[index]->ptr = ptr;
    free(item);
        return;
        }
        index = (index + 1) % INDEX_SIZE;//Go back to the beginning if you reach the end
    if (index == start) { // table full
            free(item);
        return;
        }
    }
    hashArray[index] = item;
}
// Search
void* indexSearch(IndexItem *hashArray[], int key) {
    int index = indexHashCode(key);//Calculates the first slot to check using the hash function.
    int start = index;
    while (hashArray[index] != NULL) {
        if (hashArray[index] != dummyIndexItem && hashArray[index]->key == key)
        return hashArray[index]->ptr;
        index = (index + 1) % INDEX_SIZE;//Not found in this slot ,move to next slot
        if (index == start) break;
    }
    return NULL;
}
void indexDelete(IndexItem *hashArray[], int key) {
    int index = indexHashCode(key);
    int start = index;
    while (hashArray[index] != NULL) {
        if (hashArray[index] != dummyIndexItem && hashArray[index]->key == key) {
    free(hashArray[index]);
            hashArray[index] = dummyIndexItem;
        return;
        }
        index = (index + 1) % INDEX_SIZE;
        if (index == start) break;
    }
}
void indexFreeAll(IndexItem *hashArray[]) {
    for (int i = 0; i < INDEX_SIZE; i++) {
        if (hashArray[i] != NULL && hashArray[i] != dummyIndexItem) {
            free(hashArray[i]);
        }
        hashArray[i] = NULL;
    }
}
//========================================================================logging==================================================================
char currentUserName[60];
void getCurrentDateTime(char *buffer, int size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", t);
}

int getNextLogID() {
    FILE *fp = fopen(LOG_FILE, "r");
    int id = 1;
    char line[512];

    if (fp == NULL)
        return id;

    fgets(line, sizeof(line), fp);

    while (fgets(line, sizeof(line), fp))
        id++;

    fclose(fp);
    return id;
}

void writeHeaderIfNeeded() {
    FILE *fp = fopen(LOG_FILE, "r");
    if (fp == NULL) {
        fp = fopen(LOG_FILE, "w");
        fprintf(fp,
            "LogID,TableName,ColumnName,ID,OldValue,NewValue,ActionType,LogDate,Username\n");
    }
    fclose(fp);
}

void writeLog(const char *tableName, const char *column, int rowID,
              const char *oldValue, const char *newValue,
              const char *action)
{
    FILE *fp = fopen(LOG_FILE, "a");
    char date[30];

    int logID = getNextLogID();
    getCurrentDateTime(date, sizeof(date));

    fprintf(fp, "%d,%s,%s,%d,%s,%s,%s,%s,%s\n",
        logID, tableName, column, rowID,
        oldValue, newValue, action,
        date,currentUserName);

    fclose(fp);
}
    //---------------------------------------------------------------table row struct----------------------------------------------------------------------
    typedef struct {//represent one row in each table.
        int OrgID;
        char OrgName[51];
        char OrgType[31];
    } Organization;

    typedef struct {
        int role_id;
        char name[51];
        char description[201];
    } Role;

    typedef struct {
        int user_id;
        char FirstName[51];
        char LastName[101];
        char email[51];
        char Phone[21];
        char status[11];
        int OrgID;
        int role_id;
    } User;

    typedef struct {
        int ZoneID;
        char ZoneName[51];
        char description[256];
    } Zone;

    typedef struct {
        int gate_id;
        char name[101];
        int enabled;
        char location[256];
    } Gate;

    typedef struct {
        int ZoneID;
        int gate_id;
    } GateZone;

    typedef struct {
        int vehicle_id;
        char license_plate[21];
        char make[51];
        char model[51];
        char color[31];
        char status[11];
        int ZoneID;
        int user_id;
    } Vehicle;

    typedef struct {
        int permission_id;
        char target_type[21];
        int target_id;
        char action[11];
        char created_by[37];
        int gate_id;
        int role_id;
    } Permission;

    typedef struct {
        int log_id;
        char timestamp[30];
        char outcome[21];
        char reason[256];
        int gate_id;
        int vehicle_id;
        int user_id;
    } AccessLog;
    //-----------------------------------------------------------------Linked list node structs----------------------------------------------------------
//A linked-list node that stores one row and a pointer to the next row
    typedef struct OrganizationNode { Organization data ; struct OrganizationNode *next;} OrganizationNode;
    typedef struct RoleNode {Role data;struct RoleNode *next;} RoleNode;
    typedef struct UserNode {User data;struct UserNode *next;} UserNode;
    typedef struct ZoneNode {Zone data;struct ZoneNode *next;} ZoneNode;
    typedef struct GateNode {Gate data;struct GateNode *next;} GateNode;
    typedef struct GateZoneNode {GateZone data;struct GateZoneNode *next;} GateZoneNode;
    typedef struct VehicleNode {Vehicle data;struct VehicleNode *next;} VehicleNode;
    typedef struct PermissionNode {Permission data;struct PermissionNode *next;} PermissionNode;
    typedef struct AccessLogNode {AccessLog data;struct AccessLogNode *next;} AccessLogNode;
    //-----------------------------------------------------------------------Table structs---------------------------------------------------------------
//count: number of rows
//To control the table Instead of random nodes in memory have one controller object
    typedef struct {OrganizationNode *head;int count;int next_id;} OrganizationTable;
    typedef struct {RoleNode *head;int count;int next_id;} RoleTable;
    typedef struct {UserNode *head;int count;int next_id;} UserTable;
    typedef struct {ZoneNode *head;int count;int next_id;} ZoneTable;
    typedef struct {GateNode *head;int count;int next_id;} GateTable;
    typedef struct {GateZoneNode *head;int count;} GateZoneTable;
    typedef struct {VehicleNode *head;int count;int next_id;} VehicleTable;
    typedef struct {PermissionNode *head;int count;int next_id;} PermissionTable;
    typedef struct {AccessLogNode *head;int count;int next_id;} AccessLogTable;

//Frees all nodes in the organization table linked list.
    void free_organizations(OrganizationTable *t){
    OrganizationNode *cur=t->head;
    while(cur){ OrganizationNode *n=cur->next; free(cur); cur=n; }
    t->head=NULL; t->count=0;
}
void free_roles(RoleTable *t){
    RoleNode *cur=t->head;
    while(cur){ RoleNode *n=cur->next; free(cur); cur=n; }
    t->head=NULL; t->count=0;
}
void free_users(UserTable *t){
    UserNode *cur=t->head;
    while(cur){ UserNode *n=cur->next; free(cur); cur=n; }
    t->head=NULL; t->count=0;
}
void free_zones(ZoneTable *t){
    ZoneNode *cur=t->head;
    while(cur){ ZoneNode *n=cur->next; free(cur); cur=n; }
    t->head=NULL; t->count=0;
}
void free_gates(GateTable *t){
    GateNode *cur=t->head;
    while(cur){ GateNode *n=cur->next; free(cur); cur=n; }
    t->head=NULL; t->count=0;
}
void free_gate_zones(GateZoneTable *t){
    GateZoneNode *cur=t->head;
    while(cur){ GateZoneNode *n=cur->next; free(cur); cur=n; }
    t->head=NULL; t->count=0;
}
void free_vehicles(VehicleTable *t){
    VehicleNode *cur=t->head;
    while(cur){ VehicleNode *n=cur->next; free(cur); cur=n; }
    t->head=NULL; t->count=0;
}
void free_permissions(PermissionTable *t){
    PermissionNode *cur=t->head;
    while(cur){ PermissionNode *n=cur->next; free(cur); cur=n; }
    t->head=NULL; t->count=0;
}
void free_access_logs(AccessLogTable *t){
    AccessLogNode *cur=t->head;
    while(cur){ AccessLogNode *n=cur->next; free(cur); cur=n; }
    t->head=NULL; t->count=0;
}

//--------------------------------------------------------------------------add functions-------------------------------------------------------
OrganizationNode* add_organization(OrganizationTable *t, Organization org){
OrganizationNode *node = (OrganizationNode*)malloc(sizeof(OrganizationNode));
if (!node) return NULL;
    node->data = org;
        node->next = t->head;//Makes the new node point to the old first node
        t->head = node;
    t->count++;
    return node;
}
RoleNode* add_role(RoleTable *t, Role role){
RoleNode *node = (RoleNode*)malloc(sizeof(RoleNode));
    if (!node) return NULL;
        node->data = role;
    node->next = t->head;
        t->head = node;
    t->count++;
    return node;
}
UserNode* add_user(UserTable *t, User user){
    UserNode *node = (UserNode*)malloc(sizeof(UserNode));
    if (!node) return NULL;
node->data = user;
    node->next = t->head;
        t->head = node;
    t->count++;
        return node;
}
ZoneNode* add_zone(ZoneTable *t, Zone zone){
        ZoneNode *node = (ZoneNode*)malloc(sizeof(ZoneNode));
    if (!node) return NULL;
node->data = zone;
    node->next = t->head;
          t->head = node;
    t->count++;
 return node;
}
GateNode* add_gate(GateTable *t, Gate gate){
    GateNode *node = (GateNode*)malloc(sizeof(GateNode));
        if (!node) return NULL;
    node->data = gate;
node->next = t->head;
    t->head = node;
        t->count++;
return node;
}
GateZoneNode* add_GateZone(GateZoneTable *t, GateZone gz){
GateZoneNode *node = (GateZoneNode*)malloc(sizeof(GateZoneNode));
if (!node) return NULL;
    node->data = gz;
        node->next = t->head;
    t->head = node;
        t->count++;
return node;
}
VehicleNode* add_vehicle(VehicleTable *t, Vehicle v){
    VehicleNode *node = (VehicleNode*)malloc(sizeof(VehicleNode));
        if (!node) return NULL;
    node->data = v;
        node->next = t->head;
    t->head = node;
        t->count++;
return node;
}
PermissionNode* add_permission(PermissionTable *t, Permission per){
PermissionNode *node = (PermissionNode*)malloc(sizeof(PermissionNode));
    if (!node) return NULL;
    node->data = per;
        node->next = t->head;
    t->head = node;
        t->count++;
return node;
}
AccessLogNode* add_access_log(AccessLogTable *t, AccessLog al){
AccessLogNode *node = (AccessLogNode*)malloc(sizeof(AccessLogNode));
        if (!node) return NULL;
node->data = al;
    node->next = t->head;
    t->head = node;
t->count++;
        return node;
}
   //------------------------------------------------------------------write functions(store manager)----------------------------------------------------------------------
   //write from the memory(linked list) to the csv
    void write_organizations(OrganizationTable *t)
    {
    FILE *f = fopen("csvFiles\\organizations.csv", "w");
    if (!f) return;
    fprintf(f, "OrgID,OrgName,OrgType\n");
    OrganizationNode *cur = t->head;
    while (cur != NULL) {
    fprintf(f, "%d,%s,%s\n",
    cur->data.OrgID,
    cur->data.OrgName,
    cur->data.OrgType);
    cur = cur->next;
    }
    fclose(f);

    }
    void write_roles(RoleTable *t)
    {
    FILE *f = fopen("csvFiles\\roles.csv", "w");
    if (!f) return;
    fprintf(f, "role_id,name,description\n");
    RoleNode *cur = t->head;
    while (cur != NULL) {
    fprintf(f, "%d,%s,%s\n",
    cur->data.role_id,
    cur->data.name,
    cur->data.description);
    cur = cur->next;
    }
    fclose(f);
    }

    void write_users(UserTable *t)
    {
    FILE *f = fopen("csvFiles\\users.csv", "w");
    if (!f) return;
    fprintf(f, "user_id,FirstName,LastName,email,Phone,status,OrgID,role_id\n");
    UserNode *cur = t->head;
    while (cur != NULL) {
    fprintf(f, "%d,%s,%s,%s,%s,%s,%d,%d\n",
    cur->data.user_id,
    cur->data.FirstName,
    cur->data.LastName,
    cur->data.email,
    cur->data.Phone,
    cur->data.status,
    cur->data.OrgID,
    cur->data.role_id);
    cur = cur->next;
    }
    fclose(f);
    }

    void write_zones(ZoneTable *t)
    {
    FILE *f = fopen("csvFiles\\zones.csv", "w");
    if (!f) return;
    fprintf(f, "ZoneID,ZoneName,description\n");
    ZoneNode *cur = t->head;
    while (cur != NULL) {
    fprintf(f, "%d,%s,%s\n",
    cur->data.ZoneID,
    cur->data.ZoneName,
    cur->data.description);
    cur = cur->next;
    }
    fclose(f);
    }

    void write_gates(GateTable *t) {
    FILE *f = fopen("csvFiles\\gates.csv", "w");
    if (!f) return;
    fprintf(f, "gate_id,name,enabled,location\n");
    GateNode *cur = t->head;
    while (cur != NULL) {
    fprintf(f, "%d,%s,%d,%s\n",
    cur->data.gate_id,
    cur->data.name,
    cur->data.enabled,
    cur->data.location);
    cur = cur->next;
    }
    fclose(f);
    }

    void write_gate_zones(GateZoneTable *t)
    {
    FILE *f = fopen("csvFiles\\gate_zone.csv", "w");
    if (!f) return;
    fprintf(f, "ZoneID,gate_id\n");
    GateZoneNode *cur = t->head;
    while (cur != NULL) {
    fprintf(f, "%d,%d\n",
    cur->data.ZoneID,
    cur->data.gate_id);
    cur = cur->next;
        }
    fclose(f);
    }

    void write_permissions(PermissionTable *t)
    {
    FILE *f = fopen("csvFiles\\permissions.csv", "w");
    if (!f) return;
    fprintf(f, "permission_id,target_type,target_id,action,created_by,gate_id,role_id\n");
    PermissionNode *cur = t->head;
    while (cur != NULL) {
    fprintf(f, "%d,%s,%d,%s,%s,%d,%d\n",
    cur->data.permission_id,
    cur->data.target_type,
    cur->data.target_id,
    cur->data.action,
    cur->data.created_by,

    cur->data.gate_id,
    cur->data.role_id);
    cur = cur->next;
    }
        fclose(f);
    }

    void write_access_logs(AccessLogTable *t) {
    FILE *f = fopen( "csvFiles\\access_log.csv", "w");
    if (!f) return;
        fprintf(f, "log_id,timestamp,outcome,reason,gate_id,vehicle_id,user_id\n");
    AccessLogNode *cur = t->head;
    while (cur != NULL) {
            fprintf(f, "%d,%s,%s,%s,%d,%d,%d\n",
            cur->data.log_id,
            cur->data.timestamp,
            cur->data.outcome,
            cur->data.reason,
            cur->data.gate_id,
            cur->data.vehicle_id,
            cur->data.user_id);
            cur = cur->next;
    }
        fclose(f);
    }

    void write_vehicles(VehicleTable *t) {
    FILE *f = fopen("csvFiles\\vehicles.csv", "w");
    if (!f) return;
    fprintf(f, "vehicle_id,license_plate,make,model,color,status,ZoneID,user_id\n");
        VehicleNode *cur = t->head;
    while (cur != NULL) {
        fprintf(f, "%d,%s,%s,%s,%s,%s,%d,%d\n",
        cur->data.vehicle_id,
        cur->data.license_plate,
        cur->data.make,
        cur->data.model,
        cur->data.color,
        cur->data.status,
        cur->data.ZoneID,
        cur->data.user_id);
        cur = cur->next;
       }
        fclose(f);
    }
    //========================================================================read functios====================================================================
  //load form csv to the memory
  void read_vehicles(VehicleTable *t) {
    FILE *f = fopen("csvFiles\\vehicles.csv", "r");
    if (!f) {
        free_vehicles(t);//clear table in the memory
        t->next_id = 1;
        return;
    }
    free_vehicles(t);//to avoid duplicates
    t->next_id = 1;
    char header[200];//buffer to store the header line (column names)
    fscanf(f, "%199[^\n]\n", header);

    while (1) {
        Vehicle v;//create a struct to fill form csv row
int r = fscanf(f, "%d,%20[^,],%50[^,],%50[^,],%30[^,],%10[^,],%d,%d\n",&v.vehicle_id, v.license_plate, v.make, v.model,v.color, v.status, &v.ZoneID, &v.user_id);
        if (r != 8) break;
        add_vehicle(t, v);
        if (v.vehicle_id >= t->next_id)
            t->next_id = v.vehicle_id + 1;
    }
    fclose(f);
}

    void read_organizations(OrganizationTable *t) {
    FILE *f = fopen("csvFiles\\organizations.csv", "r");
    if (!f) {
        free_organizations(t);
         t->next_id = 1;
    return;
    }    free_organizations(t);
     t->next_id = 1;
    char header[200];
    fscanf(f, "%199[^\n]\n", header);

    while (1) {
    Organization o;
    int r = fscanf(f, "%d,%50[^,],%30[^\n]\n",&o.OrgID,o.OrgName,o.OrgType);
    if (r != 3)
    break;
    add_organization(t, o);
    if (o.OrgID >= t->next_id) t->next_id = o.OrgID + 1;
    }
    fclose(f);
    }

   void read_roles(RoleTable *t) {
    FILE *f = fopen("csvFiles\\roles.csv", "r");
    if (!f) {
        free_roles(t);
        t->next_id = 1;
        return;
    }
    free_roles(t);
    t->next_id = 1;
    char header[200];
    fscanf(f, "%199[^\n]\n", header);
    while (1) {
        Role rrow;
        int r = fscanf(f, "%d,%50[^,],%200[^\n]\n",&rrow.role_id, rrow.name, rrow.description);
        if (r != 3) break;
        add_role(t, rrow);
    if (rrow.role_id >= t->next_id) t->next_id = rrow.role_id + 1;
    }
    fclose(f);
}


    void read_users(UserTable *t) {
    FILE *f = fopen("csvFiles\\users.csv", "r");
    if (!f) {
        free_users(t);
        t->next_id = 1;
        return;
    }
    free_users(t);
    t->next_id = 1;
    char header[300];
    fscanf(f, "%299[^\n]\n", header);
    while (1) {
        User u;
int r = fscanf(f, "%d,%50[^,],%100[^,],%50[^,],%20[^,],%10[^,],%d,%d\n",&u.user_id, u.FirstName, u.LastName, u.email, u.Phone,u.status, &u.OrgID, &u.role_id);
        if (r != 8) break;
        add_user(t, u);
    if (u.user_id >= t->next_id) t->next_id = u.user_id + 1;//Ensures next_id is always bigger than any existing user_id.
    }
    fclose(f);
}
   void read_zones(ZoneTable *t) {
    FILE *f = fopen("csvFiles\\zones.csv", "r");
    if (!f) {
        free_zones(t);
        t->next_id = 1;
        return;
    }
    free_zones(t);
    t->next_id = 1;
    char header[200];
    fscanf(f, "%199[^\n]\n", header);
    while (1) {
        Zone z;
        int r = fscanf(f, "%d,%50[^,],%255[^\n]\n",&z.ZoneID, z.ZoneName, z.description);
        if (r != 3) break;
        add_zone(t, z);
    if (z.ZoneID >= t->next_id) t->next_id = z.ZoneID + 1;
    }
    fclose(f);
}

   void read_gates(GateTable *t) {
    FILE *f = fopen("csvFiles\\gates.csv", "r");
    if (!f) {
        free_gates(t);
        t->next_id = 1;
        return;
    }
    free_gates(t);
    t->next_id = 1;
    char header[200];
    fscanf(f, "%199[^\n]\n", header);
    while (1) {
        Gate g;
        int r = fscanf(f, "%d,%100[^,],%d,%100[^\n]\n",&g.gate_id, g.name, &g.enabled, g.location);
        if (r != 4) break;
        add_gate(t, g);
    if (g.gate_id >= t->next_id) t->next_id = g.gate_id + 1;
    }
    fclose(f);
}
void read_gate_zones(GateZoneTable *t) {
    FILE *f = fopen("csvFiles\\gate_zone.csv", "r");
    if (!f) {
        free_gate_zones(t);

        return;
    }
    free_gate_zones(t);

    char header[200];
    fscanf(f, "%199[^\n]\n", header);
    while (1) {
        GateZone gz;
        int r = fscanf(f, "%d,%d\n", &gz.ZoneID, &gz.gate_id);
        if (r != 2) break;
    add_GateZone(t, gz);
    }
    fclose(f);
}

    void read_permissions(PermissionTable *t) {
    FILE *f = fopen("csvFiles\\permissions.csv", "r");
    if (!f) {
        free_permissions(t);
        t->next_id = 1;
        return;
    }

    free_permissions(t);
    t->next_id = 1;

    char header[300];
    fscanf(f, "%299[^\n]\n", header);

    while (1) {
        Permission p;
        int r = fscanf(f,"%d,%20[^,],%d,%10[^,],%36[^,],%d,%d\n",
                       &p.permission_id, p.target_type, &p.target_id,p.action, p.created_by,&p.gate_id, &p.role_id);
        if (r != 7) break;
        add_permission(t, p);
        if (p.permission_id >= t->next_id) t->next_id = p.permission_id + 1;
    }
    fclose(f);
}

   void read_access_logs(AccessLogTable *t) {
    FILE *f = fopen("csvFiles\\access_log.csv", "r");
    if (!f) {
        free_access_logs(t);
        t->next_id = 1;
        return;
    }
    free_access_logs(t);
    t->next_id = 1;
    char header[300];
    fscanf(f, "%299[^\n]\n", header);
    while (1) {
        AccessLog al;
        int r = fscanf(f,"%d,%29[^,],%20[^,],%255[^,],%d,%d,%d\n",
                       &al.log_id, al.timestamp, al.outcome, al.reason,&al.gate_id, &al.vehicle_id, &al.user_id);
        if (r != 7) break;
        add_access_log(t, al);
        if (al.log_id >= t->next_id) t->next_id = al.log_id + 1;
    }
    fclose(f);
}

    //==================================================================================find by id======================================================
   Organization* find_organization_by_id(OrganizationTable *t, int OrgID) {
    (void)t;//warnings
    return (Organization*) indexSearch(orgIndex, OrgID);
}//find the key and return the ptr
Role* find_role_by_id(RoleTable *t, int role_id) {
    (void)t;
    return (Role*) indexSearch(roleIndex, role_id);
}
User* find_user_by_id(UserTable *t, int user_id) {
    (void)t;
    return (User*) indexSearch(userIndex, user_id);
}
Zone* find_zone_by_id(ZoneTable *t, int ZoneID) {
    (void)t;
    return (Zone*) indexSearch(zoneIndex, ZoneID);
}
Gate* find_gate_by_id(GateTable *t, int gate_id) {
    (void)t;
    return (Gate*) indexSearch(gateIndex, gate_id);
}
GateZone* find_gate_zone(GateZoneTable *t, int ZoneID, int gate_id) {
    (void)t;
    return (GateZone*) indexSearch(gateZoneIndex, gateZoneKey(ZoneID, gate_id));
}
Vehicle* find_vehicle_by_id(VehicleTable *t, int vehicle_id) {
    (void)t;
    return (Vehicle*) indexSearch(vehicleIndex, vehicle_id);
}
Permission* find_permission_by_id(PermissionTable *t, int permission_id) {
    (void)t;
    return (Permission*) indexSearch(permissionIndex, permission_id);
}
AccessLog* find_access_log_by_id(AccessLogTable *t, int log_id) {
    (void)t;
    return (AccessLog*) indexSearch(accessLogIndex, log_id);
}
    //============================================================insert functions====================================================================
  int insert_organization(OrganizationTable *orgs, Organization o) {
    if (find_organization_by_id(orgs, o.OrgID) != NULL) {
        printf("Insert failed: duplicate OrgID\n");
        return 0;
    }
    OrganizationNode *n = add_organization(orgs, o);
    if (!n) return 0;
    indexInsert(orgIndex, o.OrgID, &n->data);
    return 1;
}

int insert_role(RoleTable *roles, Role r) {
    if (find_role_by_id(roles, r.role_id) != NULL) {
        printf("Insert failed: duplicate role_id\n");
        return 0;
    }
    RoleNode *n = add_role(roles, r);
    if (!n) return 0;
    indexInsert(roleIndex, r.role_id, &n->data);
    return 1;
}

int insert_user(UserTable *users, OrganizationTable *orgs, RoleTable *roles, User u) {
    if (u.OrgID <= 0) { printf("Insert failed: OrgID cannot be 0\n"); return 0; }
    if (u.role_id <= 0) { printf("Insert failed: role_id cannot be 0\n"); return 0; }

    if (find_user_by_id(users, u.user_id) != NULL) {
        printf("Insert failed: duplicate user_id\n");
        return 0;
    }
    if (find_organization_by_id(orgs, u.OrgID) == NULL) {
        printf("Insert failed: OrgID FK not found\n");
        return 0;
    }
    if (find_role_by_id(roles, u.role_id) == NULL) {
        printf("Insert failed: role_id FK not found\n");
        return 0;
    }

    UserNode *n = add_user(users, u);
    if (!n) return 0;
    indexInsert(userIndex, u.user_id, &n->data);
    return 1;
}

int insert_zone(ZoneTable *zones, Zone z) {
    if (find_zone_by_id(zones, z.ZoneID) != NULL) {
        printf("Insert failed: duplicate ZoneID\n");
        return 0;
    }
    ZoneNode *n = add_zone(zones, z);
    if (!n) return 0;
    indexInsert(zoneIndex, z.ZoneID, &n->data);
    return 1;
}

int insert_gate(GateTable *gates, Gate g) {
    if (find_gate_by_id(gates, g.gate_id) != NULL) {
        printf("Insert failed: duplicate gate_id\n");
        return 0;
    }
    if (g.enabled != 0 && g.enabled != 1) {
        printf("Insert failed: enabled must be 0 or 1\n");
        return 0;
    }
    GateNode *n = add_gate(gates, g);
    if (!n) return 0;
    indexInsert(gateIndex, g.gate_id, &n->data);
    return 1;
}

int insert_vehicle(VehicleTable *vehicles, UserTable *users, ZoneTable *zones, Vehicle v) {
    if (find_vehicle_by_id(vehicles, v.vehicle_id) != NULL) {
        printf("Insert failed: duplicate vehicle_id\n");
        return 0;
    }
    if (find_user_by_id(users, v.user_id) == NULL) {
        printf("Insert failed: user_id FK not found\n");
        return 0;
    }
    if (find_zone_by_id(zones, v.ZoneID) == NULL) {
        printf("Insert failed: ZoneID FK not found\n");
        return 0;
    }

    VehicleNode *n = add_vehicle(vehicles, v);
    if (!n) return 0;
    indexInsert(vehicleIndex, v.vehicle_id, &n->data);
    return 1;
}

int insert_gate_zone(GateZoneTable *gateZones, ZoneTable *zones, GateTable *gates, GateZone gz) {
    if (find_zone_by_id(zones, gz.ZoneID) == NULL) {
        printf("Insert failed: ZoneID FK not found\n");
        return 0;
    }
    if (find_gate_by_id(gates, gz.gate_id) == NULL) {
        printf("Insert failed: gate_id FK not found\n");
        return 0;
    }
    if (find_gate_zone(gateZones, gz.ZoneID, gz.gate_id) != NULL) {
        printf("Insert failed: duplicate GateZone (ZoneID + gate_id)\n");
        return 0;
    }

    GateZoneNode *n = add_GateZone(gateZones, gz);
    if (!n) return 0;
    indexInsert(gateZoneIndex, gateZoneKey(gz.ZoneID, gz.gate_id), &n->data);
    return 1;
}

int insert_permission(PermissionTable *permissions, GateTable *gates, RoleTable *roles, VehicleTable *vehicles, Permission p) {
    if (find_permission_by_id(permissions, p.permission_id) != NULL) {
        printf("Insert failed: duplicate permission_id\n");
        return 0;
    }
    if (strcmp(p.action, "allow") != 0 && strcmp(p.action, "deny") != 0) {
        printf("Insert failed: action must be 'allow' or 'deny'\n");
        return 0;
    }
    if (p.gate_id <= 0 || find_gate_by_id(gates, p.gate_id) == NULL) {
        printf("Insert failed: gate_id FK not found\n");
        return 0;
    }
    if (p.role_id <= 0 || find_role_by_id(roles, p.role_id) == NULL) {
        printf("Insert failed: role_id FK not found\n");
        return 0;
    }

    if (strcmp(p.target_type, "role") == 0) {
        if (p.target_id <= 0 || find_role_by_id(roles, p.target_id) == NULL) {
            printf("Insert failed: target_id role not found\n");
            return 0;
        }
    } else if (strcmp(p.target_type, "vehicle") == 0) {
        if (p.target_id <= 0 || find_vehicle_by_id(vehicles, p.target_id) == NULL) {
            printf("Insert failed: target_id vehicle not found\n");
            return 0;
        }
    } else {
        printf("Insert failed: invalid target_type (use role or vehicle)\n");
        return 0;
    }

    PermissionNode *n = add_permission(permissions, p);
    if (!n) return 0;
    indexInsert(permissionIndex, p.permission_id, &n->data);
    return 1;
}

int insert_access_log(AccessLogTable *logs, GateTable *gates, VehicleTable *vehicles, UserTable *users, AccessLog al) {
    if (find_access_log_by_id(logs, al.log_id) != NULL) {
        printf("Insert failed: duplicate log_id\n");
        return 0;
    }
    if (find_gate_by_id(gates, al.gate_id) == NULL) {
        printf("Insert failed: gate_id FK not found\n");
        return 0;
    }
    if (find_vehicle_by_id(vehicles, al.vehicle_id) == NULL) {
        printf("Insert failed: vehicle_id FK not found\n");
        return 0;
    }
    if (find_user_by_id(users, al.user_id) == NULL) {
        printf("Insert failed: user_id FK not found\n");
        return 0;
    }

    AccessLogNode *n = add_access_log(logs, al);
    if (!n) return 0;
    indexInsert(accessLogIndex, al.log_id, &n->data);
    return 1;
}
//================================================================update functions================================================================
int update_organization(OrganizationTable *orgs, int OrgID, char *newName, char *newType){
    Organization *o = find_organization_by_id(orgs, OrgID);
    if(!o){ printf("Update failed: OrgID not found\n"); return 0; }
    strcpy(o->OrgName, newName);
    strcpy(o->OrgType, newType);
    return 1;
}
int update_role(RoleTable *roles, int role_id, char *newName, char *newDesc){
    Role *r = find_role_by_id(roles, role_id);
    if(!r){ printf("Update failed: role_id not found\n"); return 0; }
    strcpy(r->name, newName);
    strcpy(r->description, newDesc);
    return 1;
}
int update_user(UserTable *users, OrganizationTable *orgs, RoleTable *roles,
                int user_id, char *newFirstName, char *newLastName, char *newEmail,
                char *newPhone, char *newStatus, int newOrgID, int newRoleID){
    User *u = find_user_by_id(users, user_id);
    if(!u){ printf("Update failed: user_id not found\n"); return 0; }
    if (newOrgID <= 0) { printf("Update failed: OrgID cannot be 0\n"); return 0; }
    if (newRoleID <= 0) { printf("Update failed: role_id cannot be 0\n"); return 0; }
    if(find_organization_by_id(orgs, newOrgID) == NULL){
        printf("Update failed: OrgID FK not found\n"); return 0;
    }
    if(find_role_by_id(roles, newRoleID) == NULL){
        printf("Update failed: role_id FK not found\n"); return 0;
    }
    strcpy(u->FirstName, newFirstName);
    strcpy(u->LastName, newLastName);
    strcpy(u->email, newEmail);
    strcpy(u->Phone, newPhone);
    strcpy(u->status, newStatus);
    u->OrgID = newOrgID;
    u->role_id = newRoleID;
    return 1;
}
int update_zone(ZoneTable *zones, int ZoneID, char *newName, char *newDesc){
    Zone *z = find_zone_by_id(zones, ZoneID);
    if(!z){ printf("Update failed: ZoneID not found\n"); return 0; }
    strcpy(z->ZoneName, newName);
    strcpy(z->description, newDesc);
    return 1;
}
int update_gate(GateTable *gates, int gate_id, char *newName, int enabled, char *newLoc){
    Gate *g = find_gate_by_id(gates, gate_id);
    if(!g){ printf("Update failed: gate_id not found\n"); return 0; }
    if(enabled != 0 && enabled != 1){
        printf("Update failed: enabled must be 0 or 1\n"); return 0;
    }
    strcpy(g->name, newName);
    g->enabled = enabled;
    strcpy(g->location, newLoc);
    return 1;
}
int update_vehicle(VehicleTable *vehicles, UserTable *users, ZoneTable *zones,
                   int vehicle_id, char *plate, char *make, char *model, char *color, char *status,
                   int newZoneID, int newUserID){
    Vehicle *v = find_vehicle_by_id(vehicles, vehicle_id);
    if(!v){ printf("Update failed: vehicle_id not found\n"); return 0; }
    if(find_zone_by_id(zones, newZoneID) == NULL){
        printf("Update failed: ZoneID FK not found\n"); return 0;
}
    if(find_user_by_id(users, newUserID) == NULL){
        printf("Update failed: user_id FK not found\n"); return 0;
}
    strcpy(v->license_plate, plate);
    strcpy(v->make, make);
    strcpy(v->model, model);
    strcpy(v->color, color);
    strcpy(v->status, status);
    v->ZoneID = newZoneID;
    v->user_id = newUserID;
    return 1;
}
int update_permission(PermissionTable *permissions, GateTable *gates, RoleTable *roles, VehicleTable *vehicles,
                      int permission_id, char *target_type, int target_id, char *action, char *created_by,
                      int gate_id, int role_id)
{
    Permission *p = find_permission_by_id(permissions, permission_id);
    if(!p){ printf("Update failed: permission_id not found\n"); return 0; }
    if (strcmp(action, "allow") != 0 && strcmp(action, "deny") != 0) {
        printf("Update failed: action must be 'allow' or 'deny'\n");
        return 0;
    }
    if (gate_id <= 0 || find_gate_by_id(gates, gate_id) == NULL) {
        printf("Update failed: gate_id FK not found\n");
        return 0;
    }
    if (role_id <= 0 || find_role_by_id(roles, role_id) == NULL) {
        printf("Update failed: role_id FK not found\n");
        return 0;
    }
    if (target_id <= 0) {
        printf("Update failed: target_id cannot be 0\n");
        return 0;
    }
    if (strcmp(target_type, "role") == 0) {
        if (find_role_by_id(roles, target_id) == NULL) {
            printf("Update failed: target role not found\n");
    return 0;
        }
    } else if (strcmp(target_type, "vehicle") == 0) {
        if (find_vehicle_by_id(vehicles, target_id) == NULL) {
            printf("Update failed: target vehicle not found\n");
            return 0;
        }
    } else {
        printf("Update failed: invalid target_type\n");
        return 0;
    }
    strcpy(p->target_type, target_type);
    p->target_id = target_id;
    strcpy(p->action, action);
    strcpy(p->created_by, created_by);
    p->gate_id = gate_id;
    p->role_id = role_id;
    return 1;
}
int update_access_log(AccessLogTable *logs, GateTable *gates, VehicleTable *vehicles, UserTable *users,
                      int log_id, char *outcome, char *reason, int gate_id, int vehicle_id, int user_id){
    AccessLog *a = find_access_log_by_id(logs, log_id);
    if(!a){ printf("Update failed: log_id not found\n"); return 0; }
    if(find_gate_by_id(gates, gate_id) == NULL){ printf("Update failed: gate_id FK not found\n"); return 0; }
    if(find_vehicle_by_id(vehicles, vehicle_id) == NULL){ printf("Update failed: vehicle_id FK not found\n"); return 0; }
    if(find_user_by_id(users, user_id) == NULL){ printf("Update failed: user_id FK not found\n"); return 0; }
    strcpy(a->outcome, outcome);
    strcpy(a->reason, reason);
    a->gate_id = gate_id;
    a->vehicle_id = vehicle_id;
    a->user_id = user_id;
    return 1;
}
int update_gate_zone(GateZoneTable *gateZones, ZoneTable *zones, GateTable *gates,
                     int oldZoneID, int oldGateID, int newZoneID, int newGateID){
    GateZone *gz = find_gate_zone(gateZones, oldZoneID, oldGateID);
    if(!gz){ printf("Update failed: GateZone not found\n"); return 0; }

    if(find_zone_by_id(zones, newZoneID) == NULL){ printf("Update failed: ZoneID FK not found\n"); return 0; }
    if(find_gate_by_id(gates, newGateID) == NULL){ printf("Update failed: gate_id FK not found\n"); return 0; }
    if(find_gate_zone(gateZones, newZoneID, newGateID) != NULL &&
       !(newZoneID == oldZoneID && newGateID == oldGateID)){
        printf("Update failed: duplicate GateZone\n");
        return 0;
    }
    indexDelete(gateZoneIndex, gateZoneKey(oldZoneID, oldGateID));
    gz->ZoneID = newZoneID;
    gz->gate_id = newGateID;
    indexInsert(gateZoneIndex, gateZoneKey(newZoneID, newGateID), gz);
    return 1;
}
//==================================================================delete functions===================================================================
int delete_organization(OrganizationTable *orgs, UserTable *users, int OrgID) {
    for (UserNode *u = users->head; u; u = u->next) {
        if (u->data.OrgID == OrgID) {
            printf("Delete blocked: Organization (OrgID=%d) is referenced by Users (user_id=%d)\n",
                   OrgID, u->data.user_id);
            return 0;
        }
    }
    OrganizationNode *cur = orgs->head, *prev = NULL;
    while (cur && cur->data.OrgID != OrgID) {
        prev = cur;
        cur = cur->next;
    }
    if (!cur) {
        printf("Delete failed: Organization (OrgID=%d) not found\n", OrgID);
        return 0;
    }
    if (prev) prev->next = cur->next;
    else orgs->head = cur->next;
    free(cur);
    orgs->count--;
    indexDelete(orgIndex, OrgID);
    printf("Organization (OrgID=%d) deleted.\n", OrgID);
    return 1;
}
int delete_role(RoleTable *roles, UserTable *users, PermissionTable *perms, int role_id) {
    for (UserNode *u = users->head; u; u = u->next) {
        if (u->data.role_id == role_id) {
            printf("Delete blocked: Role (role_id=%d) is referenced by Users (user_id=%d)\n",
                   role_id, u->data.user_id);
            return 0;
        }
    }
    for (PermissionNode *p = perms->head; p; p = p->next) {
        if (p->data.role_id == role_id) {
            printf("Delete blocked: Role (role_id=%d) is referenced by Permissions (permission_id=%d as role_id FK)\n",
                   role_id, p->data.permission_id);
            return 0;
        }
        if (strcmp(p->data.target_type, "role") == 0 && p->data.target_id == role_id) {
            printf("Delete blocked: Role (role_id=%d) is referenced by Permissions (permission_id=%d as target role)\n",
                   role_id, p->data.permission_id);
            return 0;
        }
    }
    RoleNode *cur = roles->head, *prev = NULL;
    while (cur && cur->data.role_id != role_id) {
        prev = cur;
        cur = cur->next;
    }
    if (!cur) {
        printf("Delete failed: Role (role_id=%d) not found\n", role_id);
        return 0;
    }
    if (prev) prev->next = cur->next;
    else roles->head = cur->next;
    free(cur);
    roles->count--;
    indexDelete(roleIndex, role_id);
    printf("Role (role_id=%d) deleted.\n", role_id);
    return 1;
}

int delete_user(UserTable *users, VehicleTable *veh, AccessLogTable *logs, int user_id) {
    for (VehicleNode *v = veh->head; v; v = v->next) {
        if (v->data.user_id == user_id) {
        printf("Delete blocked: User (user_id=%d) is referenced by Vehicles (vehicle_id=%d)\n",
                   user_id, v->data.vehicle_id);
        return 0;
        }
    }
for (AccessLogNode *l = logs->head; l; l = l->next) {
        if (l->data.user_id == user_id) {
        printf("Delete blocked: User (user_id=%d) is referenced by Access Logs (log_id=%d)\n",
                   user_id, l->data.log_id);
        return 0;
    }
    }
    UserNode *cur = users->head, *prev = NULL;
    while (cur && cur->data.user_id != user_id) {
        prev = cur;
        cur = cur->next;
    }

    if (!cur) {
        printf("Delete failed: User (user_id=%d) not found\n", user_id);
        return 0;
    }
    if (prev) prev->next = cur->next;// deleting form the middle and end , make the prev points to next
    else users->head = cur->next;//deleting form start heads become next
    free(cur);
    users->count--;
    indexDelete(userIndex, user_id);
    printf("User (user_id=%d) deleted.\n", user_id);
    return 1;
}
int delete_zone(ZoneTable *zones, VehicleTable *veh, GateZoneTable *gz, int ZoneID) {
    for (VehicleNode *v = veh->head; v; v = v->next) {
        if (v->data.ZoneID == ZoneID) {
            printf("Delete blocked: Zone (ZoneID=%d) is referenced by Vehicles (vehicle_id=%d)\n",
                   ZoneID, v->data.vehicle_id);
            return 0;
        }
    }
for (GateZoneNode *g = gz->head; g; g = g->next) {
    if (g->data.ZoneID == ZoneID) {
            printf("Delete blocked: Zone (ZoneID=%d) is referenced by GateZone (gate_id=%d)\n",
                ZoneID, g->data.gate_id);
            return 0;
        }
    }
    ZoneNode *cur = zones->head, *prev = NULL;
    while (cur && cur->data.ZoneID != ZoneID) {
        prev = cur;
        cur = cur->next;
    }
    if (!cur) {
        printf("Delete failed: Zone (ZoneID=%d) not found\n", ZoneID);
        return 0;
    }
    if (prev) prev->next = cur->next;
    else zones->head = cur->next;
    free(cur);
    zones->count--;
    indexDelete(zoneIndex, ZoneID);
    printf("Zone (ZoneID=%d) deleted.\n", ZoneID);
    return 1;
}
int delete_gate(GateTable *gates, GateZoneTable *gz, PermissionTable *perms, AccessLogTable *logs, int gate_id) {
    for (GateZoneNode *g = gz->head; g; g = g->next) {
        if (g->data.gate_id == gate_id) {
            printf("Delete blocked: Gate (gate_id=%d) is referenced by GateZone (ZoneID=%d)\n",
                   gate_id, g->data.ZoneID);
                   return 0;
        }
    }
    for (PermissionNode *p = perms->head; p; p = p->next) {
    if (p->data.gate_id == gate_id) {
            printf("Delete blocked: Gate (gate_id=%d) is referenced by Permissions (permission_id=%d)\n",
                gate_id, p->data.permission_id);
                 return 0;
        }
    }
    for (AccessLogNode *l = logs->head; l; l = l->next) {
        if (l->data.gate_id == gate_id) {
            printf("Delete blocked: Gate (gate_id=%d) is referenced by Access Logs (log_id=%d)\n",
                   gate_id, l->data.log_id);
            return 0;
        }
    }
    GateNode *cur = gates->head, *prev = NULL;
    while (cur && cur->data.gate_id != gate_id) {
        prev = cur;
        cur = cur->next;
    }
    if (!cur) {
        printf("Delete failed: Gate (gate_id=%d) not found\n", gate_id);
        return 0;
    }
    if (prev) prev->next = cur->next;
    else gates->head = cur->next;
    free(cur);
    gates->count--;
    indexDelete(gateIndex, gate_id);
    printf("Gate (gate_id=%d) deleted.\n", gate_id);
    return 1;
}
int delete_gatezone(GateZoneTable *t, int ZoneID, int gate_id) {
    GateZoneNode *cur = t->head, *prev = NULL;
    while (cur && !(cur->data.ZoneID == ZoneID && cur->data.gate_id == gate_id)) {
        prev = cur;
        cur = cur->next;
    }
    if (!cur) {
        printf("Delete failed: GateZone not found (ZoneID=%d, gate_id=%d)\n", ZoneID, gate_id);
        return 0;
    }
    if (prev) prev->next = cur->next;
    else t->head = cur->next;
    free(cur);
    t->count--;
    indexDelete(gateZoneIndex, gateZoneKey(ZoneID, gate_id));
    printf("GateZone (ZoneID=%d, gate_id=%d) deleted.\n", ZoneID, gate_id);
    return 1;
}
int delete_vehicle(VehicleTable *veh, PermissionTable *perms, AccessLogTable *logs, int vehicle_id) {
    for (PermissionNode *p = perms->head; p; p = p->next) {
        if (strcmp(p->data.target_type, "vehicle") == 0 && p->data.target_id == vehicle_id) {
            printf("Delete blocked: Vehicle (vehicle_id=%d) is referenced by Permissions (permission_id=%d)\n",
                   vehicle_id, p->data.permission_id);
                return 0;
        }
    }
    // should i delete it or not
    for (AccessLogNode *l = logs->head; l; l = l->next) {
        if (l->data.vehicle_id == vehicle_id) {
            printf("Delete blocked: Vehicle (vehicle_id=%d) is referenced by Access Logs (log_id=%d)\n",
                   vehicle_id, l->data.log_id);
            return 0;
        }
    }
    VehicleNode *cur = veh->head, *prev = NULL;
    while (cur && cur->data.vehicle_id != vehicle_id) {
        prev = cur;
        cur = cur->next;
    }
    if (!cur) {
        printf("Delete failed: Vehicle (vehicle_id=%d) not found\n", vehicle_id);
        return 0;
    }
    if (prev) prev->next = cur->next;
    else veh->head = cur->next;
    free(cur);
    veh->count--;
    indexDelete(vehicleIndex, vehicle_id);
    printf("Vehicle (vehicle_id=%d) deleted.\n", vehicle_id);
    return 1;
}
int delete_permission(PermissionTable *permissions, int permission_id) {
    PermissionNode *cur = permissions->head;
    PermissionNode *prev = NULL;

    while (cur != NULL) {
        if (cur->data.permission_id == permission_id) {
    if (prev == NULL) permissions->head = cur->next;
            else prev->next = cur->next;
    free(cur);
        permissions->count--;
        indexDelete(permissionIndex, permission_id);

        printf("Permission (permission_id=%d) deleted.\n", permission_id);
            return 1;
        }
    prev = cur;
    cur = cur->next;
    }
    printf("Delete failed: Permission (permission_id=%d) not found\n", permission_id);
    return 0;
}
int delete_access_log(AccessLogTable *logs, int log_id) {
    AccessLogNode *cur = logs->head;
    AccessLogNode *prev = NULL;
    while (cur != NULL) {
        if (cur->data.log_id == log_id) {
            if (prev == NULL) logs->head = cur->next;
            else prev->next = cur->next;
            free(cur);
            logs->count--;
            indexDelete(accessLogIndex, log_id);
            printf("Access Log (log_id=%d) deleted.\n", log_id);
            return 1;
        }
        prev = cur;
        cur = cur->next;
    }
    printf("Delete failed: Access Log (log_id=%d) not found\n", log_id);
    return 0;
}
//============================================================helper functions====================================================================

    void get_now(char *out) {
    time_t t = time(NULL);
    struct tm *tmv = localtime(&t);
    sprintf(out, "%04d-%02d-%02d %02d:%02d:%02d",
            tmv->tm_year + 1900, tmv->tm_mon + 1, tmv->tm_mday,
            tmv->tm_hour, tmv->tm_min, tmv->tm_sec);
}

int register_gate_entry(
    VehicleTable *vehicles,
    UserTable *users,
    GateTable *gates,
    GateZoneTable *gateZones,
    PermissionTable *permissions,
    AccessLogTable *logs,
    int log_id,
    int vehicle_id,
    int gate_id
) {
    AccessLog al;
    al.log_id = log_id;
    get_now(al.timestamp);
    al.gate_id = gate_id;
    al.vehicle_id = vehicle_id;
    al.user_id = 0;

    Vehicle *v = find_vehicle_by_id(vehicles, vehicle_id);
    if (v == NULL) {
        strcpy(al.outcome, "DENIED");
        strcpy(al.reason, "Vehicle NOT FOUND");
        insert_access_log(logs, gates, vehicles, users, al);
        return 0;
    }

    User *u = find_user_by_id(users, v->user_id);
    if (u == NULL) {
        strcpy(al.outcome, "DENIED");
        strcpy(al.reason, "User NOT FOUND");
        insert_access_log(logs, gates, vehicles, users, al);
        return 0;
    }
    al.user_id = u->user_id;//user who attempt entry

    Gate *g = find_gate_by_id(gates, gate_id);
    if (g == NULL || g->enabled == 0) {
        strcpy(al.outcome, "DENIED");
        strcpy(al.reason, "Gate invalid/disabled");
        insert_access_log(logs, gates, vehicles, users, al);
        return 0;
    }

    if (find_gate_zone(gateZones, v->ZoneID, gate_id) == NULL) {
        strcpy(al.outcome, "DENIED");
        strcpy(al.reason, "Gate not assigned to vehicle zone");
        insert_access_log(logs, gates, vehicles, users, al);
        return 0;
    }

//================================================
    PermissionNode *pn = permissions->head;
    while (pn != NULL) {
        if (pn->data.gate_id == gate_id &&
            strcmp(pn->data.action, "deny") == 0) {

            int match = 0;

            if (strcmp(pn->data.target_type, "vehicle") == 0 &&
                pn->data.target_id == vehicle_id)
                match = 1;
            else if (strcmp(pn->data.target_type, "role") == 0 &&
                     pn->data.target_id == u->role_id)
                match = 1;

            if (match) {
                strcpy(al.outcome, "DENIED");
                strcpy(al.reason, "Explicit DENY rule");
                insert_access_log(logs, gates, vehicles, users, al);
                return 0;
            }
        }
        pn = pn->next;
    }


    pn = permissions->head;
    while (pn != NULL) {
        if (pn->data.gate_id == gate_id &&
            strcmp(pn->data.action, "allow") == 0) {

            int match = 0;

            if (strcmp(pn->data.target_type, "vehicle") == 0 &&
                pn->data.target_id == vehicle_id)
                match = 1;
            else if (strcmp(pn->data.target_type, "role") == 0 &&
                     pn->data.target_id == u->role_id)
                match = 1;

            if (match) {
                strcpy(al.outcome, "ALLOWED");
                strcpy(al.reason, "Permission matched");
                insert_access_log(logs, gates, vehicles, users, al);
                return 1;
            }
        }
        pn = pn->next;
    }

    strcpy(al.outcome, "DENIED");
    strcpy(al.reason, "No permission");
    insert_access_log(logs, gates, vehicles, users, al);
    return 0;
}

    int read_int(const char *msg) {
    int x;
    printf("%s", msg);
    scanf("%d", &x);
    return x;
}

void read_str(const char *msg, char *out, int max) {
    printf("%s", msg);
    scanf("%s", out);
}

void save_all(OrganizationTable *orgs, RoleTable *roles, UserTable *users, ZoneTable *zones,
              GateTable *gates, GateZoneTable *gateZones, VehicleTable *vehicles,
              PermissionTable *permissions, AccessLogTable *logs) {
    write_organizations(orgs);
    write_roles(roles);
    write_users(users);
    write_zones(zones);
    write_gates(gates);
    write_gate_zones(gateZones);
    write_vehicles(vehicles);
    write_permissions(permissions);
    write_access_logs(logs);
    printf("Saved to CSV files.\n");
}

void load_all(OrganizationTable *orgs, RoleTable *roles, UserTable *users, ZoneTable *zones,
              GateTable *gates, GateZoneTable *gateZones, VehicleTable *vehicles,
              PermissionTable *permissions, AccessLogTable *logs) {
    read_organizations(orgs);
    read_roles(roles);
    read_users(users);
    read_zones(zones);
    read_gates(gates);
    read_gate_zones(gateZones);
    read_vehicles(vehicles);
    read_permissions(permissions);
    read_access_logs(logs);
    printf("Loaded from CSV files.\n");
}
//It rebuilds all hash indexes from the linked-list tables.
//After loading from CSV the index arrays might be empty
void build_all_indexes(
    OrganizationTable *orgs, RoleTable *roles, UserTable *users, ZoneTable *zones,
    GateTable *gates, GateZoneTable *gateZones, VehicleTable *vehicles,
    PermissionTable *permissions, AccessLogTable *logs
) {
    initDummyIndexItem();

    indexFreeAll(orgIndex);//Clears old index contents so we rebuild from scratch cleanly
    indexFreeAll(roleIndex);
    indexFreeAll(userIndex);
    indexFreeAll(zoneIndex);
    indexFreeAll(gateIndex);
    indexFreeAll(gateZoneIndex);
    indexFreeAll(vehicleIndex);
    indexFreeAll(permissionIndex);
    indexFreeAll(accessLogIndex);
for (OrganizationNode *c = orgs->head; c; c = c->next)
        indexInsert(orgIndex, c->data.OrgID, &c->data);
    //The hash index for organizations
/*
For every organization in the linked list:
take its ID
take its memory address
and store them in the hash index
*/
for (RoleNode *c = roles->head; c; c = c->next)
        indexInsert(roleIndex, c->data.role_id, &c->data);
for (UserNode *c = users->head; c; c = c->next)
        indexInsert(userIndex, c->data.user_id, &c->data);
for (ZoneNode *c = zones->head; c; c = c->next)
        indexInsert(zoneIndex, c->data.ZoneID, &c->data);
for (GateNode *c = gates->head; c; c = c->next)
            indexInsert(gateIndex, c->data.gate_id, &c->data);
for (GateZoneNode *c = gateZones->head; c; c = c->next)
        indexInsert(gateZoneIndex, gateZoneKey(c->data.ZoneID, c->data.gate_id), &c->data);
for (VehicleNode *c = vehicles->head; c; c = c->next)
        indexInsert(vehicleIndex, c->data.vehicle_id, &c->data);
for (PermissionNode *c = permissions->head; c; c = c->next)
        indexInsert(permissionIndex, c->data.permission_id, &c->data);
for (AccessLogNode *c = logs->head; c; c = c->next)
        indexInsert(accessLogIndex, c->data.log_id, &c->data);
}
//===============================================================list function ====================================================================
void list_organizations(OrganizationTable *t){
    OrganizationNode *c=t->head;
    while(c){
        printf("%d | %s | %s\n", c->data.OrgID, c->data.OrgName, c->data.OrgType);
        c=c->next;
    }
}

void list_users(UserTable *t){
    UserNode *c=t->head;
    while(c){
        printf("%d | %s %s | %s | Org:%d | Role:%d\n",
               c->data.user_id, c->data.FirstName, c->data.LastName,
               c->data.email, c->data.OrgID, c->data.role_id);
        c=c->next;
    }
}

void list_roles(RoleTable *t){
    RoleNode *c=t->head;
    while(c){
        printf("%d | %s | %s \n",
               c->data.role_id, c->data.name, c->data.description);
        c=c->next;
    }
}

void list_zones(ZoneTable *t){
    ZoneNode *c=t->head;
    while(c){
        printf("%d | %s | %s\n", c->data.ZoneID, c->data.ZoneName, c->data.description);
        c=c->next;
    }
}

void list_gates(GateTable *t){
    GateNode *c=t->head;
    while(c){
        printf("%d | %s | enabled:%d | %s\n",
               c->data.gate_id, c->data.name, c->data.enabled, c->data.location);
        c=c->next;
    }
}

void list_gate_zones(GateZoneTable *t){
    GateZoneNode *c=t->head;
    while(c){
        printf("Zone:%d <-> Gate:%d\n", c->data.ZoneID, c->data.gate_id);
        c=c->next;
    }
}

void list_vehicles(VehicleTable *t){
    VehicleNode *c=t->head;
    while(c){
        printf("%d | %s | %s %s | Zone:%d | User:%d\n",
               c->data.vehicle_id, c->data.license_plate,
               c->data.make, c->data.model, c->data.ZoneID, c->data.user_id);
        c=c->next;
    }
}

void list_permissions(PermissionTable *t){
    PermissionNode *c=t->head;
    while(c){
        printf("%d | gate:%d | %s:%d | %s \n",
               c->data.permission_id, c->data.gate_id,
               c->data.target_type, c->data.target_id,
               c->data.action);
        c=c->next;
    }
}

void list_access_logs(AccessLogTable *t){
    AccessLogNode *c=t->head;
    while(c){
        printf("%d | %s | %s | %s | gate:%d | veh:%d | user:%d\n",
               c->data.log_id, c->data.timestamp,
               c->data.outcome, c->data.reason,
               c->data.gate_id, c->data.vehicle_id, c->data.user_id);
        c=c->next;
    }
}
void list_all_tables(
    OrganizationTable *orgs, RoleTable *roles, UserTable *users,
    ZoneTable *zones, GateTable *gates, GateZoneTable *gateZones,
    VehicleTable *vehicles, PermissionTable *permissions, AccessLogTable *logs
) {
    printf("\n--- Organizations ---\n");
    list_organizations(orgs);

    printf("\n--- Roles ---\n");
    list_roles(roles);

    printf("\n--- Users ---\n");
    list_users(users);

    printf("\n--- Zones ---\n");
    list_zones(zones);

    printf("\n--- Gates ---\n");
    list_gates(gates);

    printf("\n--- GateZone ---\n");
    list_gate_zones(gateZones);

    printf("\n--- Vehicles ---\n");
    list_vehicles(vehicles);

    printf("\n--- Permissions ---\n");
    list_permissions(permissions);

    printf("\n--- Access Logs ---\n");
    list_access_logs(logs);
}
//=================================================================concurrency control ====================================================
//enum : assigns readable names to integer values
typedef enum { LOCK_NONE = 0, LOCK_S = 1, LOCK_X = 2 } LockMode;
//s : read ,select
//x : insert ,update,delete
static int cc_s_count[CC_MAX_TABLES] = {0};
static int cc_x_holder[CC_MAX_TABLES] = {0};
static pthread_mutex_t cc_table_mtx[CC_MAX_TABLES];// prevent two threads editing it at the same time

LockMode ccNeededMode(int op) {
    if (op == 1) return LOCK_S;
    return LOCK_X;
}
const char* ccTableName(int t) {
    switch (t) {
        case 1: return "Organizations";
        case 2: return "Roles";
        case 3: return "Users";
        case 4: return "Zones";
        case 5: return "Gates";
        case 6: return "GateZone";
        case 7: return "Vehicles";
        case 8: return "Permissions";
        case 9: return "AccessLog";
        default: return "Unknown";
    }
}
void show_cc_menu(){
    printf("\n=== TABLES ===\n");
    printf("1) Organizations\n");
    printf("2) Roles\n");
    printf("3) Users\n");
    printf("4) Zones\n");
    printf("5) Gates\n");
    printf("6) GateZone\n");
    printf("7) Vehicles\n");
    printf("8) Permissions\n");
    printf("9) Access Logs\n");

}
const char* ccOpName(int op) {
    switch (op) {
        case 1: return "select";
        case 2: return "insert";
        case 3: return "update";
        case 4: return "delete";
        default: return "op";
    }
}
//resets lock counters and creates mutexes for all tables.
void ccInitLocks() {
    for (int i = 0; i < CC_MAX_TABLES; i++) {
        cc_s_count[i] = 0;
        cc_x_holder[i] = 0;
        pthread_mutex_init(&cc_table_mtx[i], NULL);
    }
}
//checks if the requested lock is blocked by current locks on the same table
static int ccLockExists_NoWait(int tableId, LockMode need) {
    if (need == LOCK_S) {
        // S allowed when no X
        return (cc_x_holder[tableId] == 1);
    } else {
        // X allowed only when no X(writer) or no S(reader) holders
        return (cc_x_holder[tableId] == 1 || cc_s_count[tableId] > 0);
    }
}
//prevents two threads from writing at the same time, or reading while writing.
void ccAcquire(int userId, int tableId, int op) {
    LockMode need = ccNeededMode(op);
    printf("User %d: %s %s. Started.\n", userId, ccOpName(op), ccTableName(tableId));
    while (1) {
        pthread_mutex_lock(&cc_table_mtx[tableId]);//Lock the mutex so only one thread can check/change lock variables at a time
        if (!ccLockExists_NoWait(tableId, need)) {//if not blocked
            // take the lock
            if (need == LOCK_S) {
                cc_s_count[tableId]++;
                pthread_mutex_unlock(&cc_table_mtx[tableId]);//Release mutex after updating shared state
                printf("User %d: Get S Lock\n", userId);
            } else {
                cc_x_holder[tableId] = 1;
                pthread_mutex_unlock(&cc_table_mtx[tableId]);
                printf("User %d: Get X Lock\n", userId);
            }
            break;
        }
        pthread_mutex_unlock(&cc_table_mtx[tableId]);

        printf("User %d: Wait\n", userId);
        sleep(1);
    }
}
//removes the lock after operation finishes.
void ccRelease(int userId, int tableId, int op) {
    LockMode held = ccNeededMode(op);
    pthread_mutex_lock(&cc_table_mtx[tableId]);

    if (held == LOCK_S) {
        if (cc_s_count[tableId] > 0) cc_s_count[tableId]--;
    } else {
        cc_x_holder[tableId] = 0;
    }
    pthread_mutex_unlock(&cc_table_mtx[tableId]);
    printf("User %d: end. Release lock.\n", userId);
}
//========================================================================cc hardcoded=======================================================
typedef struct {
    int userId;
    int tableId;
    int op;
    OrganizationTable *orgs;
    RoleTable *roles;
    UserTable *users;
    ZoneTable *zones;
    GateTable *gates;
    GateZoneTable *gateZones;
    VehicleTable *vehicles;
    PermissionTable *permissions;
    AccessLogTable *logs;
} CCTask;

static void ccDoOperation(CCTask *t) {
    sleep(1);
    // 1) Organizations
    if (t->tableId == 1) {
        if (t->op == 1) list_organizations(t->orgs);

        else if (t->op == 2) {
            Organization o;
            o.OrgID = t->orgs->next_id++;
            strcpy(o.OrgName, "CC_Org");
            strcpy(o.OrgType, "Test");
            insert_organization(t->orgs, o);
        }
        else if (t->op == 3) {
            update_organization(t->orgs, 1, "CC_Updated", "TypeX");
        }
        else if (t->op == 4) {
            delete_organization(t->orgs, t->users, 1);
        }
        return;
    }
    // 2) Roles
    if (t->tableId == 2) {
        if (t->op == 1) list_roles(t->roles);

        else if (t->op == 2) {
            Role r;
            r.role_id = t->roles->next_id++;
            strcpy(r.name, "CC_Role");
            strcpy(r.description, "Role from CC");
            insert_role(t->roles, r);
        }
        else if (t->op == 3) {
            update_role(t->roles, 1, "CC_Role_Upd", "Updated by CC");
        }
        else if (t->op == 4) {
            delete_role(t->roles, t->users, t->permissions, 1);
        }
        return;
    }
    // 3) Users
    if (t->tableId == 3) {
        if (t->op == 1) list_users(t->users);
        else if (t->op == 2) {
            User u;
            u.user_id = t->users->next_id++;
            strcpy(u.FirstName, "CC");
            strcpy(u.LastName, "User");
            strcpy(u.email, "cc@u.com");
            strcpy(u.Phone, "0790000000");
            strcpy(u.status, "active");
            u.OrgID = 1;      // FK
            u.role_id = 1;    // FK
            insert_user(t->users, t->orgs, t->roles, u);
        }
        else if (t->op == 3) {
            update_user(t->users, t->orgs, t->roles, 1,
                        "CCFN", "CCLN", "cc2@u.com", "0791111111", "active", 1, 1);
        }
        else if (t->op == 4) {
            delete_user(t->users, t->vehicles, t->logs, 1);
        }
        return;
    }
    // 4) Zones
    if (t->tableId == 4) {
        if (t->op == 1) list_zones(t->zones);
        else if (t->op == 2) {
            Zone z;
            z.ZoneID = t->zones->next_id++;
            strcpy(z.ZoneName, "CC_Zone");
            strcpy(z.description, "Zone from CC");
            insert_zone(t->zones, z);
        }
        else if (t->op == 3) {
            update_zone(t->zones, 1, "CC_Zone_Upd", "Updated by CC");
        }
        else if (t->op == 4) {
            delete_zone(t->zones, t->vehicles, t->gateZones, 1);
        }
        return;
    }
    // 5) Gates
    if (t->tableId == 5) {
        if (t->op == 1) list_gates(t->gates);
        else if (t->op == 2) {
            Gate g;
            g.gate_id = t->gates->next_id++;
            strcpy(g.name, "CC_Gate");
            g.enabled = 1;
            strcpy(g.location, "CC_Location");
            insert_gate(t->gates, g);
        }
        else if (t->op == 3) {
            update_gate(t->gates, 1, "CC_Gate_Upd", 1, "CC_Loc_Upd");
        }
        else if (t->op == 4) {
            delete_gate(t->gates, t->gateZones, t->permissions, t->logs, 1);
        }
        return;
    }
    // 6) GateZone
    if (t->tableId == 6) {
        if (t->op == 1) list_gate_zones(t->gateZones);
        else if (t->op == 2) {
            GateZone gz;
            gz.ZoneID = 1;     // FK
            gz.gate_id = 1;    // FK
            insert_gate_zone(t->gateZones, t->zones, t->gates, gz);
        }
        else if (t->op == 3) {
            update_gate_zone(t->gateZones, t->zones, t->gates, 1, 1, 1, 2);
        }
        else if (t->op == 4) {
            delete_gatezone(t->gateZones, 1, 1);
        }
        return;
    }
    // 7) Vehicles
    if (t->tableId == 7) {
        if (t->op == 1) list_vehicles(t->vehicles);
        else if (t->op == 2) {
            Vehicle v;
            v.vehicle_id = t->vehicles->next_id++;
            strcpy(v.license_plate, "CC-111");
            strcpy(v.make, "Make");
            strcpy(v.model, "Model");
            strcpy(v.color, "Color");
            strcpy(v.status, "active");
            v.ZoneID = 1;   // FK
            v.user_id = 1;  // FK
            insert_vehicle(t->vehicles, t->users, t->zones, v);
        }
        else if (t->op == 3) {
            update_vehicle(t->vehicles, t->users, t->zones, 1,
                           "CC-222", "Make2", "Model2", "Color2", "active", 1, 1);
        }
        else if (t->op == 4) {
            delete_vehicle(t->vehicles, t->permissions, t->logs, 1);
        }
        return;
    }
    // 8) Permissions
    if (t->tableId == 8) {
        if (t->op == 1) list_permissions(t->permissions);
        else if (t->op == 2) {
            Permission p;
            p.permission_id = t->permissions->next_id++;
            strcpy(p.target_type, "role");  // "role" or "vehicle"
            p.target_id = 1;
            strcpy(p.action, "allow");       // allow/deny
            strcpy(p.created_by, "CC");
            p.gate_id = 1;
            p.role_id = 1;
            insert_permission(t->permissions, t->gates, t->roles, t->vehicles, p);
        }
        else if (t->op == 3) {
            update_permission(t->permissions, t->gates, t->roles, t->vehicles,
                              1, "role", 1, "deny", "CC_Upd", 1, 1);
        }
        else if (t->op == 4) {
            delete_permission(t->permissions, 1);
        }
        return;
    }
    // 9) Access Logs
    if (t->tableId == 9) {
        if (t->op == 1) list_access_logs(t->logs);
        else if (t->op == 2) {
            int newLogId = t->logs->next_id++;
            register_gate_entry(t->vehicles, t->users, t->gates, t->gateZones,
                                t->permissions, t->logs, newLogId, 1, 1);
        }
        else if (t->op == 3) {
            update_access_log(t->logs, t->gates, t->vehicles, t->users,
                              1, "DENIED", "CC updated", 1, 1, 1);
        }
        else if (t->op == 4) {
            delete_access_log(t->logs, 1);
        }
        return;
    }
}
//this is what every thread run
static void* ccThreadEntry(void *arg) {
    CCTask *t = (CCTask*)arg;
//pack everything into a struct (CCTask) and pass its address
    ccAcquire(t->userId, t->tableId, t->op);//acquire lock before doing anything
    ccDoOperation(t);//do operation
    ccRelease(t->userId, t->tableId, t->op);// release lock

    return NULL;
}
//========================================================================cc menu runner==========================================================
void run_cc_test(
    OrganizationTable *orgs, RoleTable *roles, UserTable *users, ZoneTable *zones,
    GateTable *gates, GateZoneTable *gateZones, VehicleTable *vehicles,
    PermissionTable *permissions, AccessLogTable *logs
) {
    int n = read_int("How many users ? ");
    if (n <= 0) return;
    CCTask *tasks = (CCTask*)malloc(sizeof(CCTask) * n);//Allocate an array of tasks one task per user/thread
    pthread_t *ths  = (pthread_t*)malloc(sizeof(pthread_t) * n);
    for (int i = 0; i < n; i++) {
        printf("\n--- User %d ---\n", i + 1);
        show_cc_menu();
        int tableId = read_int("Choose table (1..9): ");
        int op = read_int("Choose operation (1 select, 2 insert, 3 update, 4 delete): ");
        if (tableId < 1) tableId = 1;
        if (tableId > 9) tableId = 9;
        if (op < 1) op = 1;
        if (op > 4) op = 4;
        tasks[i].userId = i + 1;
        tasks[i].tableId = tableId;
        tasks[i].op = op;
        //Give this thread access to all tables.
        tasks[i].orgs = orgs;
        tasks[i].roles = roles;
        tasks[i].users = users;
        tasks[i].zones = zones;
        tasks[i].gates = gates;
        tasks[i].gateZones = gateZones;
        tasks[i].vehicles = vehicles;
        tasks[i].permissions = permissions;
        tasks[i].logs = logs;
    }
    printf("\n.................\nSimulation starting:\n\n");
    for (int i = 0; i < n; i++)
        pthread_create(&ths[i], NULL, ccThreadEntry, &tasks[i]);
    for (int i = 0; i < n; i++)
        pthread_join(ths[i], NULL);//Wait until all threads finish

    free(tasks);
    free(ths);

}
void show_table_menu() {
    printf("\n=== TABLES ===\n");
    printf("1) Organizations\n");
    printf("2) Roles\n");
    printf("3) Users\n");
    printf("4) Zones\n");
    printf("5) Gates\n");
    printf("6) GateZone\n");
    printf("7) Vehicles\n");
    printf("8) Permissions\n");
    printf("9) Access Logs\n");
    printf("10)List All\n");
    printf("11) Concurrency Control Test\n");
    printf("0) Exit\n");
}

void show_ops_menu() {
    printf("\n--- Operations ---\n");
    printf("1) Insert\n");
    printf("2) Update\n");
    printf("3) Delete\n");
    printf("4) Search by ID\n");
    printf("5) List all rows\n");
    printf("0) Back\n");
}
void show_ops_menu_access_log() {
    printf("\n--- Operations ---\n");
    printf("1) Register Gate Entry (insert log)\n");
    printf("2) Update\n");
    printf("3) Delete\n");
    printf("4) Search by ID\n");
    printf("5) List all rows\n");
    printf("0) Back\n");
}

void table_operations_menu(
    OrganizationTable *orgs, RoleTable *roles, UserTable *users, ZoneTable *zones,
    GateTable *gates, GateZoneTable *gateZones, VehicleTable *vehicles,
    PermissionTable *permissions, AccessLogTable *logs
) {
    while (1) {
        show_table_menu();
        int t = read_int("Choose table: ");
        if (t == 0) return;

        if (t == 10) {
            list_all_tables(orgs, roles, users, zones,
                            gates, gateZones, vehicles,
                            permissions, logs);
            continue;
        }
      if (t == 11) {
    run_cc_test(orgs, roles, users, zones, gates, gateZones, vehicles, permissions, logs);
    continue;
}


        while (1) {
            if (t == 9) show_ops_menu_access_log();
            else if (t >= 1 && t <= 8) show_ops_menu();
            else { printf("Invalid table.\n"); break; }

            int op = read_int("Choose operation: ");
            if (op == 0) break;

            // ===================== TABLE 1: Organizations =====================
            if (t == 1) {
                if (op == 1) {
                    Organization o;
                    o.OrgID = orgs->next_id++;
                    read_str("OrgName: ", o.OrgName, sizeof(o.OrgName));
                    read_str("OrgType: ", o.OrgType, sizeof(o.OrgType));

                    int ok = insert_organization(orgs, o);
                    if (!ok) orgs->next_id--;
                    else {
                        build_all_indexes(orgs, roles, users, zones, gates, gateZones, vehicles, permissions, logs);
                        writeLog("Organization", "OrgID",   o.OrgID, "NULL", "NEW",     "INSERT");
                        writeLog("Organization", "OrgName", o.OrgID, "NULL", o.OrgName, "INSERT");
                        writeLog("Organization", "OrgType", o.OrgID, "NULL", o.OrgType, "INSERT");
                    }
                }
                else if (op == 2) {
                    int id = read_int("OrgID: ");
                    Organization *oldPtr = find_organization_by_id(orgs, id);
                    if (!oldPtr) {
                        printf("Update failed: OrgID not found\n");
                    } else {
                        Organization oldO = *oldPtr;
                        char nm[51], tp[31];
                        read_str("New OrgName: ", nm, sizeof(nm));
                        read_str("New OrgType: ", tp, sizeof(tp));

                        int ok = update_organization(orgs, id, nm, tp);
                        if (ok) {
                            build_all_indexes(orgs, roles, users, zones, gates, gateZones, vehicles, permissions, logs);
                            writeLog("Organization", "OrgName", id, oldO.OrgName, nm, "UPDATE");
                            writeLog("Organization", "OrgType", id, oldO.OrgType, tp, "UPDATE");
                        }
                    }
                }
                else if (op == 3) {
                    int id = read_int("OrgID: ");
                    int ok = delete_organization(orgs, users, id);
                    if (ok) {
                        build_all_indexes(orgs, roles, users, zones, gates, gateZones, vehicles, permissions, logs);
                        writeLog("Organization", "OrgID", id, "OLD", "NULL", "DELETE");
                    }
                }
                else if (op == 4) {
                    int id = read_int("OrgID: ");
                    Organization *o = find_organization_by_id(orgs, id);
                    if (!o) printf("Not found.\n");
                    else printf("%d | %s | %s\n", o->OrgID, o->OrgName, o->OrgType);
                }
                else if (op == 5) list_organizations(orgs);
            }

            // ===================== TABLE 2: Roles =====================
            else if (t == 2) {
                if (op == 1) {
                    Role r;
                    r.role_id = roles->next_id++;
                    read_str("name: ", r.name, sizeof(r.name));
                    read_str("description: ", r.description, sizeof(r.description));

                    int ok = insert_role(roles, r);
                    if (!ok) roles->next_id--;
                    else {
                        build_all_indexes(orgs, roles, users, zones, gates, gateZones, vehicles, permissions, logs);
                        writeLog("Role", "role_id",     r.role_id, "NULL", "NEW",        "INSERT");
                        writeLog("Role", "name",        r.role_id, "NULL", r.name,       "INSERT");
                        writeLog("Role", "description", r.role_id, "NULL", r.description,"INSERT");
                    }
                }
                else if (op == 2) {
                    int id = read_int("role_id: ");
                    Role *oldPtr = find_role_by_id(roles, id);
                    if (!oldPtr) {
                        printf("Update failed: role_id not found\n");
                    } else {
                        Role oldR = *oldPtr;
                        char nm[51], ds[201];
                        read_str("New name: ", nm, sizeof(nm));
                        read_str("New description: ", ds, sizeof(ds));

                        int ok = update_role(roles, id, nm, ds);
                        if (ok) {
                            build_all_indexes(orgs, roles, users, zones, gates, gateZones, vehicles, permissions, logs);
                            writeLog("Role", "name",        id, oldR.name, nm, "UPDATE");
                            writeLog("Role", "description", id, oldR.description, ds, "UPDATE");
                        }
                    }
                }
                else if (op == 3) {
                    int id = read_int("role_id: ");
                    int ok = delete_role(roles, users, permissions, id);
                    if (ok) {
                        build_all_indexes(orgs, roles, users, zones, gates, gateZones, vehicles, permissions, logs);
                        writeLog("Role", "role_id", id, "OLD", "NULL", "DELETE");
                    }
                }
                else if (op == 4) {
                    int id = read_int("role_id: ");
                    Role *r = find_role_by_id(roles, id);
                    if (!r) printf("Not found.\n");
                    else printf("%d | %s | %s\n", r->role_id, r->name, r->description);
                }
                else if (op == 5) list_roles(roles);
            }

            // ===================== TABLE 3: Users =====================
            else if (t == 3) {
                if (op == 1) {
                    User u;
                    u.user_id = users->next_id++;
                    read_str("FirstName: ", u.FirstName, sizeof(u.FirstName));
                    read_str("LastName: ",  u.LastName,  sizeof(u.LastName));
                    read_str("email: ",     u.email,     sizeof(u.email));
                    read_str("Phone: ",     u.Phone,     sizeof(u.Phone));
                    read_str("status: ",    u.status,    sizeof(u.status));
                    u.OrgID   = read_int("OrgID (FK): ");
                    u.role_id = read_int("role_id (FK): ");

                    int ok = insert_user(users, orgs, roles, u);
                    if (!ok) users->next_id--;
                    else {
                        build_all_indexes(orgs, roles, users, zones, gates, gateZones, vehicles, permissions, logs);
                        writeLog("Users", "user_id",   u.user_id, "NULL", "NEW",       "INSERT");
                        writeLog("Users", "FirstName", u.user_id, "NULL", u.FirstName, "INSERT");
                        writeLog("Users", "LastName",  u.user_id, "NULL", u.LastName,  "INSERT");
                        writeLog("Users", "email",     u.user_id, "NULL", u.email,     "INSERT");
                        writeLog("Users", "Phone",     u.user_id, "NULL", u.Phone,     "INSERT");
                        writeLog("Users", "status",    u.user_id, "NULL", u.status,    "INSERT");
                    }
                }
                else if (op == 2) {
                    int id = read_int("user_id: ");
                    User *oldPtr = find_user_by_id(users, id);
                    if (!oldPtr) {
                        printf("Update failed: user_id not found\n");
                    } else {
                        User oldU = *oldPtr;
                        char fn[51], ln[101], em[51], ph[21], st[11];
                        int newOrg, newRole;

                        read_str("New FirstName: ", fn, sizeof(fn));
                        read_str("New LastName: ",  ln, sizeof(ln));
                        read_str("New email: ",     em, sizeof(em));
                        read_str("New Phone: ",     ph, sizeof(ph));
                        read_str("New status: ",    st, sizeof(st));
                        newOrg  = read_int("New OrgID (FK): ");
                        newRole = read_int("New role_id (FK): ");

                        int ok = update_user(users, orgs, roles, id, fn, ln, em, ph, st, newOrg, newRole);
                        if (ok) {
                            build_all_indexes(orgs, roles, users, zones, gates, gateZones, vehicles, permissions, logs);
                            writeLog("Users", "FirstName", id, oldU.FirstName, fn, "UPDATE");
                            writeLog("Users", "LastName",  id, oldU.LastName,  ln, "UPDATE");
                            writeLog("Users", "email",     id, oldU.email,     em, "UPDATE");
                            writeLog("Users", "Phone",     id, oldU.Phone,     ph, "UPDATE");
                            writeLog("Users", "status",    id, oldU.status,    st, "UPDATE");
                            // OrgID/role_id changes (optional logs if you want):
                            // writeLog("Users","OrgID",  id, "OLD","NEW","UPDATE");
                            // writeLog("Users","role_id",id, "OLD","NEW","UPDATE");
                        }
                    }
                }
                else if (op == 3) {
                    int id = read_int("user_id: ");
                    int ok = delete_user(users, vehicles, logs, id);
                    if (ok) {
                        build_all_indexes(orgs, roles, users, zones, gates, gateZones, vehicles, permissions, logs);
                        writeLog("Users", "user_id", id, "OLD", "NULL", "DELETE");
                    }
                }
                else if (op == 4) {
                    int id = read_int("user_id: ");
                    User *u = find_user_by_id(users, id);
                    if (!u) printf("Not found.\n");
                    else printf("%d | %s %s | %s | Org:%d | Role:%d\n",
                                u->user_id, u->FirstName, u->LastName, u->email, u->OrgID, u->role_id);
                }
                else if (op == 5) list_users(users);
            }

            // ===================== TABLE 4: Zones =====================
            else if (t == 4) {
                if (op == 1) {
                    Zone z;
                    z.ZoneID = zones->next_id++;
                    read_str("ZoneName: ", z.ZoneName, sizeof(z.ZoneName));
                    read_str("description: ", z.description, sizeof(z.description));

                    int ok = insert_zone(zones, z);
                    if (!ok) zones->next_id--;
                    else {
                        build_all_indexes(orgs, roles, users, zones, gates, gateZones, vehicles, permissions, logs);
                        writeLog("Zone", "ZoneID",      z.ZoneID, "NULL", "NEW",        "INSERT");
                        writeLog("Zone", "ZoneName",    z.ZoneID, "NULL", z.ZoneName,   "INSERT");
                        writeLog("Zone", "description", z.ZoneID, "NULL", z.description,"INSERT");
                    }
                }
                else if (op == 2) {
                    int id = read_int("ZoneID: ");
                    Zone *oldPtr = find_zone_by_id(zones, id);
                    if (!oldPtr) {
                        printf("Update failed: ZoneID not found\n");
                    } else {
                        Zone oldZ = *oldPtr;
                        char nm[51], ds[256];
                        read_str("New ZoneName: ", nm, sizeof(nm));
                        read_str("New description: ", ds, sizeof(ds));

                        int ok = update_zone(zones, id, nm, ds);
                        if (ok) {
                            build_all_indexes(orgs, roles, users, zones, gates, gateZones, vehicles, permissions, logs);
                            writeLog("Zone", "ZoneName",    id, oldZ.ZoneName, nm, "UPDATE");
                            writeLog("Zone", "description", id, oldZ.description, ds, "UPDATE");
                        }
                    }
                }
                else if (op == 3) {
                    int id = read_int("ZoneID: ");
                    int ok = delete_zone(zones, vehicles, gateZones, id);
                    if (ok) {
                        build_all_indexes(orgs, roles, users, zones, gates, gateZones, vehicles, permissions, logs);
                        writeLog("Zone", "ZoneID", id, "OLD", "NULL", "DELETE");
                    }
                }
                else if (op == 4) {
                    int id = read_int("ZoneID: ");
                    Zone *z = find_zone_by_id(zones, id);
                    if (!z) printf("Not found.\n");
                    else printf("%d | %s | %s\n", z->ZoneID, z->ZoneName, z->description);
                }
                else if (op == 5) list_zones(zones);
            }

            // ===================== TABLE 5: Gates =====================
            else if (t == 5) {
                if (op == 1) {
                    Gate g;
                    g.gate_id = gates->next_id++;
                    read_str("name: ", g.name, sizeof(g.name));
                    g.enabled = read_int("enabled (1/0): ");
                    read_str("location: ", g.location, sizeof(g.location));

                    int ok = insert_gate(gates, g);
                    if (!ok) gates->next_id--;
                    else {
                        build_all_indexes(orgs, roles, users, zones, gates, gateZones, vehicles, permissions, logs);
                        writeLog("Gates", "gate_id",  g.gate_id, "NULL", "NEW", "INSERT");
                        writeLog("Gates", "name",     g.gate_id, "NULL", g.name,"INSERT");
                        writeLog("Gates", "enabled",  g.gate_id, "NULL", (g.enabled ? "1" : "0"), "INSERT");
                        writeLog("Gates", "location", g.gate_id, "NULL", g.location, "INSERT");
                    }
                }
                else if (op == 2) {
                    int id = read_int("gate_id: ");
                    Gate *oldPtr = find_gate_by_id(gates, id);
                    if (!oldPtr) {
                        printf("Update failed: gate_id not found\n");
                    } else {
                        Gate oldG = *oldPtr;
                        char nm[101], loc[256];
                        int en;
                        read_str("New name: ", nm, sizeof(nm));
                        en = read_int("New enabled (1/0): ");
                        read_str("New location: ", loc, sizeof(loc));

                        int ok = update_gate(gates, id, nm, en, loc);
                        if (ok) {
                            build_all_indexes(orgs, roles, users, zones, gates, gateZones, vehicles, permissions, logs);
                            writeLog("Gates", "name",     id, oldG.name, nm, "UPDATE");
                            writeLog("Gates", "enabled",  id, (oldG.enabled ? "1" : "0"), (en ? "1" : "0"), "UPDATE");
                            writeLog("Gates", "location", id, oldG.location, loc, "UPDATE");
                        }
                    }
                }
                else if (op == 3) {
                    int id = read_int("gate_id: ");
                    int ok = delete_gate(gates, gateZones, permissions, logs, id);
                    if (ok) {
                        build_all_indexes(orgs, roles, users, zones, gates, gateZones, vehicles, permissions, logs);
                        writeLog("Gates", "gate_id", id, "OLD", "NULL", "DELETE");
                    }
                }
                else if (op == 4) {
                    int id = read_int("gate_id: ");
                    Gate *g = find_gate_by_id(gates, id);
                    if (!g) printf("Not found.\n");
                    else printf("%d | %s | enabled:%d | %s\n", g->gate_id, g->name, g->enabled, g->location);
                }
                else if (op == 5) list_gates(gates);
            }

            // ===================== TABLE 6: GateZone =====================
            else if (t == 6) {
                if (op == 1) {
                    GateZone gz;
                    gz.ZoneID  = read_int("ZoneID (FK): ");
                    gz.gate_id = read_int("gate_id (FK): ");

                    int ok = insert_gate_zone(gateZones, zones, gates, gz);
                    if (ok) {
                        build_all_indexes(orgs, roles, users, zones, gates, gateZones, vehicles, permissions, logs);
                        writeLog("GateZone", "ZoneID",  gz.ZoneID, "NULL", "NEW", "INSERT");
                        writeLog("GateZone", "gate_id", gz.ZoneID, "NULL", "NEW", "INSERT");
                    }
                }
                else if (op == 2) {
                    int oz = read_int("Old ZoneID: ");
                    int og = read_int("Old gate_id: ");
                    GateZone *oldPtr = find_gate_zone(gateZones, oz, og);
                    if (!oldPtr) {
                        printf("Update failed: GateZone not found\n");
                    } else {
                        int nz = read_int("New ZoneID: ");
                        int ng = read_int("New gate_id: ");

                        int ok = update_gate_zone(gateZones, zones, gates, oz, og, nz, ng);
                        if (ok) {
                            build_all_indexes(orgs, roles, users, zones, gates, gateZones, vehicles, permissions, logs);
                            writeLog("GateZone", "ZoneID",  oz, "OLD", "NEW", "UPDATE");
                            writeLog("GateZone", "gate_id", oz, "OLD", "NEW", "UPDATE");
                        }
                    }
                }
                else if (op == 3) {
                    int z = read_int("ZoneID: ");
                    int g = read_int("gate_id: ");
                    int ok = delete_gatezone(gateZones, z, g);
                    if (ok) {
                        build_all_indexes(orgs, roles, users, zones, gates, gateZones, vehicles, permissions, logs);
                        writeLog("GateZone", "ZoneID", z, "OLD", "NULL", "DELETE");
                    }
                }
                else if (op == 4) {
                    int z = read_int("ZoneID: ");
                    int g = read_int("gate_id: ");
                    GateZone *gz = find_gate_zone(gateZones, z, g);
                    if (!gz) printf("Not found.\n");
                    else printf("Zone:%d <-> Gate:%d\n", gz->ZoneID, gz->gate_id);
                }
                else if (op == 5) list_gate_zones(gateZones);
            }

            // ===================== TABLE 7: Vehicles =====================
            else if (t == 7) {
                if (op == 1) {
                    Vehicle v;
                    v.vehicle_id = vehicles->next_id++;
                    read_str("license_plate: ", v.license_plate, sizeof(v.license_plate));
                    read_str("make: ", v.make, sizeof(v.make));
                    read_str("model: ", v.model, sizeof(v.model));
                    read_str("color: ", v.color, sizeof(v.color));
                    read_str("status: ", v.status, sizeof(v.status));
                    v.ZoneID  = read_int("ZoneID (FK): ");
                    v.user_id = read_int("user_id (FK): ");

                    int ok = insert_vehicle(vehicles, users, zones, v);
                    if (!ok) vehicles->next_id--;
                    else {
                        build_all_indexes(orgs, roles, users, zones, gates, gateZones, vehicles, permissions, logs);
                        writeLog("Vehicles", "vehicle_id",     v.vehicle_id, "NULL", "NEW", "INSERT");
                        writeLog("Vehicles", "license_plate",  v.vehicle_id, "NULL", v.license_plate, "INSERT");
                        writeLog("Vehicles", "make",           v.vehicle_id, "NULL", v.make, "INSERT");
                        writeLog("Vehicles", "model",          v.vehicle_id, "NULL", v.model, "INSERT");
                        writeLog("Vehicles", "color",          v.vehicle_id, "NULL", v.color, "INSERT");
                        writeLog("Vehicles", "status",         v.vehicle_id, "NULL", v.status, "INSERT");
                    }
                }
                else if (op == 2) {
                    int id = read_int("vehicle_id: ");
                    Vehicle *oldPtr = find_vehicle_by_id(vehicles, id);
                    if (!oldPtr) {
                        printf("Update failed: vehicle_id not found\n");
                    } else {
                        Vehicle oldV = *oldPtr;
                char pl[21], mk[51], md[51], cl[31], st[11];
                        int zid, uid;
            read_str("New plate: ", pl, sizeof(pl));
            read_str("New make: ",  mk, sizeof(mk));
            read_str("New model: ", md, sizeof(md));
        read_str("New color: ", cl, sizeof(cl));
        read_str("New status: ",st, sizeof(st));
        zid = read_int("New ZoneID (FK): ");
                uid = read_int("New user_id (FK): ");
        int ok = update_vehicle(vehicles, users, zones, id, pl, mk, md, cl, st, zid, uid);
                        if (ok) {
            build_all_indexes(orgs, roles, users, zones, gates, gateZones, vehicles, permissions, logs);
    writeLog("Vehicles", "license_plate", id, oldV.license_plate, pl, "UPDATE");
    writeLog("Vehicles", "make",          id, oldV.make, mk, "UPDATE");
            writeLog("Vehicles", "model",         id, oldV.model, md, "UPDATE");
    writeLog("Vehicles", "color",         id, oldV.color, cl, "UPDATE");
writeLog("Vehicles", "status",        id, oldV.status, st, "UPDATE");
}
    }
}
            else if (op == 3) {
int id = read_int("vehicle_id: ");
        int ok = delete_vehicle(vehicles, permissions, logs, id);
    if (ok) {
    build_all_indexes(orgs, roles, users, zones, gates, gateZones, vehicles, permissions, logs);
    writeLog("Vehicles", "vehicle_id", id, "OLD", "NULL", "DELETE");
        }
    }
        else if (op == 4) {
            int id = read_int("vehicle_id: ");
        Vehicle *v = find_vehicle_by_id(vehicles, id);
    if (!v) printf("Not found.\n");
                    else printf("%d | %s | %s %s | Zone:%d | User:%d\n",
v->vehicle_id, v->license_plate, v->make, v->model, v->ZoneID, v->user_id);
                }
    else if (op == 5) list_vehicles(vehicles);
            }
            // ===================== TABLE 8: Permissions =====================
    else if (t == 8) {
        if (op == 1) {
    Permission p;
            p.permission_id = permissions->next_id++;
        read_str("target_type (role/vehicle): ", p.target_type, sizeof(p.target_type));
            p.target_id = read_int("target_id (number): ");
        read_str("action (allow/deny): ", p.action, sizeof(p.action));
            read_str("created_by: ", p.created_by, sizeof(p.created_by));
    p.gate_id = read_int("gate_id (FK): ");
    p.role_id = read_int("role_id (FK, must exist): ");
                    int ok = insert_permission(permissions, gates, roles, vehicles, p);
            if (!ok) permissions->next_id--;
        else {
        build_all_indexes(orgs, roles, users, zones, gates, gateZones, vehicles, permissions, logs);
writeLog("Permissions", "permission_id", p.permission_id, "NULL", "NEW", "INSERT");
writeLog("Permissions", "target_type",    p.permission_id, "NULL", p.target_type, "INSERT");
writeLog("Permissions", "action",         p.permission_id, "NULL", p.action, "INSERT");
writeLog("Permissions", "created_by",     p.permission_id, "NULL", p.created_by, "INSERT");
            }
                }
    else if (op == 2) {
            int pid = read_int("permission_id: ");
Permission *oldPtr = find_permission_by_id(permissions, pid);
                if (!oldPtr) {
printf("Update failed: permission_id not found\n");
                } else {
        Permission oldP = *oldPtr;
        char tt[21], ac[11], cb[37];
int tid, gid, rid;
            read_str("New target_type (role/vehicle): ", tt, sizeof(tt));
            tid = read_int("New target_id: ");
read_str("New action (allow/deny): ", ac, sizeof(ac));
            read_str("New created_by: ", cb, sizeof(cb));
            gid = read_int("New gate_id: ");
        rid = read_int("New role_id (FK, must exist): ");
                        int ok = update_permission(permissions, gates, roles, vehicles, pid, tt, tid, ac, cb, gid, rid);
                    if (ok) {
build_all_indexes(orgs, roles, users, zones, gates, gateZones, vehicles, permissions, logs);
                writeLog("Permissions", "target_type", pid, oldP.target_type, tt, "UPDATE");
                        writeLog("Permissions", "action",      pid, oldP.action, ac, "UPDATE");
writeLog("Permissions", "created_by",  pid, oldP.created_by, cb, "UPDATE");
    }
    }
}
            else if (op == 3) {
               int pid = read_int("permission_id: ");
            int ok = delete_permission(permissions, pid);
        if (ok) {
                build_all_indexes(orgs, roles, users, zones, gates, gateZones, vehicles, permissions, logs);
            writeLog("Permissions", "permission_id", pid, "OLD", "NULL", "DELETE");
        }
    }
    else if (op == 4) {
        int pid = read_int("permission_id: ");
        Permission *p = find_permission_by_id(permissions, pid);
                if (!p) printf("Not found.\n");
        else printf("%d | gate:%d | %s:%d | %s | by:%s | role_id:%d\n",
    p->permission_id, p->gate_id, p->target_type, p->target_id,
        p->action, p->created_by, p->role_id);
        }
            else if (op == 5) list_permissions(permissions);
            }
        else if (t == 9) {
                if (op == 1) {
                    int vid = read_int("vehicle_id: ");
    int gid = read_int("gate_id: ");
                    int newLogId = logs->next_id++;
            int okAccess = register_gate_entry(vehicles, users, gates, gateZones, permissions, logs, newLogId, vid, gid);
                    printf("Access: %s\n", okAccess ? "ALLOWED" : "DENIED");
            AccessLog *a = find_access_log_by_id(logs, newLogId);
        if (a) {
                build_all_indexes(orgs, roles, users, zones, gates, gateZones, vehicles, permissions, logs);
                writeLog("Access_Log", "log_id",    newLogId, "NULL", "NEW",      "INSERT");
                writeLog("Access_Log", "timestamp", newLogId, "NULL", a->timestamp,"INSERT");
                writeLog("Access_Log", "outcome",   newLogId, "NULL", a->outcome, "INSERT");
                writeLog("Access_Log", "reason",    newLogId, "NULL", a->reason,  "INSERT");
            } else {
                    logs->next_id--;
                }
        }
    else if (op == 2) {
                    int id = read_int("log_id: ");
                AccessLog *oldPtr = find_access_log_by_id(logs, id);
        if (!oldPtr) {
                    printf("Update failed: log_id not found\n");
                    } else {
    AccessLog oldA = *oldPtr;
                char out[21], rs[256];
        int gid, vid, uid;
                read_str("New outcome: ", out, sizeof(out));
            read_str("New reason: ",  rs, sizeof(rs));
            gid = read_int("New gate_id : ");
       vid = read_int("New vehicle_id : ");
                    uid = read_int("New user_id : ");
                int ok = update_access_log(logs, gates, vehicles, users, id, out, rs, gid, vid, uid);
                if (ok) {
                build_all_indexes(orgs, roles, users, zones, gates, gateZones, vehicles, permissions, logs);
                writeLog("Access_Log", "outcome", id, oldA.outcome, out, "UPDATE");
    writeLog("Access_Log", "reason",  id, oldA.reason,  rs,  "UPDATE");
    }
    }
    }
            else if (op == 3) {
        int id = read_int("log_id: ");
    int ok = delete_access_log(logs, id);
        if (ok) {
        build_all_indexes(orgs, roles, users, zones, gates, gateZones, vehicles, permissions, logs);
            writeLog("Access_Log", "log_id", id, "OLD", "NULL", "DELETE");
    }
        }
                else if (op == 4) {
                    int id = read_int("log_id: ");
            AccessLog *a = find_access_log_by_id(logs, id);
                    if (!a) printf("Not found.\n");
     else printf("%d | %s | %s | %s | gate:%d | veh:%d | user:%d\n",
                a->log_id, a->timestamp, a->outcome, a->reason,
            a->gate_id, a->vehicle_id, a->user_id);
        }
            else if (op == 5) list_access_logs(logs);
        }
    }
}
}

    int main() {
 initDummyIndexItem();
    writeHeaderIfNeeded();
    ccInitLocks();
    printf("Enter your name to start session: ");
    scanf("%s", currentUserName);


   OrganizationTable orgs = {NULL, 0, 1};
    RoleTable roles = {NULL, 0, 1};
    UserTable users = {NULL, 0, 1};
    ZoneTable zones = {NULL, 0, 1};
    GateTable gates = {NULL, 0, 1};
    GateZoneTable gateZones = {NULL, 0};
    VehicleTable vehicles = {NULL, 0, 1};
    PermissionTable permissions = {NULL, 0, 1};
    AccessLogTable logs = {NULL, 0, 1};

    load_all(&orgs,&roles,&users,&zones,&gates,&gateZones,&vehicles,&permissions,&logs);
build_all_indexes(&orgs,&roles,&users,&zones,&gates,&gateZones,&vehicles,&permissions,&logs);
    table_operations_menu(&orgs,&roles,&users,&zones,&gates,&gateZones,&vehicles,&permissions,&logs);
     save_all(&orgs,&roles,&users,&zones,&gates,&gateZones,&vehicles,&permissions,&logs);

    return 0;
}
