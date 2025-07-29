#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include "fs.h"

/* Inicializaciones */
struct superblock superblock;
int inode_bitmap[CANTIDAD_INODOS];
int data_bitmap[CANTIDAD_BLOQUES];
struct inode inodes[CANTIDAD_INODOS];
struct block data_region_blocks[CANTIDAD_BLOQUES];

int fs_getattr(const char *path, struct stat *st) {
    if (strcmp(path, "/") == 0) {
        st->st_uid = 1717;
        st->st_mode = __S_IFDIR | 0755;
        st->st_nlink = 2;
        return 0;
    }

    int inode_index = get_inode_from_path(path);
    if (inode_index < 0) return -ENOENT;

    inode_t *inode = &superblock.inodes[inode_index];

    st->st_dev = 0;
    st->st_ino = inode->inum;
    st->st_mode = (inode->type == TYPE_DIR ? __S_IFDIR : __S_IFREG) | inode->mode;
    st->st_nlink = inode->n_links;
    st->st_uid = inode->uid;
    st->st_gid = 0;
    st->st_size = inode->size;
    st->st_atime = inode->last_access;
    st->st_mtime = inode->last_modif;
    st->st_ctime = inode->last_change_status;
    st->st_blksize = BLOCK_SIZE;
    st->st_blocks = inode->n_blocks;
    return 0;
}

int fs_read_dir(const char *path, char *entry_name, int start_index) {
    for (int i = start_index; i < CANTIDAD_INODOS; i++) {
        if (superblock.inode_bitmap[i] != USED) continue;

        const char *entry_path = superblock.inodes[i].file_name;

        if ((strcmp(path, "/") == 0 && is_direct_child_of_root(entry_path)) ||
            is_in_dir(path, entry_path)) {

            int last_slash = get_last_dentry(entry_path);
            if (last_slash < 0) continue;

            strncpy(entry_name, entry_path + last_slash + 1, MAX_LEN_NOMBRE - 1);
            entry_name[MAX_LEN_NOMBRE - 1] = '\0';
            return i + 1;
        }
    }
    return -1;
}

int fs_read(const char *path, char *buffer, size_t size, off_t offset) {
    int inode_i = get_inode_from_path(path);
    if (inode_i < 0) return -ENOENT;

    inode_t *ino = &superblock.inodes[inode_i];
    if ((size_t)offset >= ino->size) return 0;
    if (offset + size > ino->size) size = ino->size - offset;

    size_t bytes_read = 0;
    size_t remaining = size;
    int block_size = sizeof(struct block);
    int block_index = offset / block_size;
    int block_offset = offset % block_size;

    while (remaining > 0 && block_index < MAX_INODOS_POR_BLOQUE) {
        int real_block_index = ino->blocks_index[block_index];
        if (real_block_index == UNASSIGNED_BLOCK) break;

        struct block *blk = &superblock.data_blocks[real_block_index];

        size_t to_copy = block_size - block_offset;
        if (to_copy > remaining) to_copy = remaining;

        memcpy(buffer + bytes_read, blk->data + block_offset, to_copy);
        bytes_read += to_copy;
        remaining -= to_copy;
        block_index++;
        block_offset = 0;
    }

    return bytes_read;
}

// - Verifica que el path exista y que no sea un directorio.
// - Calcula el bloque y el offset dentro del bloque para cada parte de los datos a escribir.
// - Si el bloque correspondiente no existe, lo solicita mediante get_free_block_for_inode.
// - Copia los datos desde el buffer proporcionado al bloque de datos del archivo.
// - Actualiza el tamaño del archivo si se escribe más allá del tamaño anterior.
// - Actualiza los tiempos de modificación y cambio de estado.
// Devuelve la cantidad total de bytes escritos o un código de error negativo.
int fs_write(const char *path, const char *buf, size_t size, off_t offset) {
    int inode_index = get_inode_from_path(path);
    if (inode_index < 0) return -ENOENT;

    inode_t *ino = &superblock.inodes[inode_index];
    if (ino->type != TYPE_REG) return -EISDIR;

    size_t total_written = 0;

    while (total_written < size) {
        size_t abs_offset = offset + total_written;
        int block_idx = abs_offset / BLOCK_SIZE;
        int offset_in_block = abs_offset % BLOCK_SIZE;

        if (block_idx >= MAX_INODOS_POR_BLOQUE) return -EFBIG;

        if (ino->blocks_index[block_idx] == UNASSIGNED_BLOCK) {
            int res = get_free_block_for_inode(inode_index, block_idx);
            if (res < 0) return res;
            ino->n_blocks++;
        }

        int real_block_index = ino->blocks_index[block_idx];
        struct block *blk = &superblock.data_blocks[real_block_index];

        size_t space_left = BLOCK_SIZE - offset_in_block;
        size_t to_write = (size - total_written < space_left) ? (size - total_written) : space_left;

        memcpy(blk->data + offset_in_block, buf + total_written, to_write);
        total_written += to_write;
    }

    if (offset + total_written > ino->size) ino->size = offset + total_written;

    time_t now = time(NULL);
    ino->last_modif = now;
    ino->last_change_status = now;

    return total_written;
}

