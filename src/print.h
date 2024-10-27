#ifndef PRINT_H
#define PRINT_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
// Define LOGFILE for logging purposes
// Uncomment and define LOGFILE as needed
#define LOGFILE "../log/log.txt"
#define TIME_LEN 150
#define BUFFER_SIZE 4096

// Enum for data types
typedef enum {
    TYPE_STRING,
    TYPE_INT,
    TYPE_DOUBLE,
    TYPE_CHAR,
    TYPE_POINTER
} DataType;

// Linked list node structure
typedef struct Node {
    DataType type;
    void *data;
    struct Node *next;
} Node;

// Function declarations
void initLogFile();
void closeLogFile();
void logToFile(const char *format, ...);
Node* createNode(DataType type, void *data);
void freeList(Node *head);
void printSend(Node *head, const char *end, const char *delim);
Node* addArgument(Node **head, Node **tail, DataType type, void *data);
void printWithDelim(const char *format, const char *end, const char *delim, va_list args);
void print(const char *format, ...);
void prints(const char *format, const char *end, const char *delim, ...);
void printi(int count, ...);

// Single argument print functions
void print1s(const char *str);
void print1ss(const char *str);
void print1d(int i);
void print1ds(int n);
void print1f(double f);
void print1fs(double f);
void print1c(char c);
void print1cs(char c);
void print1p(void *p);
void print1ps(void *p);

int printw(const char *format, ...);

#endif // PRINT_H