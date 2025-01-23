#include <bits/stdc++.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>

struct ComData {
    char n[11];
    double p;
};

union semun {
    int val;
    struct semid_ds* buf;
    ushort* array;
} semun_arg;

inline void semWait(int sem_id) {
    struct sembuf sem_op;
    sem_op.sem_num = 0;
    sem_op.sem_op = -1;
    sem_op.sem_flg = 0;
    semop(sem_id, &sem_op, 1);
}

inline void semSignal(int sem_id) {
    struct sembuf sem_op;
    sem_op.sem_num = 0;
    sem_op.sem_op = 1;
    sem_op.sem_flg = 0;
    semop(sem_id, &sem_op, 1);
}

inline void writeInt(unsigned char* ptr, int val) {
    memcpy(ptr, &val, sizeof(int));
}

inline int readInt(unsigned char* ptr) {
    int data = 0;
    memcpy(&data, ptr, sizeof(int));
    return data;
}

class Commodities {
   private:
    unsigned int maxSize;
    int front;
    int rear;
    unsigned char* ptr;
    ComData* arr;

   public:
    Commodities() {}
    
    Commodities(unsigned char* mem, int max) {
        ptr = mem;
        maxSize = max;
        front = readInt(mem);
        mem += sizeof(int);
        rear = readInt(mem);
    }

    void push(ComData d) {
        unsigned char* mem = ptr;
        front = readInt(ptr);
        
        if (front == -1) {
            front = rear = 0;
        } else {
            rear = (rear + 1) % maxSize;
        }
        
        writeInt(mem, front);
        mem += sizeof(int);
        writeInt(mem, rear);
        mem += sizeof(int);
        mem += rear * sizeof(ComData);
        memcpy(mem, &d, sizeof(ComData));
    }
};

//Global Var
key_t s_key = 1;
key_t n_key = 2;
key_t e_key = 3;

std::string name;
double priceMean;
double priceDev;
int sleepDelay;
unsigned int bufSize;
double curPrice;
std::random_device rd;
std::mt19937 gen(rd());
std::normal_distribution<double> distribution;
unsigned char* sharedMem;
int shm_id;
int s, n, e;
Commodities coms;

void log() {
    int BUFFERSIZE = 255;
    char time[BUFFERSIZE] = {0};
    struct timespec tv;
    clock_gettime(CLOCK_REALTIME, &tv);
    struct tm* timePointerEnd = localtime(&tv.tv_sec);
    size_t nbytes = strftime(time, BUFFERSIZE, "[%m/%d/%Y %T.", timePointerEnd);
    snprintf(time + nbytes, sizeof(time) - nbytes,
             "%.3ld]", tv.tv_nsec / 1000000);
    std::string out = time;
    out += " " + name + ": ";
    std::cerr << out;
}

void produce() {
    double newPrice;
    do {
        newPrice = distribution(gen);
    } while (newPrice <= 0); 
    curPrice = newPrice;
    char time[80];
    std::time_t rawtime;
    std::tm* timeinfo;
    std::time(&rawtime);
    timeinfo = std::localtime(&rawtime);
    std::strftime(time, 80, "%Y/%m/%d %H:%M:%S", timeinfo);
    printf("%s %s: generating a new value %.2f\n", time, name.c_str(), curPrice);
}

void append() {
    ComData dd;
    strncpy(dd.n, name.c_str(), sizeof(dd.n) - 1);
    dd.n[sizeof(dd.n) - 1] = '\0';  // Ensure null termination
    dd.p = curPrice;
    log();
    printf("placing %.2lf on shared buffer\n", curPrice);
    coms.push(dd);
}

key_t generate_key(char id, int buffer_size) {
    char filename[256];
    sprintf(filename, "/tmp/commodity_%d_%c", buffer_size, id);
    
    FILE* fp = fopen(filename, "w");
    if (fp) fclose(fp);
    
    return ftok(filename, buffer_size);
}

int main(int argc, char* argv[]) {
    if (argc != 6) {
        std::cerr << "Usage: " << argv[0] << " <commodity_name> <mean_price> <std_dev> <delay_ms> <buffer_size>\n";
        return 1;
    }

    name = argv[1];
    priceMean = std::stod(argv[2]);
    priceDev = std::stod(argv[3]);
    sleepDelay = atoi(argv[4]);
    bufSize = atoi(argv[5]);
    distribution = std::normal_distribution<double>(priceMean, priceDev);
    size_t shm_size = (bufSize * sizeof(ComData)) + (2 * sizeof(int));
    
    key_t shm_key = generate_key('m', bufSize);
    s_key = generate_key('s', bufSize);
    n_key = generate_key('n', bufSize);
    e_key = generate_key('e', bufSize);

    printf("Waiting for shared memory (buffer size %d)...\n", bufSize);
    while ((shm_id = shmget(shm_key, shm_size, 0666)) == -1) {
        sleep(1);
    }

    sharedMem = (unsigned char*)shmat(shm_id, NULL, 0);
    if (sharedMem == (unsigned char*)-1) {
        perror("shmat failed");
        exit(1);
    }

    bool semaphoresReady = false;
    printf("Waiting for semaphores...\n");
    while (!semaphoresReady) {
        s = semget(s_key, 1, 0666);
        n = semget(n_key, 1, 0666);
        e = semget(e_key, 1, 0666);

        if (s != -1 && n != -1 && e != -1) {
            int s_val = semctl(s, 0, GETVAL);
            int n_val = semctl(n, 0, GETVAL);
            int e_val = semctl(e, 0, GETVAL);

            // Check if sems are  init
            if ((s_val == 1 && n_val == 0 && e_val == bufSize) || // Initial state
                (s_val >= 0 && n_val >= 0 && e_val >= 0 && (n_val + e_val == bufSize))) { // Running state
                semaphoresReady = true;
            }
        }
        
        if (!semaphoresReady) {
            printf("Waiting for consumer to initialize semaphores...\n");
            sleep(1);
        }
    }

    printf("Resources ready. Starting production...\n");
    coms = Commodities(sharedMem, bufSize);

    while (true) {
        produce();
        log();
        printf("trying to get mutex on shared buffer\n");
        semWait(e);
        semWait(s);
        append();
        semSignal(s);
        semSignal(n);
        log();
        printf("sleeping for %d ms\n", sleepDelay);
        usleep(sleepDelay * 1000);
    }

    return 0;
}