int fs_create(const char *path, mode_t mode) {
    return new_inode(path, __S_IFREG | mode, TYPE_REG);
}

int fs_mkdir(const char *path, mode_t mode) {
    return new_inode(path, __S_IFDIR | mode, TYPE_DIR);
}

/// Dado un path a un archivo, elimina el inodo que represennta al archivo a ser eliminado.
/// param path path del archivo que se quiere eliminar
/// return 0 si se elimino correctamente. En caso contrario,
/// se setea errno al error correspondiente y se devuelve ese error.
int fs_unlink(const char *path) {
    int inode_index = get_inode_from_path(path);
    if (inode_index < 0) return -ENOENT;

    inode_t *inode = &superblock.inodes[inode_index];
    if (inode->type == TYPE_DIR) return -EISDIR;

    superblock.inode_bitmap[inode_index] = FREE;

    for (int i = 0; i < inode->n_blocks; i++) {
        int block_index = inode->blocks_index[i];
        if (block_index != UNASSIGNED_BLOCK) {
            memset(&superblock.data_blocks[block_index], 0, sizeof(struct block));
            superblock.data_bitmap[block_index] = FREE;
            inode->blocks_index[i] = UNASSIGNED_BLOCK;
        }
    }

    memset(inode->file_name, 0, MAX_LEN_NOMBRE);
    return 0;
}

int fs_rmdir(const char *path) {
    int inode_index = get_inode_from_path(path);
    if (inode_index < 0) return -ENOENT;

    inode_t *inode = &superblock.inodes[inode_index];
    if (inode->type != TYPE_DIR) return -ENOTDIR;

    for (int i = 0; i < CANTIDAD_INODOS; i++) {
        if (superblock.inode_bitmap[i] == USED && is_in_dir(path, superblock.inodes[i].file_name)) {
            return -ENOTEMPTY;
        }
    }

    superblock.inode_bitmap[inode_index] = FREE;
    for (int i = 0; i < inode->n_blocks; i++) {
        int block_index = inode->blocks_index[i];
        if (block_index != FREE) {
            memset(&superblock.data_blocks[block_index], 0, sizeof(struct block));
            superblock.data_bitmap[block_index] = FREE;
            inode->blocks_index[i] = FREE;
        }
    }

    return 0;
}

// Implementa la operación truncate para archivos regulares.
// En esta versión solo se considera el caso size == 0, que vacía el archivo:
// - Libera todos los bloques asociados al archivo.
// - Actualiza el tamaño y la cantidad de bloques del inodo.
// - Actualiza los tiempos de modificación y cambio de estado.
int fs_truncate(const char *path, off_t size) {
    int inode_idx = get_inode_from_path(path);
    if (inode_idx < 0) return -ENOENT;

    inode_t *ino = &superblock.inodes[inode_idx];
    if (ino->type != TYPE_REG) return -EISDIR;

    if (size == 0) {
        for (int i = 0; i < MAX_INODOS_POR_BLOQUE; i++) {
            int bloque_idx = ino->blocks_index[i];
            if (bloque_idx != UNASSIGNED_BLOCK && superblock.data_bitmap[bloque_idx] == USED) {
                liberar_bloque(bloque_idx);
                ino->blocks_index[i] = UNASSIGNED_BLOCK;
            }
        }
        ino->n_blocks = 0;
        ino->size = 0;
    }

    time_t ahora = time(NULL);
    ino->last_modif = ahora;
    ino->last_change_status = ahora;

    return 0;
}

