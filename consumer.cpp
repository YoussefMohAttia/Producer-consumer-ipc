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

    ComData pop() {
        unsigned char* mem = ptr;
        front = readInt(mem);
        mem += sizeof(int);
        rear = readInt(mem);

        if (front == -1) {
            // Buffer empty
            ComData empty;
            memset(&empty, 0, sizeof(ComData));
            return empty;
        }

        mem += sizeof(int);
        mem += (front * sizeof(ComData));
        ComData x;
        memcpy(&x, mem, sizeof(ComData));

        if (front == rear) {
            // Last element
            front = rear = -1;
        } else {
            front = (front + 1) % maxSize;
        }

        writeInt(ptr, front);
        writeInt(ptr + sizeof(int), rear);
        
        return x;
    }
};

// Global Var
key_t s_key = 1;
key_t n_key = 2;
key_t e_key = 3;
unsigned char* sharedMem;
int shm_id;
int s, n, e;
int bufSize;
Commodities coms;

std::map<std::string, std::deque<double>> comPrices;
std::map<std::string, std::deque<double>> comDashboard;

void createCommodites() {
    std::vector<std::string> commodities = {
        "ALUMINIUM", "COPPER", "COTTON", "CRUDEOIL", "GOLD",
        "LEAD", "MENTHAOIL", "NATURALGAS", "NICKEL", "SILVER", "ZINC"
    };

    // Initialize to zero
    for (const auto& commodity : commodities) {
        comPrices[commodity] = std::deque<double>(5, 0.0);
        comDashboard[commodity] = std::deque<double>{0.0, 0.0};
    }
}

double calcAvg(const std::string& comName) {
    try {
        if (comPrices.find(comName) == comPrices.end()) {
            return 0.0;
        }

        const auto& prices = comPrices[comName];
        if (prices.empty()) return 0.0;

        double sum = 0;
        int count = 0;
        
        for (const double& price : prices) {
            if (price > 0 && count < 5) {
                sum += price;
                count++;
            }
        }
        
        return count > 0 ? sum / count : 0.0;
    } catch (const std::exception& e) {
        std::cerr << "Error in calcAvg(): " << e.what() << std::endl;
        return 0.0;
    }
}

void printDashboard() {
printf("\033[2J\033[H");
    printf("\033[1;34m+-------------------------------------+\033[0m\n");
    printf("\033[1;34m| Currency      |  Price   | AvgPrice |\033[0m\n");
    printf("\033[1;34m+-------------------------------------+\033[0m\n");
    
    for (auto& [commodity, prices] : comPrices) {
        printf("| %-11s |", commodity.c_str());
        
        double currentPrice = prices.front();
        double previousPrice = prices.size() > 1 ? prices[1] : 0;
        double avg = calcAvg(commodity);
        
        // Print curr price
        if (currentPrice == 0) {
            printf(" \033[1;34m%7.2f \033[0m|", currentPrice);
        } else if (currentPrice >= previousPrice) {
            printf(" \033[1;32m%7.2f↑\033[0m|", currentPrice);
        } else {
            printf(" \033[1;31m%7.2f↓\033[0m|", currentPrice);
        }
        
        // Print avg
        if (avg == 0) {
            printf(" \033[1;34m%7.2f \033[0m|\n", avg);
        } else if (avg >= previousPrice) {
            printf(" \033[1;32m%7.2f↑\033[0m|\n", avg);
        } else {
            printf(" \033[1;31m%7.2f↓\033[0m|\n", avg);
        }
    }
    printf("+-------------------------------------+\n");
}

void take() {
    try {
        ComData d = coms.pop();
        if (d.n[0] == '\0') return; // Skip empty data
        
        std::string name(d.n);
        if (comPrices.find(name) == comPrices.end()) {
            // Init if not exist
            comPrices[name] = std::deque<double>(5, 0.0);
        }
        
        // update price
        comPrices[name].push_front(d.p);
        while (comPrices[name].size() > 5) {
            comPrices[name].pop_back();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in take(): " << e.what() << std::endl;
    }
}

key_t generate_key(char id, int buffer_size) {
    // buffer size part of key
    char filename[256];
    sprintf(filename, "/tmp/commodity_%d_%c", buffer_size, id);
    FILE* fp = fopen(filename, "w");
    if (fp) fclose(fp);
    
    return ftok(filename, buffer_size);
}
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <buffer_size>\n";
        return 1;
    }

    bufSize = atoi(argv[1]);
    size_t shm_size = (bufSize * sizeof(ComData)) + (2 * sizeof(int));

    // Generate keys
    key_t shm_key = generate_key('m', bufSize);
    s_key = generate_key('s', bufSize);
    n_key = generate_key('n', bufSize);
    e_key = generate_key('e', bufSize);
    
    shm_id = shmget(shm_key, shm_size, 0666);
    bool need_initialization = false;

    if (shm_id == -1) {
        shm_id = shmget(shm_key, shm_size, 0666 | IPC_CREAT);
        if (shm_id == -1) {
            perror("shmget failed");
            exit(1);
        }
        need_initialization = true;
    }

    sharedMem = (unsigned char*)shmat(shm_id, NULL, 0);
    if (sharedMem == (unsigned char*)-1) {
        perror("shmat failed");
        exit(1);
    }

    s = semget(s_key, 1, 0666);
    n = semget(n_key, 1, 0666);
    e = semget(e_key, 1, 0666);

    if (s == -1 || n == -1 || e == -1) {
        s = semget(s_key, 1, 0666 | IPC_CREAT);
        n = semget(n_key, 1, 0666 | IPC_CREAT);
        e = semget(e_key, 1, 0666 | IPC_CREAT);

        if (s == -1 || n == -1 || e == -1) {
            perror("semget failed");
            exit(1);
        }
        need_initialization = true;
    }

    if (need_initialization) {
        printf("Initializing new resources for buffer size %d\n", bufSize);
        // Init shared memory
        writeInt(sharedMem, -1);      // start
        writeInt(sharedMem + sizeof(int), -1);  // end

        // Init sems
        semun_arg.val = 1;
        semctl(s, 0, SETVAL, semun_arg);
        semun_arg.val = 0;
        semctl(n, 0, SETVAL, semun_arg);
        semun_arg.val = bufSize;
        semctl(e, 0, SETVAL, semun_arg);
    } else {
        printf("Using existing resources for buffer size %d\n", bufSize);
    }

    createCommodites();
    coms = Commodities(sharedMem, bufSize);
    
    while (true) {
        semWait(n);
        semWait(s);
        take();
        semSignal(s);
        semSignal(e);
        printDashboard();
        usleep(100000);
    }
    return 0;
}
