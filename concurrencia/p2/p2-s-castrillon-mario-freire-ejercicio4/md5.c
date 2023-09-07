#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <openssl/evp.h>
#include <threads.h>

#include "options.h"
#include "queue.h"


#define MAX_PATH 1024
#define BLOCK_SIZE (10*1024*1024)
#define MAX_LINE_LENGTH (MAX_PATH * 2)


struct file_md5 {
    char *file;
    unsigned char *hash;
    unsigned int hash_size;
};


struct reader_args {
    char* dir;
    struct args* args;

};

struct args{
    queue in_q;
    queue out_q;
    struct options opt;
};




void get_entries(char *dir, void* args);

// Funcion auxiliar para crear el thread que lee los archivos
int aux(void * args){
    struct reader_args *a = args;


    get_entries(a->dir,a->args);
    
    q_final(a->args->in_q);
    return 0;
}


void print_hash(struct file_md5 *md5) {
    for(int i = 0; i < md5->hash_size; i++) {
        printf("%02hhx", md5->hash[i]);
    }
}


void read_hash_file(char *file, char *dir, queue q) {
    FILE *fp;
    char line[MAX_LINE_LENGTH];
    char *file_name, *hash;
    int hash_len;

    if((fp = fopen(file, "r")) == NULL) {
        printf("Could not open %s : %s\n", file, strerror(errno));
        exit(0);
    }

    while(fgets(line, MAX_LINE_LENGTH, fp) != NULL) {
        char *field_break;
        struct file_md5 *md5 = malloc(sizeof(struct file_md5));

        if((field_break = strstr(line, ": ")) == NULL) {
            printf("Malformed md5 file\n");
            exit(0);
        }
        *field_break = '\0';

        file_name = line;
        hash      = field_break + 2;
        hash_len  = strlen(hash);

        md5->file      = malloc(strlen(file_name) + strlen(dir) + 2);
        sprintf(md5->file, "%s/%s", dir, file_name);
        md5->hash      = malloc(hash_len / 2);
        md5->hash_size = hash_len / 2;


        for(int i = 0; i < hash_len; i+=2)
            sscanf(hash + i, "%02hhx", &md5->hash[i / 2]);

        q_insert(q, md5);
    }


    fclose(fp);
}


void sum_file(struct file_md5 *md5) {
    EVP_MD_CTX *mdctx;
    int nbytes;
    FILE *fp;
    char *buf;

    if((fp = fopen(md5->file, "r")) == NULL) {
        printf("Could not open %s\n", md5->file);
        return;
    }

    buf = malloc(BLOCK_SIZE);
    const EVP_MD *md = EVP_get_digestbyname("md5");

    mdctx = EVP_MD_CTX_create();
    EVP_DigestInit_ex(mdctx, md, NULL);

    while((nbytes = fread(buf, 1, BLOCK_SIZE, fp)) >0)
        EVP_DigestUpdate(mdctx, buf, nbytes);

    md5->hash = malloc(EVP_MAX_MD_SIZE);
    EVP_DigestFinal_ex(mdctx, md5->hash, &md5->hash_size);

    EVP_MD_CTX_destroy(mdctx);
    free(buf);
    fclose(fp);
}


void recurse(char *entry, void *arg) {
    struct stat st;

    stat(entry, &st);

    if(S_ISDIR(st.st_mode))
        get_entries(entry, arg);
}


void add_files(char *entry, void *arg) {
    struct args *a = arg;
    struct stat st;

    stat(entry, &st);
    if(S_ISREG(st.st_mode)){
        if(q_insert(a->in_q, strdup(entry))== -1){
            printf("Error intentando insertar en la cola (no deberia ocurrir)): Saliendo\n");
            exit(101);
        }
    }
}


