#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <string.h>

#define N 10
#define MAX_FILES_PER_COMMIT 5
#define MAX_FILE_CONTENT_SIZE 40000
#define MAX_USERS 100
#define MAX_USERNAME_LENGTH 50
#define MAX_PASSWORD_LENGTH 50
#define MAX_FILE_COUNT 100

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
    int fileCount;   
    int fileIDs[MAX_FILE_COUNT];
    char originalFileName[50];
} commit;

typedef struct graphNode {
    struct commit* commit;
    struct graphNode* parent; // Parent in the directed acyclic graph
    struct graphNode* nextParent; // Next parent in the linked list of parents
} graphNode;

typedef struct repository {
    struct graphNode* nodes[N];
    File* fileHash[N]; // Array of linked lists for file storage
    int currentBranchIndex; // Index of the current branch in the branch array
    int branchCount; // Total number of branches
    char* branches[N]; // Array to store branch names
} repository;

typedef struct stackNode {
    struct graphNode* node;
    struct stackNode* next;
} stackNode;

typedef struct commitStack {
    struct stackNode* top;
} commitStack;

typedef struct queueNode {
    graphNode* node;
    struct queueNode* next;
} queueNode;

// Define a queue structure
typedef struct queue {
    queueNode* front;
    queueNode* rear;
} queue;

//-----------------FUNCTIONS--------------------------------------------

graphNode* createGraphNode(commit* commit) {
    graphNode* newNode = (graphNode*)malloc(sizeof(graphNode));
    newNode->commit = commit;
    newNode->parent = NULL;
    newNode->nextParent = NULL;
    return newNode;
}

