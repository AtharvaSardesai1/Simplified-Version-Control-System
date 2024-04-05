#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <string.h>

#define N 10000
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
} repository;

repository* initRepository() {
    repository* newRepo = (repository*)malloc(sizeof(repository));
    for (int i = 0; i < N; i++) {
        newRepo->nodes[i] = NULL;
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


graphNode* findCommonAncestor(graphNode* commit1, graphNode* commit2) {
    // Traverse ancestors of commit1 and check if they are ancestors of commit2
    graphNode* ancestor1 = commit1;
    while (ancestor1 != NULL) {
        graphNode* ancestor2 = commit2;
        while (ancestor2 != NULL) {
            if (ancestor1 == ancestor2) {
                return ancestor1; // Common ancestor found
            }
            ancestor2 = ancestor2->parent;
        }
        ancestor1 = ancestor1->parent;
    }

    return NULL; // No common ancestor found
}

void applyChanges(repository* repo, graphNode* commit, graphNode* commonAncestor) {
    // Apply changes from commit to the current branch
    while (commit != commonAncestor) {
        // Iterate over files in the commit and merge changes
        for (int i = 0; i < commit->commit->fileCount; i++) {
            int fileID = commit->commit->fileIDs[i];
            char* content = getFileContent(fileID, repo);

            // Check if file exists in common ancestor
            char* ancestorContent = getFileContent(fileID, repo);

            if (strcmp(content, ancestorContent) != 0) {
                // Files have diverged, handle conflict resolution here
                printf("Conflict detected in file %d. Manual resolution required.\n", fileID);
            } else {
                // Files are the same, no conflict
                // Perform automatic merge (if applicable) or keep the content as-is
                printf("File %d merged successfully.\n", fileID);
            }
        }

        // Move to the parent commit
        commit = commit->parent;
    }
}


void merge(repository* repo, graphNode* commit1, graphNode* commit2) {
    // Find common ancestor(s) of commit1 and commit2
    graphNode* commonAncestor = findCommonAncestor(commit1, commit2);

    if (commonAncestor == NULL) {
        printf("No common ancestor found. Merge aborted.\n");
        return;
    }

    // Apply changes from commit1 and commit2 to the current branch
    applyChanges(repo, commit1, commonAncestor);
    applyChanges(repo, commit2, commonAncestor);

    printf("Merge successful.\n");
}



void gitLog(repository* repo, graphNode* commit) {
    while (commit != NULL) {
        displayCommit(commit->commit, repo);
        commit = commit->parent; // Move to the parent commit
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

    addFileToCommit(node1->commit, "idris.txt", "C:/Users/adeeb/Downloads/idris.txt", myRepo);

    displayCommit(node1->commit, myRepo);
    displayCommit(node2->commit, myRepo);
    displayCommit(node3->commit, myRepo);

    freeCommitTree(node1);
    freeCommitTree(node2);
    freeCommitTree(node3);

    return 0;
}
