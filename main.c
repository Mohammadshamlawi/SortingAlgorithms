#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"
#pragma ide diagnostic ignored "misc-no-recursion"

#include <stdio.h>
#include "stdlib.h"
#include "time.h"
#include "sys/time.h"
#include "mysql.h"
#include "string.h"
#include "pthread.h"
#include "math.h"

void *executeAlgorithms(void *);

char selectionSort(u_int64[], int);

char bubbleSort(u_int64[], int);

char improvedBubbleSort(u_int64[], int);

char insertionSort(u_int64[], int);

char mergeSort(u_int64[], int);

char quickSort(u_int64[], int, int);

char heapSort(u_int64[], int);

char countingSort(u_int64[], int);

char cocktailSort(u_int64[], int);

char pancakeSort(u_int64[], int);

char gnomeSort(u_int64[], int);

char stoogeSort(u_int64[], int, int);

char oddEvenSort(u_int64[], int);

char mergeSort3Way(u_int64[], int);

void parseArgs(int argc, char *argv[]);

double microtime();

__attribute__((unused)) void printArray(const u_int64[], u_int64);

void swap(u_int64 *, u_int64 *);

void heapify(u_int64[], int, int);

int numOfDigits(int);

int numOfDigitsf(double);

__attribute__((unused)) int qSortCompare(const void *, const void *);

void run_query(MYSQL *, const char *);

enum net_async_status run_query_non_blocking(MYSQL *, const char *);

MYSQL *init_mysql_connection(char);

void validate_db_exists(MYSQL *);

char db_name[64] = "sorting", table_name[64] = "numbers", col_name[64] = "number";

struct timeval t;
double start;

const void *algorithms[] = {
        selectionSort,
        bubbleSort,
        improvedBubbleSort,
        insertionSort,
        mergeSort,
        heapSort,
//        countingSort,
        cocktailSort,
        pancakeSort,
        gnomeSort,
        oddEvenSort,
        mergeSort3Way
};
const void *l_h_algorithms[] = {quickSort, stoogeSort};
const char *algorithm_names[] = {
        "Selection Sort",
        "Bubble Sort",
        "Improved Bubble Sort",
        "Insertion Sort",
        "Merge Sort",
        "Heap Sort",
//        "Counting Sort",
        "Cocktail Sort",
        "Pancake Sort",
        "Gnome Sort",
        "Odd-Even Sort",
        "3 Way Merge Sort",
        "Quick Sort",
        "Stooge Sort"
};

//int sizes[] = {1000, 10000, 100000, 1000000, 10000000};
const int sizes[] = {1000, 10000};
const int iterations = 10;

enum thread_status {
    CREATED,
    NOT_CREATED,
    IN_PROGRESS,
    FINISHED,
    THREAD_FAILED
};

// Thread Shared
MYSQL *conn;
u_int64 **random, **sorted, **nearly_sorted, **reverse_sorted, **buffer, *counts;
const void *approach[] = {&random, &sorted, &nearly_sorted, &reverse_sorted};
const char *approach_name[] = {"Random", "Sorted", "Nearly Sorted", "Reverse Sorted"},
        base_query[] = "INSERT INTO history(`name`,`size`,`test`,`success`,`time(ms)`) VALUES";
struct thread {
    unsigned int index;
    pthread_t id;
    enum thread_status status;
} *threads;

pthread_mutex_t pMutex;

int query_size = sizeof(base_query);
const int sizes_count = sizeof(sizes) / sizeof(sizes[0]),
        algorithm_count = sizeof(algorithms) / sizeof(algorithms[0]),
        l_h_algorithm_count = sizeof(l_h_algorithms) / sizeof(l_h_algorithms[0]),
        total_algorithms = algorithm_count + l_h_algorithm_count + 1,
        approach_count = sizeof(approach_name) / sizeof(approach_name[0]);


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-non-prototype"