// Esta funcion se utiliza para cambiar los tiempos de acceso y modificacion de un archivo.
int fs_utimens(const char *path, const struct timespec tv[2]) {
    int inode_i = get_inode_from_path(path);
    if (inode_i < 0) return -ENOENT;

    inode_t *inode = &superblock.inodes[inode_i];
    inode->last_access = tv[0].tv_sec;
    inode->last_modif = tv[1].tv_sec;

    return 0;
}

// -------------------------------- OPERACIONES CON INODOS/BLOQUES -------------------------------- //

// Libera un bloque de datos del sistema de archivos:
// - Marca el bloque como libre en el bitmap de bloques.
// - Borra su contenido poniéndolo en cero.
void
liberar_bloque(int bloque_idx)
{
	if (bloque_idx < 0 || bloque_idx >= CANTIDAD_BLOQUES)
		return;

	superblock.data_bitmap[bloque_idx] = FREE;
	memset(&superblock.data_blocks[bloque_idx], 0, sizeof(data_block_t));
}

/// Busca por el indice del inodo que representa al path pasado por parametro.
/// param path Path del inodo que se quiere obtener.
/// return Indice del inodo en la lista que contiene todos los inodos. -1 en caso de no encotrarlo.
int
get_inode_from_path(const char *path)
{
	printf("[debug] get_inode_from_path: buscando %s\n", path);
	for (int i = 0; i < CANTIDAD_INODOS; i++) {
		if (superblock.inode_bitmap[i] == USED &&
		    strcmp(superblock.inodes[i].file_name, path) == 0) {
			printf("[debug] get_inode_from_path: encontrado inodo "
			       "%d para %s\n",
			       i,
			       path);
			return i;
		}
	}
	printf("[debug] get_inode_from_path: no se encontró el inodo para %s\n",
	       path);
	return -1;
}

int
get_free_block_i()
{
	/*
	        PRE:
	        - El bitmap de bloques `data_bitmap` debe estar inicializado.
	        - Las constantes `FREE` y `CANTIDAD_BLOQUES` deben estar definidas.

	        POST:
	        - Devuelve el índice del primer bloque libre en `data_bitmap` empezando desde el índice 1.
	        - Si no hay bloques libres, devuelve -1.
	*/
	int block = 1;
	while (block < CANTIDAD_BLOQUES && superblock.data_bitmap[block] != FREE) {
		block++;
	}
	if (block == CANTIDAD_BLOQUES) {
		printf("[debug] get_free_block_i: no hay bloques libres\n");
		return -1;
	}
	return block;
}

int
get_free_block_for_inode(int inode_index, int pos_offset)
{
	/*
	        PRE:
	        - El índice `inode_index` debe ser válido y referir a un inodo existente.
	        - `pos_offset` debe estar dentro de los límites del arreglo de bloques del inodo.
	        - `inodes`, `superblock`, `data_region_blocks` y `data_bitmap` deben estar inicializados correctamente.

	        POST:
	        - Asigna un bloque libre al inodo `inodes[inode_index]` en la posición `pos_offset`.
	        - Marca el bloque en el bitmap como `USED`.
	        - Establece el puntero `file_data[pos_offset]` al bloque correspondiente.
	        - Devuelve 0 en caso de éxito, o `-ENOMEM` si no hay bloques libres.
	*/
	int free_block_index = get_free_block_i();
	if (free_block_index < 0) {
		errno = ENOMEM;
		return -ENOMEM;
	}

	printf("[debug] get_free_block_for_inode: bloque libre encontrado: "
	       "%d\n",
	       free_block_index);
	superblock.data_bitmap[free_block_index] = USED;
	superblock.inodes[inode_index].blocks_index[pos_offset] = free_block_index;
	printf("[debug] bloque %d asignado al inodo %d en la posición %d\n",
	       free_block_index,
	       inode_index,
	       pos_offset);

	return 0;
}


