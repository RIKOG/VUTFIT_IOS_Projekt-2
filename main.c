// Richard Gajdosik, WSL Ubuntu, IDE = Clion
// gcc 9.3
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <limits.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>


#define MMAP(pointer) {(pointer) = mmap(NULL, sizeof(*(pointer)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);}
#define UNMAP(pointer) {munmap((pointer), sizeof((pointer)));}
//Todo poviem procesom nech sa zatvoria
//Todo try catch states where fork didnt get created

int *order_of_prints = NULL;
int *number_of_elves_waiting = NULL;
int *number_of_reindeers_waiting = NULL;
int *reindeers_ready_flag = NULL;
int *christmas_flag = NULL;
sem_t *semafor_elf;
sem_t *semafor_santa;
sem_t *semafor_reindeer;
sem_t *semafor_writing_incrementing;
FILE *fp, *random_generator;

void process_santa(int number_of_reindeers_total) {
    while (1) {
        sem_wait(semafor_writing_incrementing);
        fprintf(fp, "%d: Santa: going to sleep\n", *order_of_prints);
        fflush(fp);
        (*order_of_prints)++;
        sem_post(semafor_writing_incrementing);

        sem_wait(semafor_santa);
        // All the reindeers are ready
        if ((*reindeers_ready_flag) == 1) {
            // We hitch the reindeers
            for (int i = 0; i < number_of_reindeers_total; i++) {
                sem_post(semafor_reindeer);
            }
            sem_wait(semafor_writing_incrementing);
            (*christmas_flag) = 1;
            fprintf(fp, "%d: Santa: Christmas started\n", *order_of_prints);
            fflush(fp);
            (*order_of_prints)++;
            sem_post(semafor_writing_incrementing);
            exit(0);
        }
        sem_wait(semafor_writing_incrementing);
        fprintf(fp, "%d: Santa: helping elves\n", *order_of_prints);
        fflush(fp);
        (*order_of_prints)++;
        (*number_of_elves_waiting) -= 3;
        sem_post(semafor_writing_incrementing);

        sem_post(semafor_elf);
        sem_post(semafor_elf);
        sem_post(semafor_elf);
    }
}

void process_elf(int elfID, int wait_value) {
    unsigned int random_value;
    sem_wait(semafor_writing_incrementing);
    fprintf(fp, "%d: Elf %d: started\n", *order_of_prints, elfID);
    fflush(fp);
    (*order_of_prints)++;
    sem_post(semafor_writing_incrementing);

    fread(&random_value, sizeof(random_value), 1, random_generator);
    // Generating random numbers
    usleep(random_value % wait_value);

    if ((*christmas_flag) == 1) {
        sem_wait(semafor_writing_incrementing);
        fprintf(fp, "%d: Elf %d: taking holidays\n", *order_of_prints, elfID);
        fflush(fp);
        (*order_of_prints)++;
        sem_post(semafor_writing_incrementing);
        exit(0);
    }

    sem_wait(semafor_writing_incrementing);
    fprintf(fp, "%d: Elf %d: need help\n", *order_of_prints, elfID);
    fflush(fp);
    (*order_of_prints)++;
    (*number_of_elves_waiting)++;
    // Todo uvolnujem troch elfov ale nedaval som ho do poradia ? blbost
    if ((*number_of_elves_waiting) >= 3) {
        sem_post(semafor_writing_incrementing);
        sem_post(semafor_santa);
        sem_wait(semafor_elf);
    } else {
        sem_post(semafor_writing_incrementing);
        sem_wait(semafor_elf);
    }
    sem_wait(semafor_writing_incrementing);
    fprintf(fp, "%d: Elf %d: get help\n", *order_of_prints, elfID);
    fflush(fp);
    (*order_of_prints)++;
    sem_post(semafor_writing_incrementing);

    exit(0);

}

void process_reindeer(int reindeerID, int wait_value, int number_of_reindeers_total) {
    unsigned int random_value;
    sem_wait(semafor_writing_incrementing);
    fprintf(fp, "%d: RD %d: rstarted\n", *order_of_prints, reindeerID);
    fflush(fp);
    (*order_of_prints)++;
    sem_post(semafor_writing_incrementing);

    fread(&random_value, sizeof(random_value), 1, random_generator);

    // Generating numbers from range of (upperval - lowerval+1) + lowerval
    usleep(random_value % (wait_value - (wait_value / 2 + 1)) + wait_value / 2);

    sem_wait(semafor_writing_incrementing);
    (*number_of_reindeers_waiting)++;
    if ((*number_of_reindeers_waiting) >= number_of_reindeers_total) {
        (*reindeers_ready_flag) = 1;
        sem_post(semafor_santa);
    }
    sem_post(semafor_writing_incrementing);

    sem_wait(semafor_reindeer);

    sem_wait(semafor_writing_incrementing);
    fprintf(fp, "%d: RD %d: get hitched\n", *order_of_prints, reindeerID);
    fflush(fp);
    (*order_of_prints)++;
    (*number_of_reindeers_waiting)++;
    sem_post(semafor_writing_incrementing);
    exit(0);
}

