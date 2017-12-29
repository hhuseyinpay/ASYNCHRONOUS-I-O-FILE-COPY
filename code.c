#include <aio.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define USAGE "\nUSAGE: %s <source path> <destination path> <number of thread> <type size(Byte(0),MB(1))> <size of the source file to be created(1 Byte to 200 MB) >\n"

#define SOURCE_FILE_NAME "source.txt"
#define DESTINATION_FILE_NAME "destination.txt"


struct thread_argument {
    int thread_num;
    int destinationfd;
    size_t bufsz;
    struct aiocb *aio_p;
};

pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER; // ekrana yazdırırken ortalık karışmaması için

void fill_sourcefile(int fd, int size, int threadnum);

void *ascopy(void *args);

int main(int argc, char *argv[])
{
        int sfd, dfd; // source and destination file descriptor
        int type;
        int filesize;
        int threadnum;
        char *sourcefile;
        char *destinationfile;

        if (argc != 6) {
                fprintf(stderr, USAGE, argv[0]);
                return EXIT_FAILURE;
        }

        sourcefile = argv[1];
        if (strcmp(sourcefile, "-") == 0) {
                sfd = open(SOURCE_FILE_NAME, O_RDWR | O_CREAT | O_TRUNC, 0777);
        } else {
                char *tmp = malloc(strlen(sourcefile) + strlen(SOURCE_FILE_NAME));
                stpcpy(tmp, sourcefile);
                strcat(tmp, SOURCE_FILE_NAME);
                sourcefile = tmp;
                sfd = open(sourcefile, O_RDWR | O_CREAT | O_TRUNC, 0777);
        }
        if (sfd == -1) {
                perror("Error at open <source path>");
                fprintf(stderr, USAGE, argv[0]);
                return EXIT_FAILURE;
        }

        destinationfile = argv[2];
        if (strcmp(destinationfile, "-") == 0) {
                dfd = open(DESTINATION_FILE_NAME, O_WRONLY | O_CREAT | O_TRUNC, 0777);
        } else {
                char *tmp = malloc(strlen(destinationfile) + strlen(DESTINATION_FILE_NAME));
                stpcpy(tmp, destinationfile);
                strcat(tmp, DESTINATION_FILE_NAME);
                destinationfile = tmp;
                dfd = open(destinationfile, O_WRONLY | O_CREAT | O_TRUNC, 0777);
        }

        if (dfd == -1) {
                perror("Error at open <destination path>");
                fprintf(stderr, USAGE, argv[0]);
                return EXIT_FAILURE;
        }

        threadnum = atoi(argv[3]);
        if (0 > threadnum || threadnum > 10) {
                perror("Error at <number of thread>");
                fprintf(stderr, USAGE, argv[0]);
                return EXIT_FAILURE;
        }


        type = atoi(argv[4]);
        if (type == 0) {
                type = 1; //byte
        } else if (type == 1) {
                type = 1048576; // megabyte
        } else {
                perror("Error at <type size>");
                fprintf(stderr, USAGE, argv[0]);
                return EXIT_FAILURE;
        }

        /*
         * filesize 1byte'dan küçük 200MB'dan büyük olamaz.
         */
        filesize = atoi(argv[5]);
        if ((filesize < 1) || (type == 1048576 && filesize > 200) || (type == 1 && filesize > 209715200)) {
                perror("Error at <type size>");
                fprintf(stderr, USAGE, argv[0]);
                return EXIT_FAILURE;
        }
        filesize = filesize * type;

        int i; // will use for loops
        int tmp;

        /*
         * to prepare a source file randomly
         */
        fill_sourcefile(sfd, filesize, threadnum);

        /*
         * buradaki işlem sayesinde dosya boyutu tam bölünecek. Fazlalık kısım ilk threade eklenecek
         */
        tmp = filesize % threadnum;
        filesize = filesize - tmp;
        filesize = filesize / threadnum;
        /*
         * Zero out the aiocb structure
         */
        struct aiocb aio_list[threadnum];
        memset(&aio_list, 0, sizeof(struct aiocb) * threadnum);
        aio_list[0].aio_fildes = sfd;
        for (i = 1; i < threadnum; i++) {
                aio_list[i].aio_fildes = sfd;
                aio_list[i].aio_offset += i * (filesize) + tmp;
        }


        /*
         * create threads
         */
        pthread_t threads[threadnum];
        struct thread_argument thread_args[threadnum];

        memset(&thread_args, 0, sizeof(struct thread_argument) * threadnum);
        thread_args[0].bufsz = tmp; // fazlalık byte ilk thread'e ekleniyor.
        for (i = 0; i < threadnum; i++) {
                thread_args[i].thread_num = i;
                thread_args[i].destinationfd = dfd;
                thread_args[i].bufsz += filesize;
                thread_args[i].aio_p = &aio_list[i];
                pthread_create(&threads[i], NULL, ascopy, &thread_args[i]);
        }

        void *status;
        for (i = 0; i < threadnum; i++)
                pthread_join(threads[i], &status);

        return 0;
}


void fill_sourcefile(int fd, int size, int threadnum)
{
        int i;
        char *buff = malloc(size), *buff2;

        /*
         * buradaki işlem sayesinde dosya boyutu tam bölünecek. Fazlalık kısım ilk threade eklenecek
         */
        int tmp = size % threadnum;
        size = size - tmp;
        size = size / threadnum;

        memset(buff, 'A', sizeof(char) * (size + tmp));

        buff2 = buff + size + tmp;
        for (i = 1; i < threadnum; i++) {
                memset(buff2, 'A' + i, sizeof(char) * size);
                buff2 += size;
        }

        if (write(fd, buff, size * threadnum + tmp) < 0) {
                perror("Cannot write to source file.");
                exit(EXIT_FAILURE);
        }
        free(buff);
}

void *ascopy(void *args)
{
        struct thread_argument *arg = args;

        char *buffer = calloc(arg->bufsz, sizeof(char));
        arg->aio_p->aio_buf = buffer;
        arg->aio_p->aio_nbytes = arg->bufsz;

        pthread_mutex_lock(&print_lock);
        printf("thread %d: %%0 (start reading) \n", arg->thread_num);
        pthread_mutex_unlock(&print_lock);

        /*
         * read process
         */
        aio_read(arg->aio_p);
        while (aio_error(arg->aio_p) == EINPROGRESS);
        aio_return(arg->aio_p);

        pthread_mutex_lock(&print_lock);
        printf("thread %d: %%50 (finished reading, start writing) \n", arg->thread_num);
        printf("%s\n", buffer);
        pthread_mutex_unlock(&print_lock);

        /*
         * write process
         */
        arg->aio_p->aio_fildes = arg->destinationfd;
        aio_write(arg->aio_p);
        while (aio_error(arg->aio_p) == EINPROGRESS);
        aio_return(arg->aio_p);

        pthread_mutex_lock(&print_lock);
        printf("thread %d: %%100 (copy was completed) \n", arg->thread_num);
        pthread_mutex_unlock(&print_lock);
        free(buffer);
        return NULL;
}