int
get_free_inode_i()
{
	/*
	        PRE:
	        - El bitmap de inodos `inode_bitmap` debe estar inicializado.
	        - La constante `CANTIDAD_INODOS` debe estar definida.

	        POST:
	        - Devuelve el índice del primer inodo libre en `inode_bitmap`, comenzando desde 0.
	        - Si no hay inodos libres, devuelve -1.
	*/
	int inode = 0;
	while (inode < CANTIDAD_INODOS && superblock.inode_bitmap[inode] != FREE) {
		inode++;
	}
	if (inode == CANTIDAD_INODOS) {
		return -1;
	}
	return inode;
}

int
new_inode(const char *path, mode_t mode, int type)
{
	/*
	        PRE:
	        - `path` no debe ser NULL.
	        - La longitud de `path` debe ser menor o igual a `MAX_LEN_NOMBRE`.
	        - `mode` debe ser un modo de archivo válido (usualmente permisos y tipo).
	        - `type` debe ser un valor válido (ej: `TYPE_REG` o `TYPE_DIR`).
	        - Las estructuras globales `superblock`, `inodes`, `data_region_blocks`,
	`data_bitmap` e `inode_bitmap` deben estar correctamente inicializadas.
	        - Debe haber al menos un bloque libre y un inodo libre disponible.

	        POST:
	        - Si hay espacio, crea un nuevo inodo en el índice libre encontrado:
	                        * Marca el inodo y un bloque como `USED`.
	                        * Inicializa todos los campos del inodo con valores por defecto.
	                        * Asocia el primer bloque disponible al nuevo inodo.
	                        * Copia el `path` al campo `file_name` del inodo.
	        - Devuelve `0` en caso de éxito.
	        - Si el `path` es demasiado largo, devuelve `-ENAMETOOLONG` y deja `errno` en `ENAMETOOLONG`.
	        - Si no hay espacio (sin bloques o sin inodos), devuelve `-ENOSPC` y deja `errno` en `ENOSPC`.
	*/
	if (path == NULL) {
		printf("[debug] ERROR: path es NULL\n");
		errno = EINVAL;
		return -EINVAL;
	}

	if (strlen(path) > MAX_LEN_NOMBRE) {
		printf("[debug] ERROR: path demasiado largo (%zu > %d)\n",
		       strlen(path),
		       MAX_LEN_NOMBRE);
		errno = ENAMETOOLONG;
		return -ENAMETOOLONG;
	}

	int block_index = get_free_block_i();
	int inode_index = get_free_inode_i();

	printf("[debug] bloque libre: %d, inodo libre: %d\n",
	       block_index,
	       inode_index);

	if (inode_index < 0 || block_index < 0) {
		printf("[debug] ERROR: sin espacio - inode_index=%d, "
		       "block_index=%d\n",
		       inode_index,
		       block_index);
		errno = ENOSPC;
		return -ENOSPC;
	}

	superblock.data_bitmap[block_index] = USED;
	superblock.inode_bitmap[inode_index] = USED;

	printf("[debug] marcando inodo y bloque como usados\n");

	inode_t *inode = &superblock.inodes[inode_index];

	inode->inum = inode_index;
	inode->mode = mode;
	inode->type = type;
	inode->uid = geteuid();
	inode->size = 0;

	time_t curr_time = time(NULL);
	inode->last_change_status = curr_time;
	inode->last_access = curr_time;
	inode->last_modif = curr_time;

	inode->n_links = (type == TYPE_REG) ? 1 : 2;
	inode->n_blocks = 1;

	inode->blocks_index[0] = block_index;

	// Ya no usamos punteros: file_data[0] = &data_blocks[block_index]; → ELIMINADO

	strcpy(inode->file_name, path);

	printf("[debug] NUEVO INODO CREADO: %s, index: %d\n", path, inode_index);
	return 0;
}


// -------------------------------- OPERACIONES CON DIRECTORIOS -------------------------------- //

/// Cuenta la cantidad de partes del path separadas por `/`
/// param path El path del archivo o directorio a verificar
/// return TRUE si el path tiene una sola parte (es decir, es un hijo directo del root), FALSE en caso contrario.
bool
is_direct_child_of_root(const char *path)
{
	int count = 0;
	for (size_t i = 0; path[i] != '\0'; i++) {
		if (path[i] == '/')
			count++;
	}
	return count == 1;
}