int main(int argc, char *argv[]) {

    parseArgs(argc, argv);

    srand(time(0)); // NOLINT(*-msc51-cpp)

    if (pthread_mutex_init(&pMutex, NULL) != 0) {
        printf("\nMutex init has failed\n");
        return 1;
    }

    conn = init_mysql_connection(0);

    run_query(conn, "DROP TABLE IF EXISTS `history`");
    run_query(conn,
              "CREATE TABLE history (id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,name VARCHAR(255) NOT NULL,`size` INT NOT NULL,test VARCHAR(255) NOT NULL,success TINYINT(1) UNSIGNED NOT NULL DEFAULT 0,`time(ms)` DOUBLE NOT NULL)");

    MYSQL_RES *result;

    printf("Caching data...");

    random = calloc(sizes_count, sizeof(u_int64 *));
    sorted = calloc(sizes_count, sizeof(u_int64 *));
    nearly_sorted = calloc(sizes_count, sizeof(u_int64 *));
    reverse_sorted = calloc(sizes_count, sizeof(u_int64 *));
    buffer = calloc(sizes_count, sizeof(u_int64 *));
    counts = calloc(sizes_count, sizeof(u_int64));

    for (int i = 0; i < sizes_count; ++i) {
        char query[500];
        sprintf(query, "SELECT `%s` FROM %s ORDER BY RAND() LIMIT %d", col_name, table_name, sizes[i]);
        run_query(conn, query);

        result = mysql_store_result(conn);
        counts[i] = mysql_num_rows(result);

        random[i] = calloc(counts[i], sizeof(u_int64));
        sorted[i] = calloc(counts[i], sizeof(u_int64));
        nearly_sorted[i] = calloc(counts[i], sizeof(u_int64));
        reverse_sorted[i] = calloc(counts[i], sizeof(u_int64));
        buffer[i] = calloc(counts[i], sizeof(u_int64));

        for (int j = 0; j < counts[i]; ++j) random[i][j] = strtoull(*mysql_fetch_row(result), NULL, 10);

        memcpy(sorted[i], random[i], sizeof(random[i]) * counts[i]);
        qsort(sorted[i], counts[i], sizeof(sorted[i]), qSortCompare);

        memcpy(nearly_sorted[i], sorted[i], sizeof(sorted[i]) * counts[i]);
        memcpy(reverse_sorted[i], sorted[i], sizeof(sorted[i]) * counts[i]);
        for (int j = 0; j < counts[i] / 2; ++j) swap(&reverse_sorted[i][j], &reverse_sorted[i][counts[i] - (j + 1)]);
        for (int j = 0; j + 1 < counts[i]; j += 10) swap(&nearly_sorted[i][j], &nearly_sorted[i][j + 1]);
    }

    query_size += ((int) sizeof("'C Quick Sort'") * sizes_count * approach_count) +
                  (1 * (sizes_count * approach_count * total_algorithms - 1)) +
                  (sizes_count * approach_count * total_algorithms) + 1;
    for (int i = 0; i < sizes_count; ++i)
        query_size += (numOfDigits(sizes[i]) * sizes_count * approach_count * total_algorithms);
    for (int i = 0; i < approach_count; ++i)
        query_size += (((int) strlen(approach_name[i]) + 2) * sizes_count * approach_count * total_algorithms);
    for (int i = 0; i < total_algorithms - 1; ++i)
        query_size += (((int) sizeof(algorithm_names[i]) + 2) * sizes_count * approach_count);

    printf("\nDone\n");
    printf("Creating threads...");

    threads = calloc(iterations, sizeof(struct thread));
    for (int i = 0; i < iterations; ++i) {
        threads[i].status = CREATED;
        threads[i].index = i;
        if (pthread_create(&(threads[i].id), NULL, executeAlgorithms, &(threads[i].index)) != 0) {
            threads[i].status = NOT_CREATED;
        }
    }
    printf("\nDone\n");
    printf("Executing...");

    for (int i = 0; i < iterations; ++i) {
        if (threads[i].status != NOT_CREATED) pthread_join(threads[i].id, NULL);
    }
    mysql_close(conn);
    pthread_mutex_destroy(&pMutex);

    printf("\nDone\n");

    return 0;
}

