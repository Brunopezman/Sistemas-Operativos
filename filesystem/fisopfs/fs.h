#include <sys/stat.h>
#include <time.h>

#define BLOCK_SIZE 4096 //4 kb
#define MAX_LEN_NOMBRE 64
#define CANTIDAD_INODOS 250
#define MAX_INODOS_POR_BLOQUE 16 
#define CANTIDAD_BLOQUES 4000 // 250 * 16
#define TYPE_DIR 0
#define TYPE_REG 1
#define FREE 0
#define USED 1
#define UNASSIGNED_BLOCK -1

#define DEFAULT_FILE_DISK "persistence_file.fisopfs"
extern char *filedisk;

/*Funcionalidades principales*/
int fs_getattr(const char *path, struct stat *st);
int fs_read_dir(const char *path, char *entry_name, int start_index);
int fs_read(const char *path, char *buffer, size_t size, off_t offset);
int fs_write(const char *path, const char *buf, size_t size, off_t offset);
int fs_create(const char *path, mode_t mode);
int fs_mkdir(const char *path, mode_t mode);
int fs_unlink(const char *path);
int fs_rmdir(const char *path);
int fs_truncate(const char *path, off_t size);
int fs_utimens(const char *path, const struct timespec tv[2]); 

/* Persistencia */
void init_default_fs_and_persist(void);
int unmarshall_default_file(FILE *file);
void persist_fs_data(const char *file);

/* Operaciones auxiliares*/
void liberar_bloque(int bloque_idx);
int get_free_block_for_inode(int inode_index, int pos_offset);
int is_in_dir(const char *dir_path, const char *path);
int get_inode_from_path(const char *path);
int new_inode(const char *path, mode_t mode, int type);
int get_free_block_i();
int get_free_inode_i();
int get_last_dentry(const char *path);
bool is_direct_child_of_root(const char *path);

typedef struct block {
    char data[BLOCK_SIZE];
} data_block_t;

typedef struct inode {
    int inum; /* número de inodo */
    char file_name[MAX_LEN_NOMBRE]; /* nombre de archivo */
    int type; /* Indica si el tipo es DIR o FIlE */
    off_t size ; /* Indica el tamaño total del archivo, en bytes */
    mode_t mode ; /* Indica el modo de protección */
    uid_t uid ; /* ID de usuario del propietario */
    gid_t gid ; /* ID de grupo del propietario */ 
    time_t last_access ; /* Hora del último acceso */
    time_t last_modif ; /* Hora de la última modificación */
    time_t last_change_status ; /* Hora del último cambio de estado */
    nlink_t n_links ; /* número de enlaces duros (hardlinks) */
    blkcnt_t n_blocks ; /* Número de bloques asignados */
    int blocks_index[MAX_INODOS_POR_BLOQUE]; /* índices de bloques asignados */
} inode_t;

struct superblock {
	inode_t *inodes; /* Puntero al array de inodos (representa files o dir) */
	int blocks; /* Cantidad total de bloques en el fs*/
	int *inode_bitmap; /* Indica que inodos están en uso (1) o libres (0) */
	int *data_bitmap; /* Indica qué bloques de datos están en uso (1) o libres (0) */
    data_block_t *data_blocks; /* Puntero al array de datablock donde se almacena el contenido de archivos*/
};