int init_semaphores() {
    MMAP(order_of_prints);
    MMAP(number_of_elves_waiting);
    MMAP(number_of_reindeers_waiting);
    MMAP(reindeers_ready_flag);
    MMAP(christmas_flag);

    sem_unlink("/xgajdo33.semafor_elf");
    sem_unlink("/xgajdo33.semafor_santa");
    sem_unlink("/xgajdo33.semafor_reindeer");
    sem_unlink("/xgajdo33.semafor_writing_incrementing");
    if ((fp = fopen("text.txt", "w+")) == NULL) {
        fprintf(stderr, "The file failed to open!\n");
        exit(-1);
    }
    if ((random_generator = fopen("/dev/random", "r")) == NULL) {
        fprintf(stderr, "The file failed to open!\n");
        exit(-1);
    }
    if ((semafor_elf = sem_open("/xgajdo33.semafor_elf", O_CREAT | O_EXCL, 0644, 0)) == SEM_FAILED) {
        return -1;
    }
    if ((semafor_santa = sem_open("/xgajdo33.semafor_santa", O_CREAT | O_EXCL, 0644, 0)) == SEM_FAILED) {
        return -1;
    }
    if ((semafor_reindeer = sem_open("/xgajdo33.semafor_reindeer", O_CREAT | O_EXCL, 0644, 0)) == SEM_FAILED) {
        return -1;
    }
    if ((semafor_writing_incrementing = sem_open("/xgajdo33.semafor_writing_incrementing", O_CREAT | O_EXCL, 0644,
                                                 1)) == SEM_FAILED) {
        return -1;
    }
    return 0;
}

void clean_up() {
    UNMAP(order_of_prints);
    UNMAP(number_of_elves_waiting);
    UNMAP(number_of_reindeers_waiting);
    UNMAP(reindeers_ready_flag);
    UNMAP(christmas_flag);
    // TODO sem_close viacej
    sem_close(semafor_elf);
    sem_unlink("/xgajdo33.semafor_elf");
    sem_unlink("/xgajdo33.semafor_santa");
    sem_unlink("/xgajdo33.semafor_semafor_reindeer");
    sem_unlink("/xgajdo33.semafor_writing_incrementing");


    if (fclose(fp) == EOF) {
        fprintf(stderr, "Closing of the file failed!\n");
    }
    if (fclose(random_generator) == EOF) {
        fprintf(stderr, "Closing of the file failed!\n");
    }
}

// Returns number if the given string contains a number and nothing but number chars, otherwise returns -1
int check_if_number(char string[]) {
    char *ptr;
    int number = 0, i = 0;
    // We just check if everything in string is between 9 and 0 chars
    while (string[i] != '\0') {
        // todo add support for +500 and -500.. maybe even support for 5.2 decimals
        if (string[i] <= '9' && string[i] >= '0') {
        } else {
            fprintf(stderr, "One of the given arguments is not an integer or has negative value!\n");
            return -1;
        }
        i++;
    }
    // Conversion
    number = strtol(string, &ptr, 0);
    return number;
}

int main(int argc, char *argv[]) {
    if (init_semaphores() != 0) {
        fprintf(stderr, "Initialization of semaphores failed!\n");
        exit(-1);
    }
    (*order_of_prints)++;
    // Todo what if we get really big argument ?
    char string[1001];
    int flag_if_number = 0, arguments_values[4] = {0};
    // We check whether the number of arguments is correct
    if (argc != 5) {
        fprintf(stderr, "The number of arguments is incorrect!\n");
        clean_up();
        exit(-1);
    }
    // We load and convert arguments from char arrays to integer arrays
    for (int i = 1; i < argc; i++) {
        strcpy(string, argv[i]);
        flag_if_number = check_if_number(string);
        // The function returned -1 meaning the string wasnt holding numeric chars
        if (flag_if_number == -1) {
            clean_up();
            return -1;
        }
        arguments_values[i - 1] = flag_if_number;
    }
    // We check whether the arguments are of the right size
    if (
            (arguments_values[0] >= 1000 || arguments_values[0] <= 0)
            ||
            (arguments_values[1] >= 20 || arguments_values[1] <= 0)
            ||
            (arguments_values[2] > 1000 || arguments_values[2] < 0)
            ||
            (arguments_values[3] > 1000 || arguments_values[3] < 0)
            ) {
        fprintf(stderr, "The size of arguments is incorrect!\n");
        clean_up();
        return -1;
    }
// Prints out values
    for (int i = 0; i < 4; i++) {
        printf("%d\n", arguments_values[i]);
    }

    pid_t santa = fork();
    if (santa == 0) {
        process_santa(arguments_values[1]);
    }
    pid_t elve_generator = fork();
    if (elve_generator == 0) {
        for (int i = 1; i <= arguments_values[0]; i++) {
            printf("%d\n", i);
            pid_t elf = fork();
            if (elf == 0) {
                process_elf(i, arguments_values[2]);
            }
        }
        exit(0);
    }
    pid_t reindeer_generator = fork();
    if (reindeer_generator == 0) {
        for (int i = 1; i <= arguments_values[1]; i++) {
            pid_t reindeer = fork();
            if (reindeer == 0) {
                process_reindeer(i, arguments_values[3], arguments_values[1]);
            }
        }
        exit(0);
    }

    // Waiting until parent process is the only one left
    int IDs;
    do {
        IDs = wait(NULL);
    } while (IDs != -1);
    clean_up();
    return 0;
}
