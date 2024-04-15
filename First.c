#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <string.h>

#define N 10
#define MAX_FILES_PER_COMMIT 5
#define MAX_FILE_CONTENT_SIZE 40000

// STRUCTURES

typedef struct file {
    int id; // Unique file ID
    char name[50];
    char content[MAX_FILE_CONTENT_SIZE];
    struct file* next; // Linked list pointer
} file;

typedef struct commit {
    char message[100];
    int id;
    char author[50];
    char timestamp[20];
    int fileCount;
    int fileIDs[MAX_FILES_PER_COMMIT];
} commit;

typedef struct commitNode {
    struct commit* commit;
    struct commitNode* children[N];
} commitNode;

typedef struct repository {
    struct commitNode* root;
    struct file* fileHash[N]; // Array of linked lists for file storage
} repository;

repository* initRepository() {
    repository* newRepo = (repository*)malloc(sizeof(repository));
    newRepo->root = NULL;

    // Initialize file hash array
    for (int i = 0; i < N; i++) {
        newRepo->fileHash[i] = NULL;
    }

    return newRepo;
}

// FUNCTIONS

commit* createCommit(const char* message, int id, const char* author) {
    commit* newCommit = (commit*)malloc(sizeof(commit));
    snprintf(newCommit->message, sizeof(newCommit->message), "%s", message);
    newCommit->id = id;
    snprintf(newCommit->author, sizeof(newCommit->author), "%s", author);

    // Generating a timestamp (for example purposes)
    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);
    strftime(newCommit->timestamp, sizeof(newCommit->timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    newCommit->fileCount = 0; // Initialize file count to zero

    return newCommit;
}

commitNode* createCommitNode(commit* commit) {
    commitNode* newNode = (commitNode*)malloc(sizeof(commitNode));
    newNode->commit = commit;

    for (int i = 0; i < N; i++) {
        newNode->children[i] = NULL;
    }

    return newNode;
}

// ...

void addFileToCommit(commit* commit, const char* name, const char* filePath, repository* repo) {
    if (commit->fileCount < MAX_FILES_PER_COMMIT) {
        // Create a new file ID
        int fileID = rand(); // Use a better method for real projects
        printf("Cleared 1\n");

        // Store the file ID in the commit structure
        commit->fileIDs[commit->fileCount] = fileID;
        printf("Cleared 2\n");

        // Read content from the file
        FILE* file = fopen(filePath, "r");
        if (file != NULL) {
            printf("File opened successfully.\n");

            size_t bytesRead = fread(repo->fileHash[fileID % N]->content, 1, MAX_FILE_CONTENT_SIZE, file);
            // if (bytesRead > 0) {
            //     repo->fileHash[fileID % N]->content[bytesRead] = '\0'; // Null-terminate the content
                printf("File content read successfully.\n");
            } else {
                perror("Error reading file content");
            }

            fclose(file);
        } else {
            perror("Error opening file");
            return;  // Exit the function if file cannot be opened
        }
        printf("Cleared 3\n");

        commit->fileCount++;

        printf("File added successfully. ID: %d\n", fileID);
    } else {
        printf("Maximum number of files per commit reached.\n");
    }
}

void displayCommit(commit* commit, repository* repo) {
    printf("Commit ID: %d\n", commit->id);
    printf("Author: %s\n", commit->author);
    printf("Message: %s\n", commit->message);
    printf("Timestamp: %s\n", commit->timestamp);
    printf("Files:\n");
    for (int i = 0; i < commit->fileCount; i++) {
        int fileID = commit->fileIDs[i];
        printf("  Name: file%d.txt\n", fileID);

        // Retrieve file content based on file ID from the repository's file hash array
        int hashIndex = fileID % N;
        file* currentFile = repo->fileHash[hashIndex];
        while (currentFile != NULL) {
            if (currentFile->id == fileID) {
                printf("  Content:\n%s\n", currentFile->content);
                break;
            }
            currentFile = currentFile->next;
        }

    }
    printf("\n");
}

char* getFileContent(int fileID, repository* repo) {
    // Retrieve file content based on file ID from the repository's file hash array
    int hashIndex = fileID % N;
    file* currentFile = repo->fileHash[hashIndex];
    while (currentFile != NULL) {
        if (currentFile->id == fileID) {
            return currentFile->content;
        }
        currentFile = currentFile->next;
    }
    return "File not found";
}


void freeCommitTree(commitNode* root) {
    if (root != NULL) {
        for (int i = 0; i < N; i++) {
            freeCommitTree(root->children[i]);
        }
        free(root->commit);
        free(root);
    }
}

// MAIN FUNCTION

int main() {
    // Initialize repository
    repository* myRepo = initRepository();

    srand(time(NULL));
    printf("Reached here 1\n");
    commit* commit1 = createCommit("Initial commit", 1, "John Doe");
    commit* commit2 = createCommit("Fix bug #123", 2, "Alice Smith");
    commit* commit3 = createCommit("Add feature X", 3, "Bob Johnson");
    printf("Reached here 1\n");

    // Add files to commits
    addFileToCommit(commit1, "C:/Users/Atharva/Desktop/Trial.txt", myRepo);
    //addFileToCommit(commit2, "file2.txt", "path/to/file2.txt", myRepo);
    //addFileToCommit(commit3, "file3.txt", "path/to/file3.txt", myRepo);
    printf("Reached here 1\n");

    // Displaying the commit tree
    displayCommit(commit1, myRepo);
    displayCommit(commit2, myRepo);
    displayCommit(commit3, myRepo);
    printf("Reached here 1\n");
    // Freeing allocated memory
    freeCommitTree(myRepo->root);

    // Freeing repository and file hash
    //free(myRepo);

    return 0;
}