void *executeAlgorithms(void *arg) {
    int *index = arg;
    u_int64 query = query_size;
    double (*run_times)[sizes_count][approach_count] =
            calloc(sizes_count * approach_count * total_algorithms, sizeof(*run_times));
    char (*success_sort)[sizes_count][approach_count] =
            calloc(sizes_count * approach_count * total_algorithms, sizeof(*success_sort));

    threads[*index].status = IN_PROGRESS;

    for (int i = 0; i < sizes_count; ++i) { // 1000, 20000, 50000, 100000, 500000, 1000000
        for (int j = 0; j < approach_count; ++j) {
            memcpy(buffer[i], ((void ***) approach[j])[0][i], sizeof(((void ***) approach[j])[0][i]) * counts[i]);
            start = microtime();
            qsort(buffer[i], counts[i], sizeof(buffer[i]), qSortCompare);
            run_times[i][j][total_algorithms - 1] = (microtime() - start) * 1000; // ms
            success_sort[i][j][total_algorithms - 1] = 1;
            query += (numOfDigitsf(run_times[i][j][total_algorithms - 1]));

            for (int l = 0; l < algorithm_count; ++l) {
                memcpy(buffer[i], ((void ***) approach[j])[0][i], sizeof(((void ***) approach[j])[0][i]) * counts[i]);
                start = microtime();
                char res = ((char (*)()) algorithms[l])(buffer[i], counts[i]);
                run_times[i][j][l] = res == 0 ? -1 : (microtime() - start) * 1000; // ms
                success_sort[i][j][l] =
                        memcmp(sorted[i], buffer[i], sizeof(sorted[i]) * counts[i]) == 0 ? 1 : 0;
                query += (numOfDigitsf(run_times[i][j][l]));
            }

            for (int l = 0; l < l_h_algorithm_count; ++l) {
                memcpy(buffer[i], ((void ***) approach[j])[0][i], sizeof(((void ***) approach[j])[0][i]) * counts[i]);
                start = microtime();
                char res = ((char (*)()) l_h_algorithms[l])(buffer[i], 0, counts[i] - 1);
                run_times[i][j][l + algorithm_count] = res == 0 ? -1 : (microtime() - start) * 1000; // ms
                success_sort[i][j][l + algorithm_count] =
                        memcmp(sorted[i], buffer[i], sizeof(sorted[i]) * counts[i]) == 0 ? 1 : 0;
                query += (numOfDigitsf(run_times[i][j][l + algorithm_count]));
            }
        }
    }

    char *insert_query = calloc(query, sizeof(char));
    if (insert_query != NULL) {
        memcpy(insert_query, base_query, sizeof(base_query));
        for (int i = 0; i < sizes_count; ++i) {
            for (int j = 0; j < approach_count; ++j) {
                for (int k = 0; k < total_algorithms; ++k) {
                    sprintf(insert_query, "%s('%s',%d,'%s',%d,%f),", insert_query,
                            k == (total_algorithms - 1) ? "C Quick Sort" : algorithm_names[k],
                            sizes[i], approach_name[j], success_sort[i][j][k], run_times[i][j][k]);
                }
            }
        }

        size_t query_length = strlen(insert_query);
        if (insert_query[query_length - 1] == ',') insert_query[query_length - 1] = '\0';

        pthread_mutex_lock(&pMutex);

        run_query(conn, insert_query);

        pthread_mutex_unlock(&pMutex);
    }

    threads[*index].status = FINISHED;

    pthread_exit(NULL);
    return NULL;
}

#pragma clang diagnostic pop

/// Algorithms
char selectionSort(u_int64 arr[], int n) {
    for (int i = 0, min = i; i < n - 1; ++i, min = i) {
        for (int j = i + 1; j < n; ++j)
            if (arr[j] < arr[min]) min = j;

        if (min != i) swap(&arr[min], &arr[i]);
    }

    return 1;
}

char bubbleSort(u_int64 arr[], int n) {
    for (int i = 0; i < n - 1; ++i) {
        for (int j = 0; j < n - i - 1; ++j) {
            if (arr[j] > arr[j + 1]) swap(&arr[j], &arr[j + 1]);
        }
    }

    return 1;
}

char improvedBubbleSort(u_int64 arr[], int n) {
    char done = 0;
    for (int i = 0; i < n - 1; ++i) {
        for (int j = 0; j < n - i - 1; ++j) {
            if (arr[j] > arr[j + 1]) {
                swap(&arr[j], &arr[j + 1]);
                done = 1;
            }
        }

        if (!done) break;
    }

    return 1;
}

char insertionSort(u_int64 arr[], int n) {
    for (u_int64 i = 1, j, key = arr[i]; i < n; ++i, key = arr[i]) {
#pragma clang diagnostic push
#pragma ide diagnostic ignored "ConstantConditionsOC"
        for (j = i - 1; j >= 0 && arr[j] > key; --j) arr[j + 1] = arr[j];
#pragma clang diagnostic pop
        arr[j + 1] = key;
    }

    return 1;
}

