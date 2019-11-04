
#ifndef KV_FDW_H_
#define KV_FDW_H_


#include <stdbool.h>
#include <semaphore.h>

#include "postgres.h"
#include "lib/stringinfo.h"
#include "nodes/nodes.h"
#include "access/attnum.h"
#include "utils/relcache.h"


typedef struct SharedMem {
    sem_t agent[2];
    sem_t worker[2];
    bool workerProcessCreated;
    char area[65536];  // assume ~64K for a tuple is enough
} SharedMem;

/* Holds the option values to be used when reading or writing files.
 * To resolve these values, we first check foreign table's options,
 * and if not present, we then fall back to the default values.
 */
typedef struct KVFdwOptions {
    char *filename;
} KVFdwOptions;

/*
 * The scan state is for maintaining state for a scan, either for a
 * SELECT or UPDATE or DELETE.
 *
 * It is set up in BeginForeignScan and stashed in node->fdw_state and
 * subsequently used in IterateForeignScan, EndForeignScan and ReScanForeignScan.
 */
typedef struct TableReadState {
    bool isKeyBased;
    bool done;
    StringInfo key;
} TableReadState;

/*
 * The modify state is for maintaining state of modify operations.
 *
 * It is set up in BeginForeignModify and stashed in
 * rinfo->ri_FdwState and subsequently used in ExecForeignInsert,
 * ExecForeignUpdate, ExecForeignDelete and EndForeignModify.
 */
typedef struct TableWriteState {
    CmdType operation;
    AttrNumber keyJunkNo;
} TableWriteState;

typedef enum FuncName {
    OPEN = 0,
    CLOSE,
    COUNT,
    GETITER,
    DELITER,
    NEXT,
    GET,
    PUT,
    DELETE,
    TERMINATE
} FuncName;


/* Defines */
#define KVKEYJUNK "__key_junk"

#define KVFDWNAME "kv_fdw"

#define BACKFILE "/KVSharedMem"

#define PERMISSION 0777

#define PATHMAXLENGTH 4096


/* Function declarations for extension loading and unloading */
extern void _PG_init(void);

extern void _PG_fini(void);


/* Functions used across files in kv_fdw */
extern KVFdwOptions *KVGetOptions(Oid foreignTableId);

extern void SerializeAttribute(TupleDesc tupleDescriptor,
                               Index index,
                               Datum datum,
                               StringInfo buffer);

extern Datum ShortVarlena(Datum datum, int typeLength, char storage);

extern void *KVStorageThreadFun(void *arg);

extern SharedMem *OpenRequest(Oid relationId, SharedMem *ptr);

extern void CloseRequest(Oid relationId, SharedMem *ptr);

extern uint64 CountRequest(Oid relationId, SharedMem *ptr);

extern void GetIterRequest(Oid relationId, SharedMem *ptr);

extern void DelIterRequest(Oid relationId, SharedMem *ptr);

extern bool NextRequest(Oid relationId,
                        SharedMem *ptr,
                        char** key,
                        uint32* keyLen,
                        char** val,
                        uint32* valLen);

extern bool GetRequest(Oid relationId,
                       SharedMem *ptr,
                       char* key,
                       uint32 keyLen,
                       char** val,
                       uint32* valLen);

extern void PutRequest(Oid relationId,
                       SharedMem *ptr,
                       char* key,
                       uint32 keyLen,
                       char* val,
                       uint32 valLen);

extern void DeleteRequest(Oid relationId,
                          SharedMem *ptr,
                          char* key,
                          uint32 keyLen);


/* global variables */

pthread_t kvStorageThread;


#endif
