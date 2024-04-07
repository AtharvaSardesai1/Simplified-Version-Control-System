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

typedef struct stackNode {
    struct commit* commit;
    struct stackNode* next;
} stackNode;

typedef struct commitStack {
    struct stackNode* top;
} commitStack;


repository* initRepository() {
    repository* newRepo = (repository*)malloc(sizeof(repository));
    for (int i = 0; i < N; i++) {
        newRepo->nodes[i] = NULL;
        newRepo->fileHash[i] = NULL;
        newRepo->branches[i] = NULL;
    }
    newRepo->currentBranchIndex = -1; // No branch selected initially
    newRepo->branchCount = 0;
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

void addFileToCommit(commit* commit, const char* name, const char* filePath, repository* repo) {
    if (commit->fileCount < MAX_FILES_PER_COMMIT) {
        int fileID = rand(); // Use a better method for real projects

        commit->fileIDs[commit->fileCount] = fileID;

        FILE* file = fopen(filePath, "rb");
        if (file != NULL) {
            size_t bytesRead = fread(repo->fileHash[fileID % N]->content, 1, MAX_FILE_CONTENT_SIZE - 1, file);
            if (bytesRead > 0) {
                repo->fileHash[fileID % N]->content[bytesRead] = '\0'; // Null-terminate the content
            } else {
                perror("Error reading file content");
            }
            fclose(file);
        } else {
            perror("Error opening file");
            return;
        }

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

void freeCommitTree(graphNode* root) {
    if (root != NULL) {
        freeCommitTree(root->nextParent);
        free(root->commit);
        free(root);
    }
}

// Stack functions

commitStack* initCommitStack() {
    commitStack* stack = (commitStack*)malloc(sizeof(commitStack));
    stack->top = NULL;
    return stack;
}

void push(commit* commit, commitStack* stack) {
    stackNode* newNode = (stackNode*)malloc(sizeof(stackNode));
    newNode->commit = commit;
    newNode->next = stack->top;
    stack->top = newNode;
}

int isEmpty(commitStack* stack) {
    return stack->top == NULL;
}

commit* pop(commitStack* stack) {
    if (isEmpty(stack)) {
        printf("Error: Stack underflow\n");
        return NULL;
    }
    stackNode* temp = stack->top;
    commit* poppedCommit = temp->commit;
    stack->top = temp->next;
    free(temp);
    return poppedCommit;
}

// Function to create a new branch
void createBranch(repository* repo, const char* branchName) {
    if (repo->branchCount < N) {
        repo->branches[repo->branchCount] = strdup(branchName);
        repo->branchCount++;
        printf("Branch created: %s\n", branchName);
    } else {
        printf("Maximum number of branches reached.\n");
    }
}

// Function to switch to a different branch
void checkoutBranch(repository* repo, const char* branchName) {
    for (int i = 0; i < repo->branchCount; i++) {
        if (strcmp(repo->branches[i], branchName) == 0) {
            repo->currentBranchIndex = i;
            printf("Switched to branch: %s\n", branchName);
            return;
        }
    }
    printf("Branch not found: %s\n", branchName);
}

void displayCommitInfo(commit* commit) {
    printf("Commit ID: %d\n", commit->id);
    printf("Author: %s\n", commit->author);
    printf("Message: %s\n", commit->message);
    printf("\n");
}

void displayCommitHistory(commitStack* stack) {
    stackNode* current = stack->top;
    while (current != NULL) {
        displayCommitInfo(current->commit);
        current = current->next;
    }
}

// MAIN FUNCTION

int main() {
    repository* myRepo = initRepository();

    srand(time(NULL));

    graphNode* node1 = createGraphNode(createCommit("Initial commit", 1, "John Doe"));
    graphNode* node2 = createGraphNode(createCommit("Fix bug #123", 2, "Alice Smith"));
    graphNode* node3 = createGraphNode(createCommit("Add feature X", 3, "Bob Johnson"));

    addParent(node2, node1);
    addParent(node3, node2);

    addFileToCommit(node1->commit, "Trial.txt", "C:/Users/Atharva/Desktop/Trial.txt", myRepo);

    // Create and initialize commit stack
    commitStack* stack = initCommitStack();

    // Push commits onto the stack
    push(node1->commit, stack);
    push(node2->commit, stack);
    push(node3->commit, stack);

    // Display all commits
    printf("All Commits:\n");
    displayCommit(node1->commit, myRepo);
    displayCommit(node2->commit, myRepo);
    displayCommit(node3->commit, myRepo);

    printf("History :\n");
    // Display commit history
    displayCommitHistory(stack);

    // Creating branches
    createBranch(myRepo, "main");
    createBranch(myRepo, "feature1");
    createBranch(myRepo, "feature2");

    // Switching branches
    checkoutBranch(myRepo, "feature1");

    // Free stack memory
    stackNode* current = stack->top;
    while (current != NULL) {
        stackNode* temp = current;
        current = current->next;
        free(temp);
    }
    free(stack);

    freeCommitTree(node1);
    freeCommitTree(node2);
    freeCommitTree(node3);

    return 0;
}