char mergeSort(u_int64 arr[], int n) {
    if (n < 2) return 0;

    int mid = n / 2, right_size = n - mid;
    u_int64 left[mid], right[right_size];
    for (int i = 0; i < mid; ++i) left[i] = arr[i];
    for (int i = mid; i < n; ++i) right[i - mid] = arr[i];

    mergeSort(left, mid);
    mergeSort(right, right_size);

    int i = 0, j = 0, k = 0;
    for (; i < mid && j < right_size; ++k) arr[k] = left[i] <= right[j] ? left[i++] : right[j++];
    for (; i < mid; ++i, ++k) arr[k] = left[i];
    for (; j < right_size; ++j, ++k) arr[k] = right[j];

    return 1;
}

char quickSort(u_int64 arr[], int l, int r) {
    if (l < r) {
        int i = l - 1;
        for (u_int64 j = l, pivot = arr[r]; j <= r; ++j) {
            if (arr[j] < pivot) swap(&arr[++i], &arr[j]);
        }
        swap(&arr[i + 1], &arr[r]);
        quickSort(arr, l, ++i - 1);
        quickSort(arr, i + 1, r);
    }

    return 1;
}

char heapSort(u_int64 arr[], int n) {
    for (int i = (n / 2) - 1; i >= 0; --i) heapify(arr, n, i);

    for (int i = n - 1; i >= 0; --i) {
        swap(&arr[0], &arr[i]);
        heapify(arr, i, 0);
    }

    return 1;
}

// Does not accept negative values.
char countingSort(u_int64 arr[], int n) {
    u_int64 max = arr[0], output[n];
    for (int i = 1; i < n; ++i) if (max < arr[i]) max = arr[i];

    u_int64 *count = calloc(max + 1, sizeof(u_int64));
    if (count == NULL) return 0;
    for (int i = 0; i < n; ++i) count[arr[i]]++;
    for (int i = 1; i <= max; ++i) count[i] += count[i - 1];

    for (int i = n - 1; i >= 0; --i) {
        if (n <= (count[arr[i]] - 1)) return 0;
        output[count[arr[i]] - 1] = arr[i];
        count[arr[i]]--;
    }

    for (int i = 0; i < n; ++i) arr[i] = output[i];

    return 1;
}

char cocktailSort(u_int64 arr[], int n) {
    char swapped = 1;
    for (int i = 0, end = n - 1; swapped; ++i) {
        swapped = 0;
        for (int j = i; j < end; ++j) {
            if (arr[j] > arr[j + 1]) {
                swap(&arr[j], &arr[j + 1]);
                swapped = 1;
            }
        }

        if (!swapped) break;
        swapped = 0;

        for (int j = --end - 1; j >= i; --j) {
            if (arr[j] > arr[j + 1]) {
                swap(&arr[j], &arr[j + 1]);
                swapped = 1;
            }
        }
    }

    return 1;
}

char pancakeSort(u_int64 arr[], int n) {
    for (int size = n, mi = 0; size > 1;) {
        for (int i = 0; i < size; ++i) if (arr[i] > arr[mi]) mi = i;

        if (mi != --size) {
            for (int i = mi, s = 0; s < i; ++s, --i)
                swap(&arr[s], &arr[i]);

            for (int i = size, s = 0; s < i; ++s, --i)
                swap(&arr[s], &arr[i]);
        }
    }

    return 1;
}

char gnomeSort(u_int64 arr[], int n) {
    for (int i = 0; i < n;) {
        if (i == 0) i++;

        if (arr[i] >= arr[i - 1]) i++;
        else {
            swap(&arr[i], &arr[i - 1]);
            i--;
        }
    }

    return 1;
}

char stoogeSort(u_int64 arr[], int l, int h) {
    if (l >= h) return 0;
    if (arr[l] > arr[h]) swap(&arr[l], &arr[h]);

    if ((h - l + 1) > 2) {
        int a = (h - l + 1) / 3;
        stoogeSort(arr, l, h - a);
        stoogeSort(arr, l + a, h);
        stoogeSort(arr, l, h - a);
    }

    return 1;
}

