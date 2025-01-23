#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h> 

#define GRID_SIZE 20
#define NUM_THREADS 4
#define GENERATIONS 32


int grid[GRID_SIZE][GRID_SIZE];
pthread_barrier_t barrier;
 int temp[GRID_SIZE][GRID_SIZE];

typedef struct {
    int startrow;
    int endrow;
} thread_data_t;

void print_grid() {
   
        system("clear"); 
   
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (grid[i][j] == 1) {
                printf("# ");
            } else {
                printf("  ");
            }
        }
        printf("\n");
    }
    usleep(500000);  
}

void initialize_grid() {
    
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j] = 0;
        }
    }
}

void initialize_patterns() {
    initialize_grid();

    grid[1][1] = 1;
    grid[1][2] = 1;
    grid[2][1] = 1;
    grid[2][2] = 1;

    grid[5][6] = 1;
    grid[6][6] = 1;
    grid[7][6] = 1;
   
    grid[10][10] = 1;
    grid[11][11] = 1;
    grid[12][9] = 1;
    grid[12][10] = 1;
    grid[12][11] = 1;
}

void* compute_next_gen(void* arg) {
    
    thread_data_t *data = (thread_data_t *)arg;
    int start = data->startrow;
    int end = data->endrow;

    for (int i = start; i <= end; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            int neighbors = 0; // live neighbors count

            // Check neighbors
            for (int n_row = -1; n_row <= 1; n_row++) {
                for (int n_col = -1; n_col <= 1; n_col++) {
                    if (n_row == 0 && n_col == 0) continue; // skip the same cell

/////////////////// neig in bounds
                    if ((i + n_row) >= 0 && (i + n_row) < GRID_SIZE && (j + n_col) >= 0 && (j + n_col) < GRID_SIZE) {
                        neighbors += grid[i + n_row][j + n_col]; // Count
                    }
                }
            }

            // rules of the game
            if (grid[i][j] == 1) { is alive
                if (neighbors < 2 || neighbors > 3) { // Underpopulation or overpopulation
                    temp[i][j] = 0;
                } else {
                    temp[i][j] = 1;
                }
            } else { // is dead
                if (neighbors == 3) { // Reproduction
                    temp[i][j] = 1;
                } else {
                    temp[i][j] = 0;
                }
            }
        }
    }

    // Synchronize threads 
    pthread_barrier_wait(&barrier);

    // Update the main grid from temp 
    for (int i = start; i <= end; i++) {
        memcpy(grid[i], temp[i], GRID_SIZE * sizeof(int)); 
    }

    return NULL; // Exit
}


int main() {
    initialize_patterns(); 

    pthread_t threads[NUM_THREADS]; 
    thread_data_t thread_data[NUM_THREADS];
    pthread_barrier_init(&barrier, NULL, NUM_THREADS); // Initialize the barrier


    for (int gen = 0; gen < GENERATIONS; gen++) {
//////////  Create threads to calc next gen
        for (int i = 0; i < NUM_THREADS; i++) {
            thread_data[i].startrow = i * (GRID_SIZE / NUM_THREADS);
            thread_data[i].endrow = (i == NUM_THREADS - 1) ? GRID_SIZE - 1 : (i + 1) * (GRID_SIZE / NUM_THREADS) - 1;
            pthread_create(&threads[i], NULL, compute_next_gen, (void*)&thread_data[i]);
        }
        // Wait for threads execution
        for (int i = 0; i < NUM_THREADS; i++) {
            pthread_join(threads[i], NULL);
        }
        print_grid(); 
    }

    pthread_barrier_destroy(&barrier);

    return 0;
}
