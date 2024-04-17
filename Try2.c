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

int nextFileID = 1;

typedef struct File {
    int fileID;
    char content[MAX_FILE_CONTENT_SIZE];
    struct File* next;
} File;

typedef struct commit {
    char message[100];
    int fileID;
    char author[50];
    char timestamp[20];
    int fileCount;
    int fileIDs[MAX_FILE_COUNT];
} commit;

typedef struct graphNode {
    struct commit* commit;
    struct graphNode* parent;
    struct graphNode* nextParent;
} graphNode;

typedef struct repository {
    graphNode* nodes[N]; // Array of pointers to the heads of linked lists
    File* fileHash[N];   // Array of linked lists for file storage
    int currentBranchIndex;
    int branchCount;
    char* branches[N];
} repository;

graphNode* createGraphNode(commit* commit) {
    graphNode* newNode = (graphNode*)malloc(sizeof(graphNode));
    if (newNode == NULL) {
        printf("Error: Memory allocation failed.\n");
        return NULL;
    }
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

void addParent(graphNode* child, graphNode* parent) {
        // Add parent to the beginning of the linked list
        parent->nextParent = child->parent;
        child->parent = parent;
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
    printf("111111\n");

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
    printf("22222222\n");

    commit* newCommit = (commit*)malloc(sizeof(commit));
    if (newCommit == NULL) {
        printf("Error: Memory allocation failed.\n");
        fclose(file);
        return NULL;
    }

    snprintf(newCommit->message, sizeof(newCommit->message), "%s", message);
    newCommit->fileID = newFile->fileID;
    snprintf(newCommit->author, sizeof(newCommit->author), "%s", author);

    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);
    strftime(newCommit->timestamp, sizeof(newCommit->timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    graphNode* newNode = createGraphNode(newCommit);
    printf("33333333333\n");

    index = id % N;
    if (repo->nodes[index] == NULL) {
        repo->nodes[index] = newNode;
    } else {
        graphNode* temp = repo->nodes[index];
        while (temp->nextParent != NULL) {
            temp = temp->nextParent;
        }
        temp->nextParent = newNode;
    }

    return newNode;
}

void cloneBranch(repository* repo, const char* originalBranchName, const char* newBranchName) {
    // Find the index of the original branch
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

    // Clone the original branch
    repo->branches[repo->branchCount] = strdup(newBranchName);
    repo->branchCount++;

    // Set the current branch index to the new branch
    repo->currentBranchIndex = repo->branchCount - 1;

    // Copy the contents of the main branch to the new branch
    graphNode* mainBranch = repo->nodes[0]; // Assuming main branch is at index 0
    graphNode* newBranch = repo->nodes[repo->branchCount - 1]; // New branch is at the last index
    while (mainBranch != NULL) {
        graphNode* newNode = createGraphNode(mainBranch->commit);
        addParent(newNode, newBranch); // Add the node to the new branch
        mainBranch = mainBranch->nextParent;
    }

    printf("Branch '%s' created from branch '%s'.\n", newBranchName, originalBranchName);
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

int main() {
    char repoName[100];
    printf("Enter repository name: ");
    scanf("%s", repoName);

    repository* myRepo = initRepository(repoName);

    char fileName[50];
    printf("Enter file name: ");
    scanf("%s", fileName);

    char message[100];
    printf("Enter commit message: ");
    scanf("%s", message);

    const char* author = "John Doe";

    graphNode* commit = commit_file(fileName, message, nextFileID, author, myRepo);

    char updatedFileName[50];
    printf("Enter updated file name: ");
    scanf("%s", updatedFileName);

    printf("Enter commit message: ");
    scanf("%s", message);

    printFileChanges(fileName, updatedFileName);

    graphNode* commit2 = commit_file(updatedFileName, message, nextFileID, author, myRepo);

    char branch[100];
    printf("Enter the branch name: ");
    scanf("%s", branch);

    cloneBranch(myRepo, "main", branch);
    printRepository(myRepo);

    // Print the contents of the repository's nodes
    printf("Contents of repository->nodes:\n");
    for (int i = 0; i < N; i++) {
        printf("Index %d:\n", i);
        graphNode* node = myRepo->nodes[i];
        while (node != NULL) {
            printf("Message: %s\n", node->commit->message);
            printf("File ID: %d\n", node->commit->fileID);
            printf("Author: %s\n", node->commit->author);
            printf("Timestamp: %s\n", node->commit->timestamp);
            printf("\n");
            node = node->nextParent;
        }
    }


    return 0;
}