char oddEvenSort(u_int64 arr[], int n) {
    for (char sorted_bool = 0; !sorted_bool;) {
        sorted_bool = 1;
        for (int i = 1; i <= n - 2; i += 2) {
            if (arr[i] > arr[i + 1]) {
                swap(&arr[i], &arr[i + 1]);
                sorted_bool = 0;
            }
        }

        for (int i = 0; i <= n - 2; i += 2) {
            if (arr[i] > arr[i + 1]) {
                swap(&arr[i], &arr[i + 1]);
                sorted_bool = 0;
            }
        }
    }

    return 1;
}

char mergeSort3Way(u_int64 arr[], int n) {
    if (n < 2) return 0;
    else if (n == 2) {
        mergeSort(arr, n);
        return 0;
    }

    int mid1 = n / 3, mid2 = 2 * mid1;
    u_int64 left[mid1], middle[mid2 - mid1], right[n - mid2];
    for (int i = 0; i < mid1; ++i) left[i] = arr[i];
    for (int i = mid1; i < mid2; ++i) middle[i - mid1] = arr[i];
    for (int i = mid2; i < n; ++i) right[i - mid2] = arr[i];

    mergeSort3Way(left, mid1);
    mergeSort3Way(middle, mid2 - mid1);
    mergeSort3Way(right, n - mid2);

    int i = 0, j = 0, k = 0, l = 0;
    while (i < mid1 && j < (mid2 - mid1) && k < (n - mid2)) {
        arr[l++] = (left[i] <= middle[j] && left[i] <= right[k])
                   ? left[i++]
                   : ((middle[j] <= left[i] && middle[j] <= right[k]) ? middle[j++] : right[k++]);
    }

    while (i < mid1 && j < (mid2 - mid1)) arr[l++] = left[i] <= middle[j] ? left[i++] : middle[j++];
    while (i < mid1 && k < (n - mid2)) arr[l++] = left[i] <= right[k] ? left[i++] : right[k++];
    while (j < (mid2 - mid1) && k < (n - mid2)) arr[l++] = middle[j] <= right[k] ? middle[j++] : right[k++];

    while (i < mid1) arr[l++] = left[i++];
    while (j < mid2 - mid1) arr[l++] = middle[j++];
    while (k < n - mid2) arr[l++] = right[k++];

    return 1;
}
// END Algorithms

/// Utilities
void parseArgs(int argc, char *argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (strcasecmp(argv[i], "--help") == 0 || strcasecmp(argv[i], "-h") == 0) {
            printf("Usage: SortingAlgorithms.exe [OPTIONS]...\n");
            printf("Run sorting algorithms using the provided database rows.\n\n");
            printf("--db, -d\tSpecify the database to pull data from. DEFAULT:[%s]\n", db_name);
            printf("--table, -t\tSpecify the table to pull data from. DEFAULT:[%s]\n", table_name);
            printf("--column, -c\tSpecify the table column to pull data from. DEFAULT:[%s]\n", col_name);
            exit(0);
        }

        if (strcasecmp(argv[i], "--db") == 0 || strcasecmp(argv[i], "-d") == 0) {
            i++;
            int j = 0;
            for (; argv[i][j] != '\0' && j <= 63; ++j) db_name[j] = argv[i][j];
            for (; db_name[j] != '\0' && j <= 63; ++j) db_name[j] = '\0';
            continue;
        }

        if (strcasecmp(argv[i], "--table") == 0 || strcasecmp(argv[i], "-t") == 0) {
            i++;
            int j = 0;
            for (; argv[i][j] != '\0' && j <= 63; ++j) table_name[j] = argv[i][j];
            for (; table_name[j] != '\0' && j <= 63; ++j) table_name[j] = '\0';
            continue;
        }

        if (strcasecmp(argv[i], "--column") == 0 || strcasecmp(argv[i], "-c") == 0) {
            i++;
            int j = 0;
            for (; argv[i][j] != '\0' && j <= 63; ++j) col_name[j] = argv[i][j];
            for (; col_name[j] != '\0' && j <= 63; ++j) col_name[j] = '\0';
            continue;
        }
    }
}

double microtime() {
    gettimeofday(&t, NULL);
//    1692712422.9379
    return (double) t.tv_sec + ((double) t.tv_usec / 1000000.0);
}

