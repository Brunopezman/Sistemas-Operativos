#define FUSE_USE_VERSION 30

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

char *filedisk = DEFAULT_FILE_DISK;

static int
fisopfs_getattr(const char *path, struct stat *st)
{
	printf("[debug] fisopfs_getattr - path: %s\n", path);
	return fs_getattr(path, st);
}

static int
fisopfs_readdir(const char *path,
                void *buffer,
                fuse_fill_dir_t filler,
                off_t offset,
                struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_readdir - path: %s\n", path);
	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);

	char entry_name[MAX_LEN_NOMBRE];
	int idx = 0;
	while ((idx = fs_read_dir(path, entry_name, idx)) > 0) {
		filler(buffer, entry_name, NULL, 0);
	}
	return 0;
}

static int
fisopfs_read(const char *path,
             char *buf,
             size_t size,
             off_t offset,
             struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_read - path: %s, offset: %lu, size: %lu\n",
	       path,
	       offset,
	       size);
	return fs_read(path, buf, size, offset);
}

static int
fisopfs_mkdir(const char *path, mode_t mode)
{
	printf("[debug] Funcion para crear un directorio. fisopfs_mkdir - "
	       "path: %s - mode: %d \n",
	       path,
	       mode);
	return fs_mkdir(path, mode);
}

static int
fisopfs_unlink(const char *path)
{
	printf("[debug] into fisop_unlink\n");
	return fs_unlink(path);
}

static int
fisopfs_rmdir(const char *path)
{
	printf("[debug] into fisopfs_rmdir - path: %s\n", path);
	return fs_rmdir(path);
}

static int
fisopfs_write(const char *path,
              const char *buf,
              size_t size,
              off_t offset,
              struct fuse_file_info *fi)
{
	printf("[debug] Funcion para escritura. "
	       "fisopfs_write - path: %s - size: %lu - offset: %lu\n",
	       path,
	       size,
	       offset);
	return fs_write(path, buf, size, offset);
}

static int
fisopfs_truncate(const char *path, off_t size)
{
	return fs_truncate(path, size);
}

static int
fisopfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	printf("[debug] Funcion para la creacion de "
	       "archivos. fisopfs_create - path: %s - mode: %d \n",
	       path,
	       mode);
	return fs_create(path, mode);
}

static int
fisopfs_utimens(const char *path, const struct timespec tv[2])
{
	printf("[debug] fisopfs_utimens - path: %s\n", path);
	return fs_utimens(path, tv);
}

// Inicializa las estructuras en los campos correspondientes del superbloque y
// luego inicializa cada celda marcandolas como 'FREE'. Se ejecuta en main() al montar el filesystem
static void *
fisopfs_init(struct fuse_conn_info *conn_info)
{
	printf("[debug] into fisop_init\n");
	FILE *file;
	// intento abrir archivo
	file = fopen(filedisk, "rb");
	if (!file) {
		// Si NO existe, se crea y se escribe en disco
		init_default_fs_and_persist();
	} else {
		// Si SI existe, se lee de disco
		int res_of_unmarshalling = unmarshall_default_file(file);
		if (res_of_unmarshalling < 0) {
			errno = EIO;
		}
		fclose(file);
	}
	return NULL;
}

/// Guarda los datos del file system en el archivo de disco y libera los recursos utilizados.
static void
fisopfs_destroy(void *private_data)
{
	printf("[debug] into fisop_destroy\n");
	persist_fs_data(filedisk);
}

/// Called on each close so that the filesystem has a chance to report delayed
/// errors. Hace lo mismo que destroy pero devulve un int.
static int
fisopfs_flush(const char *path, struct fuse_file_info *fi)
{
	printf("[debug] into fisop_flush\n");
	persist_fs_data(filedisk);
	return 0;
}

static struct fuse_operations operations = {
	.getattr = fisopfs_getattr,
	.readdir = fisopfs_readdir,
	.read = fisopfs_read,

	// Funciones a implementar (vamos a ir agregando mas, por ahora estas son las que se me ocurren)
	.mkdir = fisopfs_mkdir,
	.rmdir = fisopfs_rmdir,
	.write = fisopfs_write,  // Funciones para '>' y '>>'
	.unlink = fisopfs_unlink,
	.create = fisopfs_create,  // cat y redirecciones del estilo: echo "holaMundo" > file.txt
	.utimens = fisopfs_utimens,  // relacionado con create: para actualizar los tiempos de creacion
	.truncate = fisopfs_truncate,
	.init = fisopfs_init,
	.destroy = fisopfs_destroy,
	.flush = fisopfs_flush,
};

int
main(int argc, char *argv[])
{
	for (int i = 1; i < argc - 1; i++) {
		if (strcmp(argv[i], "--filedisk") == 0) {
			filedisk = argv[i + 1];

			// We remove the argument so that fuse doesn't use our
			// argument or name as folder.
			// Equivalent to a pop.
			for (int j = i; j < argc - 1; j++) {
				argv[j] = argv[j + 2];
			}

			argc = argc - 2;
			break;
		}
	}
	printf("[debug] usando archivo de disco: %s\n", filedisk);
	return fuse_main(argc, argv, &operations, NULL);
}
