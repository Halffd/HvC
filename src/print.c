#include "print.h"

#ifdef LOGFILE
FILE *logFile;

char timeBuffer[TIME_LEN];
time_t currentTime;
struct tm *local_time;

// Function to initialize the log file
void initLogFile() {
    logFile = fopen(LOGFILE, "a");
    if (!logFile) {
        perror("Failed to open log file");
        exit(EXIT_FAILURE);
    }
}

// Function to close the log file
void closeLogFile() {
    if (logFile) {
        fclose(logFile);
    }
}

// Function to log messages to the log file
void logToFile(const char *format, ...) {
    if (!logFile) {
        logFile = fopen(LOGFILE, "a");
        if (!logFile) {
            perror("Failed to open log file");
            return;
        }
        fprintf(logFile, "\n--------------------\n");
    }

    currentTime = time(NULL);
    // converte para a hora local
    local_time = localtime(&currentTime);


    // Get current time in milliseconds
    struct timeval tp;
    gettimeofday(&tp, NULL);
    long int current_ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;

    // Get the known time in milliseconds
    long int known_ms = currentTime * 1000; // Convert to milliseconds

    // Calculate the difference in milliseconds
    long int diff_ms = current_ms - known_ms;

    // Convert the difference to seconds as float
    float diff_seconds = diff_ms / 1000.0f;
    // Format the time as "dd/mm/yyyy hh:mm:ss"
    strftime(timeBuffer, TIME_LEN, "%d/%m/%Y %H:%M:", local_time);
    char secBuf[3];
    strftime(secBuf, TIME_LEN, "%S", local_time);
    float sec = atoi(secBuf) + diff_seconds;
    fprintf(logFile, "%s%06.3f : ", timeBuffer, sec); // Add a newline for better formatting
    // Handle variable arguments
    va_list args;
    va_start(args, format);
    vfprintf(logFile, format, args);
    va_end(args);
    fflush(logFile); // Ensure the output is flushed to the file
}
#endif

// Function to create a new node
Node* createNode(DataType type, void *data) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (!newNode) {
        perror("Failed to allocate memory for node");
        return NULL;
    }
    newNode->type = type;
    newNode->data = data;
    newNode->next = NULL;
    return newNode;
}

// Function to free the linked list
void freeList(Node *head) {
    Node *tmp = head;
    while (tmp != NULL) {
        Node *next = tmp->next;

        if (tmp->data) {
            // Free dynamically allocated memory for int, double, char but not for string literals or pointer values
            if (tmp->type == TYPE_INT || tmp->type == TYPE_DOUBLE || tmp->type == TYPE_CHAR) {
                free(tmp->data);  // Free memory only if it was dynamically allocated
            }
        }
        free(tmp);  // Free the node itself
        tmp = next;
    }
}
// Function to print and log the linked list
void printSend(Node *head, const char *end, const char *delim) {
    int first = 1;
    if (head == NULL) return; // Early return if head is NULL
    Node *current = head;

    while (current) {
        if (current == NULL || current->data == NULL) return; // Early return if data is NULL
        
        if (!first) {
            printf("%s", delim);
            logToFile("%s", delim);
        }
        
        switch (current->type) {
            case TYPE_STRING:
                printf("%s", (char *)current->data);
                logToFile("%s", (char *)current->data);
                break;
            case TYPE_INT:
                printf("%d", *(int *)current->data);
                logToFile("%d", *(int *)current->data);
                break;
            case TYPE_DOUBLE:
                printf("%f", *(double *)current->data);
                logToFile("%f", *(double *)current->data);
                break;
            case TYPE_CHAR:
                printf("%c", *(char *)current->data);
                logToFile("%c", *(char *)current->data);
                break;
            case TYPE_POINTER:
                printf("%p", current->data); // If you want to print the pointer's address
                logToFile("%p", current->data);
                break;
            default:
                printf("Unknown type");
                logToFile("Unknown type");
                break;
        }

        first = 0;
        current = current->next;
    }
    printf("%s", end);
    logToFile("%s", end);
}

// Unified function for node creation and handling
Node* addArgument(Node **head, Node **tail, DataType type, void *data) {
    Node *newNode = createNode(type, data);
    if (!newNode) return NULL;

    if (*head == NULL) {
        *head = newNode;
        *tail = newNode;
    } else {
        (*tail)->next = newNode;
        *tail = newNode;
    }
    return newNode;
}

// Unified print function
void printWithDelim(const char *format, const char *end, const char *delim, va_list args) {
    Node *head = NULL;
    Node *tail = NULL;

    while (*format) {
        void *data = NULL;
        switch (*format) {
            case 's':
                data = va_arg(args, char *);
                addArgument(&head, &tail, TYPE_STRING, data); // No need to copy the string
                break;
            case 'd': {
                int *i = malloc(sizeof(int));
                if (i) { // Check if malloc succeeded
                    *i = va_arg(args, int);
                    addArgument(&head, &tail, TYPE_INT, i);
                }
                break;
            }
            case 'f': {
                double *d = malloc(sizeof(double));
                if (d) { // Check if malloc succeeded
                    *d = va_arg(args, double);
                    addArgument(&head, &tail, TYPE_DOUBLE, d);
                }
                break;
            }
            case 'c': {
                char *c = malloc(sizeof(char));
                if (c) { // Check if malloc succeeded
                    *c = (char)va_arg(args, int);
                    addArgument(&head, &tail, TYPE_CHAR, c);
                }
                break;
            }
            case 'p': {
                void *ptr = va_arg(args, void *);
                addArgument(&head, &tail, TYPE_POINTER, ptr); // No need to copy pointer
                break;
            }
            default:
                fprintf(stderr, "Unknown format specifier: %c\n", *format);
                break;
        }
        format++;
    }

    printSend(head, end, delim);
    freeList(head);
}

// Convenience functions for specific print behaviors
void print(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printWithDelim(format, "\n", "", args);  // Pass va_list directly
    va_end(args);
}

void prints(const char *format, const char *end, const char *delim, ...) {
    va_list args;
    va_start(args, delim);
    printWithDelim(format, end, delim, args);  // Pass va_list directly
    va_end(args);
}

void printi(int count, ...) {
    va_list ap;
    va_start(ap, count);
    for (int i = 0; i < count; i++) {
        printf("%d ", va_arg(ap, int)); // Incorrect type specification
    }
    printf("\n");
    va_end(ap);
}

// Single argument print functions
void print1s(const char *str) {
    print("s", str);
}

void print1ss(const char *str) {
    prints("s", "\n", " ", str);
}

void print1d(int i) {
    print("d", i);
}

void print1ds(int n) {
    prints("d", "\n", " ", n);
}

void print1f(double f) {
    print("f", f);
}

void print1fs(double f) {
    prints("f", "\n", " ", f);
}

void print1c(char c) {
    print("c", c);
}

void print1cs(char c) {
    prints("c", "\n", " ", c);
}

void print1p(void *p) {
    print("p", p);
}

void print1ps(void *p) {
    prints("p", "\n", " ", p);
}

int printw(const char *format, ...) {
    char buffer[BUFFER_SIZE]; // Buffer for the formatted string

    va_list args;
    va_start(args, format);
    
    // Format the string into the buffer
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args); // Clean up the va_list

    // Print to stdout
    int result = printf("%s", buffer);
    
    // Log to file
    logToFile("%s", buffer);

    return result; // Return the result of printf
}