__attribute__((unused)) void printArray(const u_int64 arr[], u_int64 n) {
    for (int i = 0; i < n; ++i)
        printf("%llu ", arr[i]);
    printf("\n");
}

void swap(u_int64 *first, u_int64 *second) {
    u_int64 temp = *first;
    *first = *second;
    *second = temp;
}

int numOfDigits(int x) {
    return (int) floor(log10(abs(x))) + 1;
}

int numOfDigitsf(double x) {
    if ((int) x == 0) return 0;
    return (int) floor(log10(fabs(x))) + 7;
}

void heapify(u_int64 arr[], int n, int i) {
    int left = (2 * i) + 1, right = left + 1,
            largest = (left < n && arr[left] > arr[i])
                      ? (right < n && arr[right] > arr[left]) ? right : left
                      : (right < n && arr[right] > arr[i]) ? right : i;

    if (largest != i) {
        swap(&arr[i], &arr[largest]);
        heapify(arr, n, largest);
    }
}

__attribute__((unused)) int qSortCompare(const void *a, const void *b) {
    return (int) (*(u_int64 *) a - *(u_int64 *) b);
}
// END Utilities

/// Database
void run_query(MYSQL *connection, const char *query) {
    if (mysql_query(connection, query) != 0) {
        printf("mysql_query(%s) failed: %d %s\n", query, mysql_errno(connection), mysql_error(connection));
        mysql_close(connection);
        exit(1);
    }
}

//enum net_async_status run_query_non_blocking(MYSQL *conn, const char *query) {
//    enum net_async_status status;
//    while ((status = mysql_real_query_nonblocking(conn, query, strlen(query))) == NET_ASYNC_NOT_READY);
//    return status;
//}

MYSQL *init_mysql_connection(char is_thread) {
    MYSQL *connection = mysql_init(NULL);
    if (connection == NULL) {
        printf("Connection not established.");
        exit(1);
    }

    if (mysql_real_connect(connection, "localhost", "root", "root", NULL, 3306, NULL, 0) == NULL) {
        printf("mysql_real_connect() failed: %s\n", mysql_error(connection));
        mysql_close(connection);
        exit(1);
    }

    validate_db_exists(connection);

    return connection;
}

void validate_db_exists(MYSQL *connection) {
    char query[500];
//    Database Validation
    sprintf(query, "SELECT SCHEMA_NAME FROM information_schema.SCHEMATA WHERE SCHEMA_NAME = '%s'", db_name);
    run_query(connection, query);

    MYSQL_RES *result = mysql_store_result(connection);
    u_int64 count = mysql_num_rows(result);
    u_int64 field_count = mysql_field_count(connection);

    if (count == 0 && field_count != 0) {
        printf("Database does not exist.\n");
        printf("Please create database \"%s\" before proceeding.", db_name);
        exit(0);
    }

    if (mysql_select_db(connection, db_name) != 0) {
        printf("mysql_select_db(%s) failed: %d %s\n", db_name, mysql_errno(connection), mysql_error(connection));
        mysql_close(connection);
        exit(1);
    }

//    Table Validation
    sprintf(query,
            "SELECT TABLE_SCHEMA, TABLE_NAME FROM information_schema.TABLES WHERE TABLE_SCHEMA = '%s' AND TABLE_NAME = '%s'",
            db_name, table_name);
    run_query(connection, query);

    result = mysql_store_result(connection);
    count = mysql_num_rows(result);
    field_count = mysql_field_count(connection);

    if (count == 0 && field_count != 0) {
        printf("Table `%s` does not exist.\n", table_name);
        printf("Please create table \"%s\" before proceeding.", table_name);
        exit(0);
    }

//    Column Validation
    sprintf(query, "SELECT TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME FROM information_schema.COLUMNS "
                   "WHERE TABLE_SCHEMA = '%s' AND TABLE_NAME = '%s' AND COLUMN_NAME = '%s'",
            db_name, table_name, col_name);
    run_query(connection, query);

    result = mysql_store_result(connection);
    count = mysql_num_rows(result);
    field_count = mysql_field_count(connection);

    if (count == 0 && field_count != 0) {
        printf("Column `%s` does not exist.\n", col_name);
        printf("Please create column \"%s\" before proceeding.", col_name);
        exit(0);
    }
}
// END Database

#pragma clang diagnostic pop