/// Busca la posición del último carácter '/' en el path dado
/// param path Cadena que representa una ruta de archivo o directorio
/// return Índice del último '/', o -1 si no se encuentra ninguno o el path es NULL
int
get_last_dentry(const char *path)
{
	if (path == NULL) {
		return -1;
	}
	size_t path_length = strlen(path);
	int curr_slash = -1;
	for (int i = 0; i < path_length; i++) {
		if (path[i] == '/') {
			curr_slash = i;
		}
	}
	return curr_slash;
}

/// Verifica si un path está contenido dentro de un directorio
/// param dir_path El path del directorio
/// param path El path del archivo o directorio a verificar
/// return 1 si el path está contenido dentro del directorio, 0 en caso contrario.
int
is_in_dir(const char *dir_path, const char *path)
{
	if (!dir_path || !path)
		return 0;

	size_t dir_len = strlen(dir_path);
	size_t path_len = strlen(path);

	if (path_len <= dir_len)
		return 0;
	if (strncmp(dir_path, path, dir_len) != 0)
		return 0;
	if (path[dir_len] != '/')
		return 0;

	const char *rest = path + dir_len + 1;
	if (strchr(rest, '/') != NULL)
		return 0;

	return 1;
}

// -------------------------------- PERSISTENCIA EN DISCO -------------------------------- //

void
write_data(void *data, size_t size, size_t nitems, FILE *file)
{
	if (fwrite(data, size, nitems, file) < 0) {
		printf("Error al persistir file system en archivo!\n");
	}
}

void
persist_fs_data(const char *file)
{
	FILE *r_file;
	r_file = fopen(file, "wb");
	if (!r_file) {
		printf("Error al abrir el archivo para escritura de "
		       "estructuras del fs.\n");
		return;
	}

	// Guardar bitmap de inodos en disco
	write_data(inode_bitmap, sizeof(int), CANTIDAD_INODOS, r_file);

	// Guardar bitmap de bloques de datos en disco
	write_data(data_bitmap, sizeof(int), CANTIDAD_BLOQUES, r_file);

	// Guardar inodos en disco
	write_data(inodes, sizeof(inode_t), CANTIDAD_INODOS, r_file);

	// Guardar los bloques de datos en disco
	write_data(data_region_blocks, sizeof(data_block_t), CANTIDAD_BLOQUES, r_file);

	fclose(r_file);
}

/// Crea el archivo donde se persistira el file_system, escribe los datos en el
/// archivo e inicializa todos los structs necesarios para el manejo del file system
void
init_default_fs_and_persist()
{
	superblock.data_blocks = data_region_blocks;
	superblock.inodes = inodes;
	superblock.inode_bitmap = inode_bitmap;
	superblock.data_bitmap = data_bitmap;

	// Inicializar los bitmaps como libres
	for (int i = 0; i < CANTIDAD_INODOS; i++) {
		superblock.inode_bitmap[i] = FREE;

		for (int j = 0; j < MAX_INODOS_POR_BLOQUE; j++) {
			inodes[i].blocks_index[j] = UNASSIGNED_BLOCK;
		}
	}
	for (int i = 0; i < CANTIDAD_BLOQUES; i++) {
		superblock.data_bitmap[i] = FREE;
	}

	persist_fs_data(filedisk);
}

/// Lee la informacion que esta persistida sobre el file system y lo carga en
int
unmarshall_default_file(FILE *file)
{
	int read_result;

	read_result = fread(inode_bitmap, sizeof(int), CANTIDAD_INODOS, file);
	if (read_result != CANTIDAD_INODOS)
		return -1;

	read_result = fread(data_bitmap, sizeof(int), CANTIDAD_BLOQUES, file);
	if (read_result != CANTIDAD_BLOQUES)
		return -1;

	read_result = fread(inodes, sizeof(inode_t), CANTIDAD_INODOS, file);
	if (read_result != CANTIDAD_INODOS)
		return -1;

	read_result = fread(
	        data_region_blocks, sizeof(data_block_t), CANTIDAD_BLOQUES, file);
	if (read_result != CANTIDAD_BLOQUES)
		return -1;

	// Reconstrucción del superblock
	superblock.inodes = inodes;
	superblock.inode_bitmap = inode_bitmap;
	superblock.data_bitmap = data_bitmap;
	superblock.data_blocks = data_region_blocks;
	superblock.blocks = CANTIDAD_BLOQUES;

	return 0;
}