graphNode* createRepoNode(const char* repoName) {
    commit* newCommit = (commit*)malloc(sizeof(commit));
    if (newCommit == NULL) {
        printf("Error: Memory allocation failed.\n");
        return NULL;
    }

    snprintf(newCommit->message, sizeof(newCommit->message), "Initial commit for repository '%s'", repoName);
    newCommit->fileID = -1;
    snprintf(newCommit->author, sizeof(newCommit->author), "System");
    
    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);
    strftime(newCommit->timestamp, sizeof(newCommit->timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    graphNode* newNode = createGraphNode(newCommit);
    return newNode;
}

repository* initRepository(const char* repoName) {
    repository* newRepo = (repository*)malloc(sizeof(repository));
    if (newRepo == NULL) {
        printf("Error: Memory allocation failed.\n");
        return NULL;
    }

    for (int i = 0; i < N; i++) {
        newRepo->nodes[i] = NULL;
        newRepo->fileHash[i] = NULL;
        newRepo->branches[i] = NULL;
    }
    newRepo->currentBranchIndex = -1;
    newRepo->branchCount = 0;

    const char* defaultBranchName = "main";
    newRepo->branches[0] = strdup(defaultBranchName);
    newRepo->branchCount++;

    graphNode* repoNode = createRepoNode(repoName);
    newRepo->nodes[0] = repoNode;

    return newRepo;
}

void addParent(graphNode* child, graphNode* parent) {
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
    int differencesFound = 0;

    while (fgets(originalLine, sizeof(originalLine), originalFile) != NULL &&
           fgets(updatedLine, sizeof(updatedLine), updatedFile) != NULL) {
        if (strcmp(originalLine, updatedLine) != 0) {
            printf("Difference found in line %d:\n", lineNum);
            printf("Original: %s", originalLine);
            printf("Updated: %s", updatedLine);
            printf("\n");
            differencesFound = 1;
        }
        lineNum++;
    }

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

    if (!differencesFound) {
        printf("No differences found between %s and %s.\n", originalFileName, updatedFileName);
    }

    fclose(originalFile);
    fclose(updatedFile);
}

graphNode* commit_file(const char* fileName, const char* message, int id, const char* author, repository* repo) {
    FILE* file = fopen(fileName, "r");
    if (file == NULL) {
        printf("Error: Unable to open file.\n");
        return NULL;
    }

    File* newFile = (File*)malloc(sizeof(File));
    if (newFile == NULL) {
        printf("Error: Memory allocation failed.\n");
        fclose(file);
        return NULL;
    }

    newFile->fileID = id;
    newFile->next = NULL;
    newFile->content[0] = '\0';

    char line[MAX_FILE_CONTENT_SIZE];
    size_t bytesRead;
    while ((bytesRead = fread(line, 1, sizeof(line), file)) > 0) {
        strncat(newFile->content, line, bytesRead);
    }

    int index = id % N;
    if (repo->fileHash[index] == NULL) {
        repo->fileHash[index] = newFile;
    } else {
        newFile->next = repo->fileHash[index];
        repo->fileHash[index] = newFile;
    }

    commit* newCommit = (commit*)malloc(sizeof(commit));
    if (newCommit == NULL) {
        printf("Error: Memory allocation failed.\n");
        fclose(file);
        return NULL;
    }

    snprintf(newCommit->message, sizeof(newCommit->message), "%s", message);
    newCommit->fileID = newFile->fileID;
    snprintf(newCommit->author, sizeof(newCommit->author), "%s", author);
    snprintf(newCommit->originalFileName, sizeof(newCommit->originalFileName), "%s", fileName);

    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);
    strftime(newCommit->timestamp, sizeof(newCommit->timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    fclose(file);

    graphNode* newNode = createGraphNode(newCommit);

    index = newCommit->fileID % N;
    repo->nodes[index] = newNode;

    int currentBranchIndex = repo->currentBranchIndex;
    if (currentBranchIndex >= 0) {
        graphNode* parent = repo->nodes[currentBranchIndex];
        if (parent != NULL) {
            addParent(newNode, parent);
        }
    }

    return newNode;
}

void printFileList() {
    printf("File List:\n");
    File* temp = head;
    while (temp != NULL) {
        printf("File ID: %d\n", temp->fileID);
        printf("Content:\n%s\n", temp->content);
        temp = temp->next;
    }
}

void cloneBranch(repository* repo, const char* originalBranchName, const char* newBranchName) {
    int originalBranchIndex = -1;
    for (int i = 0; i < repo->branchCount; i++) {
        if (strcmp(repo->branches[i], originalBranchName) == 0) {
            originalBranchIndex = i;
            break;
        }
    }

    if (originalBranchIndex == -1) {
        printf("Error: Original branch '%s' not found.\n", originalBranchName);
        return;
    }

    int newBranchIndex = repo->branchCount;
    if (newBranchIndex < N) {
        repo->branches[newBranchIndex] = strdup(newBranchName);
        repo->branchCount++;

        graphNode* originalBranchHead = repo->nodes[originalBranchIndex];
        if (originalBranchHead == NULL) {
            printf("Error: No commits in the original branch.\n");
            return;
        }

        graphNode* newBranchHead = NULL;
        graphNode* lastNode = NULL;
        while (originalBranchHead != NULL) {
            graphNode* newNode = createGraphNode(originalBranchHead->commit);
            if (newBranchHead == NULL) {
                newBranchHead = newNode;
            }
            if (lastNode != NULL) {
                lastNode->nextParent = newNode;
            }
            lastNode = newNode;
            originalBranchHead = originalBranchHead->nextParent;
        }
        repo->nodes[newBranchIndex] = newBranchHead;
    } else {
        printf("Error: Maximum number of branches reached.\n");
    }

    printf("Branch '%s' created from branch '%s'.\n", newBranchName, originalBranchName);
}

void createBranch(repository* repo, const char* branchName) {
    if (repo->branchCount < N) {
        for (int i = 0; i < repo->branchCount; i++) {
            if (strcmp(repo->branches[i], branchName) == 0) {
                printf("Error: Branch '%s' already exists.\n", branchName);
                return;
            }
        }

        const char* originalBranchName = repo->branches[repo->currentBranchIndex];
        cloneBranch(repo, originalBranchName, branchName);
    } else {
        printf("Error: Maximum number of branches reached.\n");
    }
}

void checkoutBranch(repository* repo, const char* branchName) {
    int branchIndex = -1;
    for (int i = 0; i < repo->branchCount; i++) {
        if (strcmp(repo->branches[i], branchName) == 0) {
            branchIndex = i;
            break;
        }
    }

    if (branchIndex == -1) {
        printf("Branch not found: %s\n", branchName);
        return;
    }

    repo->currentBranchIndex = branchIndex;
    printf("Switched to branch: %s\n", branchName);
}

void freeCommitTree(graphNode* root) {
    if (root != NULL) {
        freeCommitTree(root->nextParent);
        free(root->commit);
        free(root);
    }
}

void printRepository(repository* repo) {
    printf("Repository Contents:\n");
    printf("Current Branch Index: %d\n", repo->currentBranchIndex);
    printf("Branch Count: %d\n", repo->branchCount);

    printf("Branches:\n");
    for (int i = 0; i < repo->branchCount; i++) {
        printf("%d: %s\n", i, repo->branches[i]);
    }

    printf("File Hash:\n");
    for (int i = 0; i < N; i++) {
        printf("Index %d:\n", i);
        File* file = repo->fileHash[i];
        while (file != NULL) {
            printf("File ID: %d\n", file->fileID);
            printf("Content:\n%s\n", file->content);
            file = file->next;
        }
    }

    printf("Commit Nodes:\n");
    for (int i = 0; i < N; i++) {
        printf("Index %d:\n", i);
        graphNode* node = repo->nodes[i];
        while (node != NULL) {
            printf("Message: %s\n", node->commit->message);
            printf("File ID: %d\n", node->commit->fileID);
            printf("Author: %s\n", node->commit->author);
            printf("Timestamp: %s\n", node->commit->timestamp);
            node = node->nextParent;
        }
    }
}

graphNode* findCommonAncestor(graphNode* commit1, graphNode* commit2) {
    graphNode* ancestor1 = commit1;
    while (ancestor1 != NULL) {
        graphNode* ancestor2 = commit2;
        while (ancestor2 != NULL) {
            if (ancestor1 == ancestor2) {
                return ancestor1;
            }
            ancestor2 = ancestor2->parent;
        }
        ancestor1 = ancestor1->parent;
    }

    return NULL;
}

char* getFileContent(int fileID, repository* repo) {
    int hashIndex = fileID % N;
    File* currentFile = repo->fileHash[hashIndex];
    while (currentFile != NULL) {
        if (currentFile->fileID == fileID) {
            return currentFile->content;
        }
        currentFile = currentFile->next;
    }
    return "File not found";
}

void applyChanges(repository* repo, graphNode* commit, graphNode* commonAncestor) {
    while (commit != commonAncestor) {
        for (int i = 0; i < commit->commit->fileCount; i++) {
            int fileID = commit->commit->fileIDs[i];
            char* content = getFileContent(fileID, repo);
            char* ancestorContent = getFileContent(fileID, repo);

            if (strcmp(content, ancestorContent) != 0) {
                printf("Conflict detected in file %d. Manual resolution required.\n", fileID);
            } else {
                printf("File %d merged successfully.\n", fileID);
            }
        }

        commit = commit->parent;
    }
}

void merge(repository* repo, graphNode* commit1, graphNode* commit2) {
    graphNode* commonAncestor = findCommonAncestor(commit1, commit2);

    if (commonAncestor == NULL) {
        printf("No common ancestor found. Merge aborted.\n");
        return;
    }

    applyChanges(repo, commit1, commonAncestor);
    applyChanges(repo, commit2, commonAncestor);

    printf("Merge successful.\n");
}

repository* copyRepository(repository* originalRepo) {
    char repoName[100];
    printf("Enter repository name: ");
    scanf("%s", repoName);
    repository* newRepo = initRepository(repoName);

    for (int i = 0; i < N; i++) {
        graphNode* current = originalRepo->nodes[i];
        while (current != NULL) {
            commit* newCommit = (commit*)malloc(sizeof(commit));
            memcpy(newCommit, current->commit, sizeof(commit));

            graphNode* newNode = createGraphNode(newCommit);

            int index = current->commit->fileID % N;
            if (newRepo->nodes[index] == NULL) {
                newRepo->nodes[index] = newNode;
            } else {
                newNode->nextParent = newRepo->nodes[index];
                newRepo->nodes[index] = newNode;
            }

            current = current->nextParent;
        }
    }

    for (int i = 0; i < N; i++) {
        File* currentFile = originalRepo->fileHash[i];
        while (currentFile != NULL) {
            File* newFile = (File*)malloc(sizeof(File));
            memcpy(newFile, currentFile, sizeof(File));

            int index = currentFile->fileID % N;
            if (newRepo->fileHash[index] == NULL) {
                newRepo->fileHash[index] = newFile;
            } else {
                newFile->next = newRepo->fileHash[index];
                newRepo->fileHash[index] = newFile;
            }

            currentFile = currentFile->next;
        }
    }

    return newRepo;
}

commitStack* initCommitStack() {
    commitStack* stack = (commitStack*)malloc(sizeof(commitStack));
    stack->top = NULL;
    return stack;
}

void push(graphNode* node, commitStack* stack) {
    stackNode* newNode = (stackNode*)malloc(sizeof(stackNode));
    newNode->node = node;
    newNode->next = stack->top;
    stack->top = newNode;
}

int isEmpty(commitStack* stack) {
    return stack->top == NULL;
}

graphNode* pop(commitStack* stack) {
    if (isEmpty(stack)) {
        printf("Error: Stack underflow\n");
        return NULL;
    }
    stackNode* temp = stack->top;
    graphNode* poppedNode = temp->node;
    stack->top = temp->next;
    free(temp);
    return poppedNode;
}

queue* initQueue() {
    queue* q = (queue*)malloc(sizeof(queue));
    q->front = q->rear = NULL;
    return q;
}

int isQueueEmpty(queue* q) {
    return q->front == NULL;
}

void enqueue(graphNode* node, queue* q) {
    queueNode* newNode = (queueNode*)malloc(sizeof(queueNode));
    newNode->node = node;
    newNode->next = NULL;
    if (q->rear == NULL) {
        q->front = q->rear = newNode;
        return;
    }
    q->rear->next = newNode;
    q->rear = newNode;
}

graphNode* dequeue(queue* q) {
    if (isQueueEmpty(q)) return NULL;
    queueNode* temp = q->front;
    graphNode* poppedNode = temp->node;
    q->front = q->front->next;
    if (q->front == NULL) q->rear = NULL;
    free(temp);
    return poppedNode;
}

void displayCommitInfo(graphNode* node) {
    printf("Commit ID: %d\n", node->commit->fileID);
    printf("Author: %s\n", node->commit->author);
    printf("Message: %s\n", node->commit->message);
    printf("\n");
}

void displayCommitHistory(commitStack* stack) {
    stackNode* current = stack->top;
    while (current != NULL) {
        displayCommitInfo(current->node);
        current = current->next;
    }
}

void deleteMostRecentCommit(repository* repo, commitStack* stack) {
    if (isEmpty(stack)) {
        printf("Error: Commit history is empty.\n");
        return;
    }

    graphNode* recentCommit = pop(stack);
    graphNode* recentNode = NULL;
    for (int i = 0; i < N; i++) {
        graphNode* current = repo->nodes[i];
        graphNode* prev = NULL;
        while (current != NULL) {
            if (current == recentCommit) {
                if (prev != NULL) {
                    prev->nextParent = current->nextParent;
                } else {
                    repo->nodes[i] = current->nextParent;
                }
                free(current->commit);
                free(current);
                return;
            }
            prev = current;
            current = current->nextParent;
        }
    }
}

graphNode* undoMove(commitStack* stack) {
    if (isEmpty(stack)) {
        printf("Error: Commit history is empty.\n");
        return NULL;
    }
    return stack->top->next->node;
}

void pushCommitsUsingBFS(graphNode* startNode, commitStack* stack) {
    queue* q = initQueue();
    
    int visited[N];
    memset(visited, 0, sizeof(visited));
    
    enqueue(startNode, q);
    visited[startNode->commit->fileID % N] = 1;

    while (!isQueueEmpty(q)) {
        graphNode* current = dequeue(q);
        push(current, stack);

        graphNode* parent = current->parent;
        while (parent != NULL) {
            if (!visited[parent->commit->fileID % N]) {
                enqueue(parent, q);
                visited[parent->commit->fileID % N] = 1;
            }
            parent = parent->nextParent;
        }
    }

    free(q);
}

void printBranchContent(repository* repo, const char* branchName) {
    printf("Content of branch '%s':\n", branchName);

    int branchIndex = -1;
    for (int i = 0; i < repo->branchCount; i++) {
        if (strcmp(repo->branches[i], branchName) == 0) {
            branchIndex = i;
            break;
        }
    }

    if (branchIndex == -1) {
        printf("Error: Branch '%s' not found.\n", branchName);
        return;
    }

    graphNode* headCommit = repo->nodes[branchIndex];
    while (headCommit != NULL) {
        for (int i = 0; i < headCommit->commit->fileCount; i++) {
            int fileID = headCommit->commit->fileIDs[i];
            char* content = getFileContent(fileID, repo);
            printf("File ID: %d\n", fileID);
            printf("Content:\n%s\n", content);
            printf("------------------------\n");
        }
        headCommit = headCommit->nextParent;
    }
}

int main() {
    repository* myRepo;
    commitStack* stack = initCommitStack();
    commitStack* stack2 = initCommitStack();
    graphNode* nextCommit;

    srand(time(NULL));
    char fileName[50];
    char updatedFileName[50];
    char message[100];
    char author[MAX_USERNAME_LENGTH];
    char branch[100];
    char branch2[100];
    char repoName[100];
    int choice;

    do {
        printf("\n--- Menu ---\n");
        printf("1. Init Repo\n");
        printf("2. Commit file\n");
        printf("3. Print file changes\n");
        printf("4. Create branch\n");
        printf("5. Checkout branch\n");
        printf("6. Merge\n");
        printf("7. Print repository\n");
        printf("8. Delete most recent commit\n");
        printf("9. Undo move\n");
        printf("10. Push commits using BFS\n");
        printf("11. Display commit history\n");
        printf("0. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                printf("Enter repository name: ");
                scanf("%s", repoName);

                myRepo = initRepository(repoName);
                break;

            case 2:
                printf("Enter file name: ");
                scanf("%s", fileName);
                printf("Enter commit message: ");
                scanf("%s", message);
                printf("Enter author name: ");
                scanf("%s", author);
                printf("Enter file ID: ");
                int fileID;
                scanf("%d", &fileID);

                graphNode* commitNode = commit_file(fileName, message, fileID, author, myRepo);
                if (commitNode != NULL) {
                    push(commitNode, stack);
                }
                break;

            case 3:
                if (stack->top == NULL || stack->top->next == NULL) {
                    printf("Error: There are not enough commits in the stack to compare files.\n");
                    break;
                }
                graphNode* recentCommit = stack->top->node;
                const char* originalFileName = recentCommit->commit->originalFileName;

                graphNode* recentCommit2 = stack->top->next->node;
                const char* updatedFileName = recentCommit2->commit->originalFileName;

                printFileChanges(originalFileName, updatedFileName);
                break;

            case 4:
                printf("Enter the branch name: ");
                fgets(branch, sizeof(branch), stdin);
                branch[strcspn(branch, "\n")] = 0; // Remove newline
                createBranch(myRepo, branch);
                printBranchContent(myRepo, branch);
                break;

            case 5:
                printf("Enter the branch name: ");
                fgets(branch, sizeof(branch), stdin);
                branch[strcspn(branch, "\n")] = 0; // Remove newline
                checkoutBranch(myRepo, branch);
                printBranchContent(myRepo, branch);
                break;

            case 6:
                printf("Enter commit 1 ID: ");
                int commitID1;
                scanf("%d", &commitID1);
                printf("Enter commit 2 ID: ");
                int commitID2;
                scanf("%d", &commitID2);

                graphNode* commit1 = myRepo->nodes[commitID1 % N];
                graphNode* commit2 = myRepo->nodes[commitID2 % N];
                merge(myRepo, commit1, commit2);
                break;

            case 7:
                printRepository(myRepo);
                break;

            case 8:
                deleteMostRecentCommit(myRepo, stack);
                break;

            case 9:
                nextCommit = undoMove(stack);
                if (nextCommit != NULL) {
                    printf("Next commit after undo move:\n");
                    displayCommitInfo(nextCommit);
                }
                break;

            case 10:
                printf("Enter commit ID: ");
                int bfsCommitID;
                scanf("%d", &bfsCommitID);
                graphNode* bfsCommit = myRepo->nodes[bfsCommitID % N];
                pushCommitsUsingBFS(bfsCommit, stack2);
                break;

            case 11:
                printf("History from stack:\n");
                displayCommitHistory(stack);
                printf("\nHistory from stack2:\n");
                displayCommitHistory(stack2);
                break;

            case 0:
                printf("Exiting program.\n");
                break;

            default:
                printf("Invalid choice. Please try again.\n");
        }
    } while (choice != 0);

    // Free allocated memory and exit
    // Clean up memory allocations here...

    return 0;
}