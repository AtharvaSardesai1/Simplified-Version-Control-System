
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <string.h>

#define N 10
#define MAX_FILE_CONTENT_SIZE 1000

int nextFileID = 1; // Global variable to track the next available file ID

typedef struct File {
    int fileID;
    char content[MAX_FILE_CONTENT_SIZE];
    struct File* next;
} File;

File* head = NULL; // Global variable to store the head of the linked list

typedef struct commit {
    char message[100];
    int fileID;
    char author[50];
    char timestamp[20];
} commit;

typedef struct graphNode {
    struct commit* commit;
    struct graphNode* parent; // Parent in the directed acyclic graph
    struct graphNode* nextParent; // Next parent in the linked list of parents
} graphNode;

typedef struct repository {
    struct graphNode* nodes[N];
    struct file* fileHash[N]; // Array of linked lists for file storage
    int currentBranchIndex; // Index of the current branch in the branch array
    int branchCount; // Total number of branches
    char* branches[N]; // Array to store branch names
} repository;



//-----------------FUNCTIONS--------------------------------------------
// Function to print the changes between two files

graphNode* createGraphNode(commit* commit) {
    graphNode* newNode = (graphNode*)malloc(sizeof(graphNode));
    newNode->commit = commit;
    newNode->parent = NULL;
    newNode->nextParent = NULL;

    return newNode;
}

void addParent(graphNode* child, graphNode* parent) {
    // Add parent to the beginning of the linked list
    parent->nextParent = child->parent;
    child->parent = parent;
}

void printFileChanges(const char* originalFileName, const char* updatedFileName) {
    FILE* originalFile = fopen(originalFileName, "r");
    FILE* updatedFile = fopen(updatedFileName, "r");

    if (originalFile == NULL || updatedFile == NULL) {
        printf("Error: Unable to open file.\n");
        return;
    }

    printf("Changes between %s and %s:\n", originalFileName, updatedFileName);

    char originalLine[MAX_FILE_CONTENT_SIZE];
    char updatedLine[MAX_FILE_CONTENT_SIZE];
    int lineNum = 1;
    int differencesFound = 0; // Flag to track if any differences were found

    // Compare lines of both files until one of them reaches the end
    while (fgets(originalLine, sizeof(originalLine), originalFile) != NULL &&
           fgets(updatedLine, sizeof(updatedLine), updatedFile) != NULL) {
        // Compare lines
        if (strcmp(originalLine, updatedLine) != 0) {
            printf("Difference found in line %d:\n", lineNum);
            printf("Original: %s", originalLine);
            printf("Updated: %s", updatedLine);
            printf("\n");
        }
        lineNum++;
    }

    // If one file has more lines than the other, print the remaining lines
    while (fgets(originalLine, sizeof(originalLine), originalFile) != NULL) {
        printf("Difference found in line %d:\n", lineNum);
        printf("Original: %s", originalLine);
        printf("Updated: (End of file)\n");
        printf("\n");
        differencesFound = 1;
        lineNum++;
    }

    while (fgets(updatedLine, sizeof(updatedLine), updatedFile) != NULL) {
        printf("Difference found in line %d:\n", lineNum);
        printf("Original: (End of file)\n");
        printf("Updated: %s", updatedLine);
        printf("\n");
        differencesFound = 1;
        lineNum++;
    }

    // If no differences were found, print a message
    if (!differencesFound) {
        printf("No differences found between %s and %s.\n", originalFileName, updatedFileName);
    }


    // Close files
    fclose(originalFile);
    fclose(updatedFile);
}

commit* commit_file(const char* fileName, const char* message, int id, const char* author) {
    FILE* file = fopen(fileName, "r");
    if (file == NULL) {
        printf("Error: Unable to open file.\n");
        return NULL;
    }
    printf("22222222\n");

    // Allocate memory for the new file node
    File* newFile = (File*)malloc(sizeof(File));
    if (newFile == NULL) {
        printf("Error: Memory allocation failed.\n");
        fclose(file);
        return NULL;
    }

    // Initialize file ID and content
    newFile->fileID = nextFileID++;
    newFile->next = NULL;
    newFile->content[0] = '\0'; // Initialize content to an empty string

    printf("4444444\n");
    // Read file content line by line and concatenate to newFile->content
    char line[MAX_FILE_CONTENT_SIZE];
    while (fgets(line, sizeof(line), file) != NULL) {
        strcat(newFile->content, line);
    }

    // Store the new file in the linked list
    if (head == NULL) {
        head = newFile;
    } else {
        // Traverse to the end of the list
        File* temp = head;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        // Append the new file
        temp->next = newFile;
    }
    printf("55555555\n");
    // Allocate memory for the new commit
    commit* newCommit = (commit*)malloc(sizeof(commit));
    if (newCommit == NULL) {
        printf("Error: Memory allocation failed.\n");
        return NULL;
    }
    printf("aaaaaaaaa\n");

    // Populate commit fields
    snprintf(newCommit->message, sizeof(newCommit->message), "%s", message);
    newCommit->fileID = newFile->fileID;
    snprintf(newCommit->author, sizeof(newCommit->author), "%s", author);
    printf("bbbbbbbbbb\n");

    // Generating a timestamp (for example purposes)
    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);
    strftime(newCommit->timestamp, sizeof(newCommit->timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    printf("cccccccccc\n");

    // Close file
    fclose(file);

    return newCommit;
}


// Function to print the contents of the linked list
void printFileList() {
    printf("File List:\n");
    File* temp = head;
    while (temp != NULL) {
        printf("File ID: %d\n", temp->fileID);
        printf("Content:\n%s\n", temp->content);
        temp = temp->next;
    }
}

int main() {
    char fileName[50];
    printf("Enter file name: ");
    scanf("%s", fileName);

    char message[100];
    printf("Enter commit message: ");
    scanf("%s", message); // Assuming single-word messages for simplicity

    // Assuming you have some way to get the author name, for example:
    const char* author = "John Doe"; 
 

    // Commit the original file with the provided message and author
    commit_file(fileName, message, nextFileID, author);
    printf("ggggggggg\n");

    // Modify the file content (assuming the file is modified externally)
    // Here, you can write code to modify the file content

    // Assume the modified file is saved with a different name
    char updatedFileName[50];
    printf("Enter updated file name: ");
    scanf("%s", updatedFileName);

    printf("Enter commit message: ");
    scanf("%s", message); // Assuming single-word messages for simplicity

    // Print the changes between the original and updated files
    printFileChanges(fileName, updatedFileName);

    // Commit the updated file
    commit_file(updatedFileName, message, nextFileID, author);

    // Print the contents of the linked list
    //printFileList();

    return 0;
}