void walk_dir(char *dir, void (*action)(char *entry, void *arg), void *arg) {
    DIR *d;
    struct dirent *ent;
    char full_path[MAX_PATH];

    if((d = opendir(dir)) == NULL) {
        printf("Could not open dir %s\n", dir);
        return;
    }

    while((ent = readdir(d)) != NULL) {
        if(strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") ==0)
            continue;

        snprintf(full_path, MAX_PATH, "%s/%s", dir, ent->d_name);

        action(full_path, arg);
    }

    closedir(d);
}



void get_entries(char *dir, void* args) {
    walk_dir(dir, add_files, args);
    walk_dir(dir, recurse, args);
}

void check(struct options opt) {
    queue in_q;
    struct file_md5 *md5_in, md5_file;

    in_q  = q_create(opt.queue_size,1);

    read_hash_file(opt.file, opt.dir, in_q);
    q_final(in_q);
    while((md5_in = q_remove(in_q))) {
        md5_file.file = md5_in->file;

        sum_file(&md5_file);

        if(memcmp(md5_file.hash, md5_in->hash, md5_file.hash_size)!=0) {
            printf("File %s doesn't match.\nFound:    ", md5_file.file);
            print_hash(&md5_file);
            printf("\nExpected: ");
            print_hash(md5_in);
            printf("\n");
        }

        free(md5_file.hash);

        free(md5_in->file);
        free(md5_in->hash);
        free(md5_in);
    }

    q_destroy(in_q);
}


int sum_op(void * arg){
    struct args *args = arg;
    char *ent;
    struct file_md5 *md5;
    // Se procesan los archivos de la cola de entrada y se pasan a la cola de salida
    while((ent = q_remove(args->in_q)) != NULL) {
            

        md5 = malloc(sizeof(struct file_md5));

        md5->file = ent;
        sum_file(md5);

        q_insert(args->out_q, md5);
    }
    //mtx_lock(&args->counter_q_mtx);
    //args->counter_q--;

    //if(args->counter_q == 0) q_final(args->out_q);
    q_final(args->out_q);
    //mtx_unlock(&args->counter_q_mtx);

    return 0;
}

int writer_op(void * arg){
    struct args *args = arg;
    FILE *out;
    struct file_md5 *md5;
    int dirname_len;

    // Transpaso a archivo de salida
    if((out = fopen(args->opt.file, "w")) == NULL) {
        printf("Could not open output file\n");
        exit(0);
    }

    dirname_len = strlen(args->opt.dir) + 1; // length of dir + /
    // Se procesan los archivos en la cola de salida, se guarda la información
    // en el archivo pasado por parámetro
    while((md5 = q_remove(args->out_q)) != NULL) {
        fprintf(out, "%s: ", md5->file + dirname_len);
        for(int i = 0; i < md5->hash_size; i++)
            fprintf(out, "%02hhx", md5->hash[i]);
        fprintf(out, "\n");

        free(md5->file);
        free(md5->hash);
        free(md5);
    }
    fclose(out);
    return 0;
}

void sum(struct options opt) {
    queue in_q, out_q;
    
    in_q  = q_create(opt.queue_size,1);
    out_q = q_create(opt.queue_size,opt.num_threads);
    ///////////////////////////////////////////
    thrd_t reader;
    thrd_t writer;
    thrd_t ops[opt.num_threads];
    struct args args;
    struct reader_args a;

    a.dir=opt.dir;
    a.args=&args;
    args.opt = opt;

    args.in_q = in_q;
    args.out_q = out_q;
    //args.counter_q = opt.num_threads;
    //mtx_init(&args.counter_q_mtx,mtx_plain);
    
    // Se añaden a la cola de entrada los archivos
    thrd_create(&reader,aux,&a);
    for(int i = 0; i < opt.num_threads; i++) thrd_create(&ops[i],sum_op,&args);
    thrd_create(&writer,writer_op,&args);

    // Poner los joins para evitar leaks de memoria
    for(int i = 0; i < opt.num_threads; i++) thrd_join(ops[i],NULL);
    thrd_join(reader,NULL);
    thrd_join(writer,NULL);

    q_destroy(in_q);
    q_destroy(out_q);

}


int main(int argc, char *argv[]) {
    

    struct options opt;

    opt.num_threads = 5;
    opt.queue_size  = 1000;
    opt.check       = true;
    opt.file        = NULL;
    opt.dir         = NULL;

    read_options (argc, argv, &opt);
    if(opt.check)
        check(opt);
    else
        sum(opt);

}
